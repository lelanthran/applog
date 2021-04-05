
#include <stdio.h>
#include <stdlib.h>

#include "applog.h"

int main (void)
{
   int ret = EXIT_FAILURE;

   printf ("Starting applog tests (%s)\n", applog_version);
   ret = EXIT_SUCCESS;

errorexit:
   return ret;
}

