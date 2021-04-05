
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#ifndef WITHOUT_THREAD_SAFETY
#include <pthread.h>
#endif

#include "applog.h"

static FILE *g_outfile = NULL;
static uint64_t g_start_time = 0;
char *g_dirname = NULL;

#ifndef WITHOUT_THREAD_SAFETY
static pthread_mutex_t g_lock;
#endif


int applog_startup (const char *dirpath)
{
   // 1. Init g_lock, g_start_time
   pthread_mutexattr_t attrs;
   pthread_mutexattr_init (&attrs);
   pthread_mutexattr_settype (&attrs, PTHREAD_MUTEX_RECURSIVE_NP);
   pthread_mutex_init (&g_lock, &attrs);
   pthread_mutexattr_destroy (&attrs);

   g_start_time = (uint64_t) time (NULL);


   // 2. Determine the directory pathname, init g_dirname
   // 3. Rename all the matching files in the directory pathname
   // 4. Create the current file
   // 5. Write the start event log entry
   return 0;
}

void applog_shutdown (void)
{
   if (g_outfile && g_outfile!=stdout && g_outfile!=stderr) {
      fclose (g_outfile);
      g_outfile = NULL;
   }

   free (g_dirname);

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
   pthread_mutex_lock (&g_lock);
   fprintf (g_outfile, "+%" PRIu64 ":%s:%zu:", now - g_start_time, source, line);
   vfprintf (g_outfile, fmts, ap);
   pthread_mutex_unlock (&g_lock);
}

