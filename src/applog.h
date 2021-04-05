
#ifndef H_APPLOG
#define H_APPLOG

#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>

// This is a thread-safe logging library. The non-thread-safe functions are applog_startup()
// and applog_shutdown().


#define APPLOG(...)  applog_log (__FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

   // Specify a NULL path to get the default location for the plaform. If path is specified
   // and cannot be opened the default path is used and an error is signalled. In all cases
   // the function applog_shutdown() *MUST* be called after this function returns and before
   // the program ends.
   //
   // If the dirpath directory does not exist, (whether default or supplied) an attempt will
   // be made to create it. If all attempts to create it fail, then an error is signalled.
   //
   // RETURNS:
   //    0     Success.
   //    1     Could not use provided path, successfully used builtin defaults.
   //    -1    Fatal error, no logging path is sed and no logfile is available.
   int applog_startup (const char *dirpath);
   void applog_shutdown (void);

   // Retrieve the pathname of the directory that contains the logfiles. The returned path
   // may not necessarily be the one supplied to _startup(); it will always be the one that
   // is actually used.
   const char *applog_dirname (void);

   void applog_log (const char *source, size_t line, const char *fmts, ...);
   void applog_vlog (const char *source, size_t line, const char *fmts, va_list ap);


#ifdef __cplusplus
};
#endif

#endif

