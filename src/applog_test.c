
#include <stdio.h>
#include <stdlib.h>

#include "applog.h"

int main (void)
{
   int ret = EXIT_FAILURE;

   printf ("Starting applog tests (%s)\n", applog_version);

   int rc = applog_startup (NULL); // TODO: Must also test with valid and invalid paths

   switch (rc) {
      case 0:  printf ("Initialised the applog library\n");  break;
      case 1:  printf ("Failed init: not fatal\n");          break;
      case -1: printf ("Failed init: fatal\n");
               goto errorexit;
               break;
   }

   APPLOG ("Testing, logfile path is [%s]\n", applog_dirname);

   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

