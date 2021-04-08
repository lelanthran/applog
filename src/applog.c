
#ifndef WITHOUT_THREAD_SAFETY
#define _POSIX_C_SOURCE 200112L
#include <pthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#include <unistd.h>

#include "applog.h"

#define TEMP_FILENAME      ("applog.tmp")
#define DEFAULT_PREFIX     ("applog")

static FILE *g_outfile = NULL;
static uint64_t g_start_time = 0;
char *g_dirname = NULL;
char *g_fname_prefix = NULL;

#ifndef WITHOUT_THREAD_SAFETY
static pthread_mutex_t g_lock;
#endif


static char *lstrcat (const char *s1, ...)
{
   char *ret = NULL;

   size_t nbytes = 1;
   const char *tmp = s1;

   va_list ap, ac;
   va_start (ap, s1);

   va_copy (ac, ap);
   while (tmp) {
      nbytes += strlen (tmp);
      tmp = va_arg (ac, const char *);
   }
   va_end (ac);

   if (!(ret = malloc (nbytes))) {
      va_end (ap);
      return NULL;
   }
   ret[0] = 0;

   while (s1) {
      strcat (ret, s1);
      s1 = va_arg (ap, const char *);
   }
   va_end (ap);
   return ret;
}

static const char *get_homedir (void)
{
   static char tmpbuf[1024 * 4];

   tmpbuf[0] = 0;
   tmpbuf[sizeof tmpbuf -1] = 0;

   if ((getenv ("HOME"))) {
      strncpy (tmpbuf, getenv ("HOME"), sizeof tmpbuf -1);
   } else {
      if ((getenv ("HOMEDRIVE"))) {
         strncpy (tmpbuf, getenv ("HOMEDRIVE"), sizeof tmpbuf -1);
         if ((getenv ("HOMEPATH"))) {
            strncat (tmpbuf, getenv ("HOMEPATH"), sizeof tmpbuf -1);
         }
      }
   }

   return tmpbuf;
}

static const char *get_currentdir (void)
{
   static char tmpbuf[1024 * 4];

   tmpbuf[0] = 0;
#ifdef PLATFORM_WINDOWS
   _getcwd (tmpbuf, sizeof tmpbuf -1);
#else
   getcwd (tmpbuf, sizeof tmpbuf -1);
#endif

   tmpbuf[sizeof tmpbuf -1] = 0;

   return tmpbuf;
}

static bool rename_all_files (void)
{
   bool error = true;
   char *fnames[10] = {
      NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL,
   };
   char num[4];
   char *newpath = NULL;

   size_t nfnames = sizeof fnames / sizeof fnames[0];

   for (size_t i=0; i<nfnames; i++) {
      snprintf (num, sizeof num, "%zu", i);
      if (!(fnames[i] = lstrcat (g_dirname, "/", g_fname_prefix, ".", num, NULL)))  {
         goto errorexit;
      }
   }

   // Not checking for errors here, as we expect a certain class of error anyway.
   for (int i=(nfnames - 2); i>=0; i--) {
      snprintf (num, sizeof num, "%i", i+1);
      free (newpath);
      newpath = lstrcat (g_dirname, "/", g_fname_prefix, ".", num, NULL);
      if (!newpath)
         goto errorexit;

      // printf ("Renaming [%s] -> [%s]\n", fnames[i], newpath);
      rename (fnames[i], newpath);
   }

   free (newpath);
   newpath = lstrcat (g_dirname, "/", g_fname_prefix, NULL);

   // printf ("Renaming [%s] -> [%s]\n", newpath, fnames[0]);
   rename (newpath, fnames[0]);

   error = false;

errorexit:
   free (newpath);
   for (size_t i=0; i<nfnames; i++) {
      free (fnames[i]);
   }
   return !error;
}

/* *********************************************************************** */

int applog_startup (const char *dirpath, const char *fname_prefix)
{
   int ret = -1;
   char *path_tmpfile = NULL,
        *path_logfile = NULL;

   if (!fname_prefix)
      fname_prefix = DEFAULT_PREFIX;

   /* *************************************************************
    *  1. Init g_lock, g_start_time, g_fname_prefix
    */
#ifndef WITHOUT_THREAD_SAFETY
   pthread_mutexattr_t attrs;
   pthread_mutexattr_init (&attrs);
   pthread_mutexattr_settype (&attrs, PTHREAD_MUTEX_RECURSIVE_NP);
   pthread_mutex_init (&g_lock, &attrs);
   pthread_mutexattr_destroy (&attrs);
#endif

   g_start_time = (uint64_t) time (NULL);

   if (!(g_fname_prefix = lstrcat (fname_prefix, NULL))) {
      return -1;
   }

   /* *************************************************************
    *  2. Determine the directory pathname and init g_dirname
    */
   static const char *paths[] = {
      // We populate [0] with parameter dirpath
      NULL,
      // We populate [1] with the $HOME variable
      NULL,
#ifdef PLATFORM_WINDOWS
      "c:/Windows/system32",
#else
      "/var/log/",
#endif
      // We populate [3] with the $PWD variable
      NULL,
   };

   size_t npaths = sizeof paths / sizeof paths[0];

   paths[0] = dirpath;
   paths[1] = get_homedir ();
   paths[3] = get_currentdir ();

   // If the first attempt succeeds then the return value of zero indicates
   // to the caller that their preferred dirpath was used.
   ret = 0;
   for (size_t i=0; i<npaths; i++) {

      free (path_tmpfile);
      if (!(path_tmpfile = lstrcat (paths[i], "/", TEMP_FILENAME, NULL)))
         continue;

      if ((g_outfile = fopen (path_tmpfile, "w"))!=NULL) {
         char *tmp = malloc (strlen (paths[i]) + 1);
         if (!tmp)
            return -1;
         strcpy (tmp, paths[i]);
         g_dirname = tmp;
         // printf ("%zu: Using [%s] in [%s]\n", i, path_tmpfile, g_dirname);
         break;
      }

      // If the first attempt failed, then subsequent attempts will return this
      // value, which indicates that we succeeded but not with the dirpath specified
      // by the caller.
      ret = 1;
   }

   free (path_tmpfile); path_tmpfile = NULL;
   fclose (g_outfile); g_outfile = NULL;

   if (!g_dirname) {
      return -1;
   }

   /* *************************************************************
    *  3. Rename all the matching files in the directory pathname
    */
   if (!(rename_all_files ())) {
      return -1;
   }

   /* *************************************************************
    *  4. Create the current file
    */
   if (!(path_tmpfile = lstrcat (g_dirname, "/", TEMP_FILENAME, NULL)) ||
       !(path_logfile = lstrcat (g_dirname, "/", fname_prefix, NULL))) {
      free (path_tmpfile);
      return -1;
   }

   int rc = rename (path_tmpfile, path_logfile);

   g_outfile = fopen (path_logfile, "w");

   free (path_tmpfile); path_tmpfile = NULL;
   free (path_logfile); path_logfile = NULL;

   if (rc!=0 || g_outfile==NULL) {
      return -1;
   }

   /* *************************************************************
    *  5. Write the start event log entry
    */
   char hrtime[30];
   memset (hrtime, 0, sizeof hrtime);
   const char *sztime = ctime ((const time_t *)&g_start_time);
   if (sztime) {
      strncpy (hrtime, sztime, sizeof hrtime - 1);
   }
   if ((strchr (hrtime, '\n'))) {
      (strchr (hrtime, '\n'))[0] = 0;
   }
   APPLOG ("%" PRIu64 ":Started applog (%s, %s)\n", g_start_time, dirpath, fname_prefix);

   return ret;
}

void applog_shutdown (void)
{
   if (g_outfile && g_outfile!=stdout && g_outfile!=stderr) {
      fclose (g_outfile);
      g_outfile = NULL;
   }

   free (g_dirname);
   free (g_fname_prefix);

#ifndef WITHOUT_THREAD_SAFETY
   pthread_mutex_destroy (&g_lock);
   memset (&g_lock, 0, sizeof g_lock);
#endif
}


const char *applog_dirname (void)
{
   return g_dirname;
}

void applog_log (const char *source, size_t line, const char *fmts, ...)
{
   va_list ap;
   va_start (ap, fmts);
   applog_vlog (source, line, fmts, ap);
   va_end (ap);
}

void applog_vlog (const char *source, size_t line, const char *fmts, va_list ap)
{
   uint64_t now = (uint64_t) time (NULL);
#ifndef WITHOUT_THREAD_SAFETY
   pthread_mutex_lock (&g_lock);
#endif
   fprintf (g_outfile, "+%" PRIu64 ":%s:%zu:", now - g_start_time, source, line);
   vfprintf (g_outfile, fmts, ap);
#ifndef WITHOUT_THREAD_SAFETY
   pthread_mutex_unlock (&g_lock);
#endif
}

