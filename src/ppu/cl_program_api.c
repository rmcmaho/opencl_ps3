#include "cl.h"
#include "cl_ps3.h"
#include "../lua/kernel_parser.h"

/**
 *
 * creates a program object for a context, and loads the source code
 * specified by the text strings in the strings array into the program
 * object. The devices associated with the program object are the devices
 * associated with context.
 *
 * context must be a valid OpenCL context.
 *
 * strings is an array of count pointers to optionally null-terminated
 * character strings that make up the source code.
 *
 * The lengths argument is an array with the number of chars in each
 * string (the string length). If an element in lengths is zero, its
 * accompanying string is null-terminated. If lengths is NULL, all strings
 * in the strings argument are considered null-terminated. Any length value
 * passed in that is greater than zero excludes the null terminator in its count.
 *
 * errcode_ret will return an appropriate error code. If errcode_ret is
 * NULL, no error code is returned.
 *
 * clCreateProgramWithSource returns a valid non-zero program object and
 * errcode_ret is set to CL_SUCCESS if the program object is created
 * successfully. It returns a NULL value with one of the following error
 * values returned in errcode_ret:
        errcode_ret returns CL_INVALID_CONTEXT if context is not a valid
	context.
   
	errcode_ret returns CL_INVALID_VALUE if count is zero or if strings
	or any entry in strings is NULL.
        
	errcode_ret returns CL_COMPILER_NOT_AVAILABLE if a compiler is not
	available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in table 4.3
	is set to CL_FALSE.
        
	errcode_ret returns CL_OUT_OF_HOST_MEMORY if there is a failure to
	allocate resources required by the OpenCL implementation on the host.
 *
 *
 */

extern
cl_program clCreateProgramWithSource (cl_context context,
                                      cl_uint count,
                                      const char **strings,
                                      const size_t *lengths,
                                      cl_int *errcode_ret)
{
  PRINT_DEBUG("\n====\tCreating program with source\t====\n");

  if(context == NULL || context == (cl_context)0)
    {
      setErrCode(errcode_ret, CL_INVALID_CONTEXT);
      return (cl_program)0;
    }

  
  if(lengths == NULL || strings == NULL || count <= 0)
    {
      setErrCode(errcode_ret, CL_INVALID_VALUE);
      return (cl_program)0;
    }

  FILE *outputFile;
  time_t seconds;
  seconds = time (NULL);
  char fileName[33];

  //convert integer to string
  sprintf(fileName,"%ld.c",seconds);

  PRINT_DEBUG("Creating file %s ...\n", fileName);
  
  if( (outputFile = fopen(fileName, "w")) == NULL)
    {
      fprintf(stderr, "Error opening file.\n");
      return (cl_program)0;
    }

  PRINT_DEBUG("File created successfully\n");
  PRINT_DEBUG("Writing to file ...\n");

  int i;
  size_t sumSourceSize = 0;
  for(i=0; i<count; i++)
    {
      fputs(strings[i], outputFile);
      sumSourceSize += lengths[i];
    }

  //close file so Lua can open it
  fclose(outputFile);

  PRINT_DEBUG("Finished writing to file.\n");

  cl_program program = malloc(sizeof(struct _cl_program));
  program->program_ref_count = 1;

  program->program_context = context;
  clRetainContext(context);

  program->program_num_devices = 0;
  program->program_devices = NULL;

  PRINT_DEBUG("Allocating memory for program_source\n");

  program->program_source = malloc(sumSourceSize);

  PRINT_DEBUG("Concatanating strings together...\n");
  //copy the first string and concat the rest
  strcpy(program->program_source, strings[0]);
  for(i=1; i<count; i++)
    {
      strcat(program->program_source, strings[i]);
    }

  lua_State *L = createLuaState();

  PRINT_DEBUG("Loading kernel file %s...\n", fileName);
  loadKernelFile(L, fileName);

  char *kernelName;
  int numArgs;

  loadNextKernel(L, &kernelName, &numArgs);

  if(numArgs < 0 || kernelName == NULL)
    {
      PRINT_DEBUG("ERROR:numArgs returned less than zero\n");
      setErrCode(errcode_ret, CL_INVALID_VALUE);
      return (cl_program)0;
    }

  
  // TODO: This next section is a bit messy.
  // It should be cleaned up.
  // Generating the linked list and then copying the data could
  //  be put into separate functions.

  //temporary linked list struct
  struct list_element {
    char *funcName;
    int numArgs;
    struct list_element * next;
  };

  struct list_element *head, *curr;

  head = NULL;

  int numKernels = 0;
  while(kernelName != NULL)
    {
      PRINT_DEBUG("Kernel name: %s  Num args: %d\n", kernelName, numArgs);

      numKernels++;

      curr = malloc(sizeof(struct list_element));
      curr->funcName = malloc(strlen(kernelName));
      strcpy(curr->funcName, kernelName);
      //free(kernelName);
      
      curr->numArgs = numArgs;
      curr->next = head;
      head = curr;
      
      loadNextKernel(L, &kernelName, &numArgs);
    }

  lua_close(L);
  //TODO: Delete the temporary file

  curr = head;

  PRINT_DEBUG("Transfering func names to program object\n");

  program->program_num_kernels = numKernels;
  program->program_kernel_names = malloc(numKernels * sizeof(char*));
  program->program_kernel_num_args = malloc(numKernels * sizeof(int));

  i=0;
  while(curr)
    {
      program->program_kernel_names[i] = malloc(strlen(curr->funcName));
      strcpy(program->program_kernel_names[i],curr->funcName);
      
      program->program_kernel_num_args[i] = curr->numArgs;

      //the structs should be freed
      curr = curr->next;
    }


  //TODO: This function probably has a bunch of memory leaks.
  PRINT_DEBUG("\n====\tReturn program  \t====\n");

  return program;

}



/**
 * Creates a program object for a context, and loads the binary bits
 * specified by binary into the program object.
 *
 * context must be a valid OpenCL context.
 *
 * device_list is a pointer to a list of devices that are in context.
 * device_list must be a non-NULL value. The binaries are loaded for
 * devices specified in this list.
 *
 * num_devices is the number of devices listed in device_list.
 * The devices associated with the program object will be the list of
 * devices specified by device_list. The list of devices specified by
 * device_list must be devices associated with context.
 *
 * lengths is an array of the size in bytes of the program binaries to be
 * loaded for devices specified by device_list.
 *
 * binaries is an array of pointers to program binaries to be loaded for
 * devices specified by device_list. For each device given by
 * device_list[i], the pointer to the program binary for that device is
 * given by binaries[i] and the length of this corresponding binary is given
 * by lengths[i].
 * lengths[i] cannot be zero and binaries[i] cannot be a NULL pointer.
 *
 * The program binaries specified by binaries contain the bits that describe
 * the program executable that will be run on the device(s) associated with
 * context. The program binary can consist of either or both:
 *      Device-specific executable(s), and/or,
 *      Implementation-specific intermediate representation (IR) which will
 *      be converted to the device-specific executable.
 *
 * binary_status returns whether the program binary for each device
 * specified in device_list was loaded successfully or not. It is an array
 * of num_devices entries and returns CL_SUCCESS in binary_status[i] if
 * binary was successfully loaded for device specified by device_list[i];
 * otherwise returns CL_INVALID_VALUE if lengths[i] is zero or if
 * binaries[i] is a NULL value or CL_INVALID_BINARY in binary_status[i] if
 * program binary is not a valid binary for the specified device. If
 * binary_status is NULL, it is ignored.
 *
 * errcode_ret will return an appropriate error code. If errcode_ret is NULL,
 * no error code is returned.
 *
 * clCreateProgramWithBinary returns a valid non-zero program object and
 * errcode_ret is set to CL_SUCCESS if the program object is created
 * successfully. It returns a NULL value with one of the following error
 * values returned in errcode_ret:
        errcode_ret returns CL_INVALID_CONTEXT if context is not a valid
	context.

        errcode_ret returns CL_INVALID_VALUE if device_list is NULL or
	num_devices is zero.

        errcode_ret returns CL_INVALID_DEVICE if OpenCL devices listed in
	device_list are not in the list of devices associated with context.

        errcode_ret returns CL_INVALID_VALUE if lengths or binaries are NULL
	or if any entry in lengths[i] is zero or binaries[i] is NULL.

        errcode_ret returns CL_INVALID_BINARY if an invalid program binary
	was encountered for any device. binary_status will return specific
	status for each device.

        errcode_ret returns CL_OUT_OF_HOST_MEMORY if there is a failure
	to allocate resources required by the OpenCL implementation on
	the host.
 *
 */

extern
cl_program clCreateProgramWithBinary (cl_context context,
                                      cl_uint num_devices,
                                      const cl_device_id *device_list,
                                      const size_t *lengths,
                                      const char **binaries,
                                      cl_int *binary_status,
                                      cl_int *errcode_ret)
{


  if(context == NULL || context == (cl_context)0)
    {
      *errcode_ret = CL_INVALID_CONTEXT;
      return (cl_program)0;
    }

  
  if(num_devices < 1 || device_list == NULL)
    {
      *errcode_ret = CL_INVALID_VALUE;
      return (cl_program)0;
    }

  if(lengths == NULL || binaries == NULL)
    {
      *errcode_ret = CL_INVALID_VALUE;
      return (cl_program)0;
    }

  PRINT_DEBUG("\n====\tCreating program  \t====\n");

  cl_program program = malloc(sizeof(struct _cl_program));

  program->program_ref_count = 1;
  program->program_context = context;
  program->program_num_devices = num_devices;

  //Should memcpy() these
  program->program_devices = device_list;

  PRINT_DEBUG("Set devices\n");

  program->program_source = NULL;
  program->program_binary_sizes = lengths;
   
  PRINT_DEBUG("Before malloc\n");
 
  program->program_binaries = malloc(sizeof(char *));

  PRINT_DEBUG("After first malloc\n");

  *(program->program_binaries) = malloc((*lengths)+1);

  PRINT_DEBUG("After second malloc\n");

  strcpy(*(program->program_binaries),*((char **)binaries));

  PRINT_DEBUG("After strcpy\n");

  char *name = *(program->program_binaries);
  name[(*lengths)] = '\0';

  PRINT_DEBUG("Opening spe image %s\n", name);

  program->program_elfs = spe_image_open(name);  

  if (!program->program_elfs) {
    PRINT_DEBUG("Could not open spe image\n");
    *errcode_ret = CL_INVALID_BINARY;
    return (cl_program)0;
  }

  PRINT_DEBUG("\n====\tReturning program  \t====\n");

  return program;

}



extern cl_int
clRetainProgram(cl_program program)
{
  if (program == (cl_program) 0)
    return CL_INVALID_PROGRAM;
  else
    {
      (program->program_ref_count)++;
      return CL_SUCCESS;
    }
}


extern cl_int
clReleaseProgram(cl_program program)
{
  (program->program_ref_count)--;
  if(program->program_ref_count < 1)
    {
      spe_image_close(program->program_elfs);
      free(*(program->program_binaries));
      free(program->program_binaries);
      free(program);
    }

  return CL_SUCCESS;
}
