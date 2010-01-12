//#include "cl.h"
#include "cl_ps3.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libspe2.h>

//function prototypes
void testDeviceInfo (cl_device_id);
int loadFile(char *fileName, char ***strings, size_t **lengths);

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef WORK_ITEMS
#define WORK_ITEMS 3
#endif

void
delete_memobjs(cl_mem *memobjs, int n)
{
  int i;
  for (i=0; i<n; i++)
    clReleaseMemObject(memobjs[i]);
}



int
main ()
{
  cl_context context;
  cl_command_queue cmd_queue;
  cl_device_id *devices;
  cl_program program = (cl_program)0;
  cl_kernel kernel;
  cl_mem memobjs[3];
  size_t global_work_size[1];
  size_t local_work_size[1];
  size_t cb;
  cl_int err;
  int n = WORK_ITEMS;
  cl_float4 srcA[n], srcB[n];

  printf ("OpenCL on PS3 \n");
  // printf("Num processors: %ld \n",sysconf(_SC_NPROCESSORS_CONF));
  // printf("Command returned %d \n",system("uname"));


  // create the OpenCL context on a CPU device
  context =
    clCreateContextFromType (NULL, CL_DEVICE_TYPE_CPU, NULL, NULL, NULL);

  if (context == (cl_context) 0)
    return -1;

  if (DEBUG)
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
  //free(devices);

  memobjs[0] = clCreateBuffer(context,
			      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			      sizeof(cl_float4) * n, srcA, &err);
  if (memobjs[0] == (cl_mem)0)
    {
      if(DEBUG)
	{
	  fprintf(stderr,"\nInvalid buffer\n\n");
	  fprintf(stderr,"Errcode: %d\n", err);
	}

      clReleaseCommandQueue(cmd_queue);
      clReleaseContext(context);
      return -1;
    }

  memobjs[1] = clCreateBuffer(context,
			      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			      sizeof(cl_float4) * n, srcB, NULL);
  if (memobjs[1] == (cl_mem)0)
    {
      delete_memobjs(memobjs, 1);
      clReleaseCommandQueue(cmd_queue);
      clReleaseContext(context);
      return -1;
    }

  memobjs[2] = clCreateBuffer(context,
			      CL_MEM_READ_WRITE,
			      sizeof(cl_float) * n, NULL, NULL);
  if (memobjs[2] == (cl_mem)0)
    {
      delete_memobjs(memobjs, 2);
      clReleaseCommandQueue(cmd_queue);
      clReleaseContext(context);
      return -1;
    }


  const char *input = "hello_spe.elf";
  size_t len = strlen(input);
  size_t *size = &len;
  /*  program = clCreateProgramWithBinary(context, 1, devices, 
				      size, &input,
				      NULL, &err);
  */
  
  char **strings;
  size_t *lengths;
  int count;
  count = loadFile("test_kernel.c",&strings, &lengths);
  //printf("length[6]: %d length[8]: %d\n", lengths[6], lengths[8]);
  //printf("strings[6]: %s strings[8]: %s\n", strings[6], strings[8]);
  //
  program = clCreateProgramWithSource(context, count, strings, lengths, &err);
  //


  if (program == (cl_program)0)
    {
      PRINT_DEBUG("Error creating program: %d\n", err);
      PRINT_DEBUG("Deleteing memobjs\n");
      delete_memobjs(memobjs, 3);
      PRINT_DEBUG("Releasing command queue\n");
      clReleaseCommandQueue(cmd_queue);
      PRINT_DEBUG("Releasing contextn\n");
      clReleaseContext(context);
      return -1;
    }

  free(devices);


  kernel = clCreateKernel(program, "hello_spe", NULL);
  if (kernel == (cl_kernel)0)
    {
      PRINT_DEBUG("Error creating kernel\n");
      delete_memobjs(memobjs, 3);
      clReleaseCommandQueue(cmd_queue);
      clReleaseContext(context);
      return -1;
    }

  cl_ulong argp = 12345;
  err = clSetKernelArg(kernel, 0, sizeof(cl_ulong), (void *) &argp);

  cl_ulong envp = 67890;
  err = clSetKernelArg(kernel, 1, sizeof(cl_ulong), (void *) &envp);


  // set work-item dimensions
  global_work_size[0] = n;
  local_work_size[0]= 1;

  // execute kernel
  err = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL,
			       global_work_size, local_work_size,
			       0, NULL, NULL);

  delete_memobjs(memobjs, 3);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(cmd_queue);
  clReleaseContext (context);


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


int loadFile(char *fileName, char ***strings, size_t **lengths)
{

  FILE *inputFile;

  if((inputFile = fopen(fileName, "r")) == NULL)
    {
      fprintf(stderr, "Error opening file.\n");
      return -1;
    }

  char buffer[256];
  int count = 0;

  while(fgets(buffer, sizeof(buffer), inputFile) != NULL )
    {
      count++;
    }

  rewind(inputFile);

  (*strings) = malloc(count*sizeof(char*));
  (*lengths) = malloc(count*sizeof(size_t));

  count = 0;
  while(fgets(buffer, sizeof(buffer), inputFile) != NULL )
    {
      size_t size = strlen(buffer);
      (*strings)[count] = malloc(size);
      strcpy((*strings)[count], buffer);

      //printf("%d: %s", count, (*strings)[count]);

      (*lengths)[count] = size;

      count++;
    }  


  return count;
}
