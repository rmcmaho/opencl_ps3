//#ifndef __CELL_PLATFORM_C
//#define __CELL_PLATFORM_C

#include "cl.h"
#include "cl_ps3.h"

/*
#include <libspe2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

*/

// Helper functions

extern void
setErrCode(cl_int * errcode_ret, cl_int code)
{
  if(errcode_ret != NULL)
    *errcode_ret = code;
}


//#endif // __CELL_PLATFORM_C
