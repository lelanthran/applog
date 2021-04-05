%module applog
%{
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
%}

   bool applog_startup (const char *path);
   void applog_shutdown (void);
   const char *applog_dirname (void);
   void applog_log (const char *source, size_t line, ...);

