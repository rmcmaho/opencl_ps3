//#include "cl.h"
#include "cl_ps3.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//function prototypes
void testDeviceInfo (cl_device_id);

#define DEBUG 1

int
main ()
{
  cl_context context;
  cl_command_queue cmd_queue;
  cl_device_id *devices;
  cl_program program;
  cl_kernel kernel;
  cl_mem memobjs[3];
  size_t global_work_size[1];
  size_t local_work_size[1];
  size_t cb;
  cl_int err;

  printf ("OpenCL on PS3 \n");
  // printf("Num processors: %ld \n",sysconf(_SC_NPROCESSORS_CONF));
  // printf("Command returned %d \n",system("uname"));


  // create the OpenCL context on a CPU device
  context =
    clCreateContextFromType (NULL, CL_DEVICE_TYPE_CPU, NULL, NULL, NULL);

  if (context == (cl_context) 0)
    return -1;

  if (0)
    {
      printf ("Outside\n");
      printf ("context: %p\n", context);
      printf ("*context: %p\n", *context);
      printf ("context->device_list: %p\n", (context->device_list));
      printf ("*(context->device_list: %p\n", *(context->device_list));
      printf ("**(context->device_list: %p\n", **(context->device_list));

      cl_device_id test = *(context->device_list);
      printf ("Out Max units: %d\n", test->device_max_compute_units);
    }


  // get the list of GPU devices associated with context
  clGetContextInfo (context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
  devices = malloc (cb);
  clGetContextInfo (context, CL_CONTEXT_DEVICES, cb, devices, NULL);

  testDeviceInfo (*devices);
  
  // create a command-queue
  cmd_queue = clCreateCommandQueue(context, devices[0], 0, NULL);
  if (cmd_queue == (cl_command_queue)0)
    {
      if(DEBUG)
	fprintf(stderr,"\nInvalid command queue\n\n");

      clReleaseContext(context);
      return -1;
    }
  free(devices);

  clReleaseContext (context);
  clReleaseCommandQueue(cmd_queue);

  return 0;
}


void
testDeviceInfo (cl_device_id device)
{

  size_t size = sizeof (cl_uint);
  cl_uint value = 0;

  fprintf(stderr, "\n\n");
  fprintf(stderr, "====\t%20s\t====\n\n", "Testing Device Info");

  clGetDeviceInfo (device, CL_DEVICE_TYPE, size, &value, NULL);
  fprintf (stderr, "Device type: %d\n", value);

  clGetDeviceInfo (device, CL_DEVICE_MAX_COMPUTE_UNITS, size, &value, NULL);
  fprintf (stderr, "Max compute units: %d\n", value);

  clGetDeviceInfo (device, CL_DEVICE_MAX_CLOCK_FREQUENCY, size, &value, NULL);
  fprintf (stderr, "Max clock: %d\n", value);

  clGetDeviceInfo (device, CL_DEVICE_GLOBAL_MEM_SIZE, size, &value, NULL);
  fprintf (stderr, "Max mem: %d\n", value);

  fprintf(stderr, "\n====\t%-20s\t====\n\n", "END");

}
