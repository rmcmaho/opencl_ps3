#include "cl.h"
#include "cl_ps3.h"
#include <stdlib.h>
#include <string.h>


/**
 * program is a program object with a successfully built executable.
 * kernel_name is a function name in the program declared with the
 * __kernel qualifer.
 *
 * errcode_ret will return an appropriate error code. If errcode_ret is
 * NULL, no error code is returned.
 *
 * clCreateKernel returns a valid non-zero kernel object and errcode_ret
 * is set to CL_SUCCESS if the kernel object is created successfully. It
 * returns a NULL value with one of the following error values returned in
 * errcode_ret:
        errcode_ret returns CL_INVALID_PROGRAM if program is not a valid
	program object.
        
	errcode_ret returns CL_INVALID_PROGRAM_EXECUTABLE if there is no
	successfully built executable for program.

        errcode_ret returns CL_INVALID_KERNEL_NAME if kernel_name is not
	found in program.

        errcode_ret returns CL_INVALID_KERNEL_DEFINITION if the function
	definition for __kernel function given by kernel_name such as the
	number of arguments, the argument types are not the same for all
	devices for which the program executable has been built.

        errcode_ret returns CL_INVALID_VALUE if kernel_name is NULL.

        errcode_ret returns CL_OUT_OF_HOST_MEMORY if there is a failure
	to allocate resources required by the OpenCL implementation o
	the host.
 *
 */

extern cl_kernel
clCreateKernel (cl_program program,
		const char * kernel_name,
		cl_int * errcode_ret )
{

  setErrCode(errcode_ret, CL_SUCCESS);

  if(program == NULL || program == (cl_program)0)
    {
      setErrCode(errcode_ret, CL_INVALID_PROGRAM);
      return (cl_kernel)0;
    }

  if(kernel_name == NULL)
    {
      setErrCode(errcode_ret, CL_INVALID_VALUE);
      return (cl_kernel)0;
    }

  PRINT_DEBUG("\n====\tCreating kernel  \t====\n");

  int compare_result;
  int i;
  PRINT_DEBUG("Num kernels: %d\n", program->program_num_kernels);
  for(i=0; i<program->program_num_kernels; i++)
    {
      PRINT_DEBUG("Compare %s with %s\n", kernel_name, program->program_kernel_names[i]);
      compare_result = strcmp(kernel_name, program->program_kernel_names[i]);
      PRINT_DEBUG("Compare result: %d\n", compare_result);
      if (compare_result == 0)
	break;
    }
  

  if(compare_result != 0)
    {
      PRINT_DEBUG("CL_INVALID_KERNEL_NAME");
      setErrCode(errcode_ret, CL_INVALID_KERNEL_NAME);
      PRINT_DEBUG("\n====\tReturning kernel  \t====\n");
      return (cl_kernel)0;
    }

  PRINT_DEBUG("Allocating space for kernel...\n");

  cl_kernel kernel = malloc(sizeof(struct _cl_kernel));

  if(kernel == NULL)
    {
      setErrCode(errcode_ret, CL_OUT_OF_HOST_MEMORY);
      return (cl_kernel)0;
    }

  PRINT_DEBUG("Space allocated.\n");

  int size = strlen(program->program_kernel_names[i]);
  kernel->kernel_function_name = malloc(size+1);

  PRINT_DEBUG("Copying function name...\n");

  strcpy(kernel->kernel_function_name, program->program_kernel_names[i]);
  kernel->kernel_function_name[size] = '\0';

  PRINT_DEBUG("Copying complete.\n");

  kernel->kernel_num_args = program->program_kernel_num_args[i];

  kernel->kernel_ref_count = 1;
  kernel->kernel_context = program->program_context;
  kernel->kernel_program = program;

  //This will be removed once testing is complete
  kernel->kernel_num_spe_args = 2;
  kernel->kernel_spe_args = malloc(sizeof(cl_ulong)*2);

  PRINT_DEBUG("\n====\tReturning kernel  \t====\n");
  
  return kernel;

}



/**
 * Used to set the argument value for a specific argument of a kernel.
 *
 * kernel is a valid kernel object.
 *
 * arg_index is the argument index. Arguments to the kernel are referred
 * by indices that go from 0 for the leftmost argument to n - 1, where n is
 * the total number of arguments declared by a kernel.
 *
 * For example, consider the following kernel:
      __kernel void
      image_filter (int n, int m,
                           __constant float *filter_weights,
                           __read_only image2d_t src_image,
                           __write_only image2d_t dst_image)
      {
            ...
      }

 * Argument index values for image_filter will be 0 for n, 1 for m, 2 for
 * filter_weights, 3 for src_image and 4 for dst_image.
 *
 * arg_value is a pointer to data that should be used as the argument value
 * for argument specified by arg_index. The argument data pointed to by
 * arg_value is copied and the arg_value pointer can therefore be reused by
 * the application after clSetKernelArg returns. The argument value
 * specified is the value used by all API calls that enqueue kernel
 * (clEnqueueNDRangeKernel and clEnqueueTask) until the argument value is
 * changed by a call to clSetKernelArg for kernel.
 *
 * If the argument is a memory object (buffer or image), the arg_value entry
 * will be a pointer to the appropriate buffer or image object. The memory
 * object must be created with the context associated with the kernel object.
 * A NULL value can also be specified if the argument is a buffer object in
 * which case a NULL value will be used as the value for the argument declared
 * as a pointer to __global or __constant memory in the kernel. If the
 * argument is declared with the __local qualifier, the arg_value entry must
 * be NULL. If the argument is of type sampler_t, the arg_value entry must
 * be a pointer to the sampler object. For all other kernel arguments, the
 * arg_value entry must be a pointer to the actual data to be used as
 * argument value.
 *
 * The memory object specified as argument value must be a buffer object
 * (or NULL) if the argument is declared to be a pointer of a built-in or
 * user defined type with the __global or __constant qualifier. If the
 * argument is declared with the __constant qualifier, the size in bytes
 * of the memory object cannot exceed CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE
 * and the number of arguments declared with the __constant qualifier cannot
 * exceed CL_DEVICE_MAX_CONSTANT_ARGS.
 *
 * The memory object specified as argument value must be a 2D image object
 * if the argument is declared to be of type image2d_t. The memory object
 * specified as argument value must be a 3D image object if argument is
 * declared to be of type image3d_t.
 *
 * arg_size specifies the size of the argument value. If the argument is a
 * memory object, the size is the size of the buffer or image object type.
 * For arguments declared with the __local qualifier, the size specified
 * will be the size in bytes of the buffer that must be allocated for the
 * __local argument. If the argument is of type sampler_t, the arg_size
 * value must be equal to sizeof(cl_sampler). For all other arguments, the
 * size will be the size of argument type.
 *
 * clSetKernelArg returns CL_SUCCESS if the function was executed
 * successfully. Otherwise, it returns one of the following errors:
        CL_INVALID_KERNEL if kernel is not a valid kernel object.

        CL_INVALID_ARG_INDEX if arg_index is not a valid argument index.

        CL_INVALID_ARG_VALUE if arg_value specified is NULL for an argument
	that is not declared with the __local qualifier or vice-versa.

        CL_INVALID_MEM_OBJECT for an argument declared to be a memory object
	when the specified arg_value is not a valid memory object.

	CL_INVALID_SAMPLER for an argument declared to be of type sampler_t
	when the specified arg_value is not a valid sampler object.

        CL_INVALID_ARG_SIZE if arg_size does not match the size of the data
	type for an argument that is not a memory object or if the argument
	is a memory object and arg_size != s
 *
 */

extern
cl_int clSetKernelArg (cl_kernel kernel,
                       cl_uint arg_index,
                       size_t arg_size,
                       const void *arg_value)
{
  
  if(arg_index < 0 || arg_index > 1)
    {
      return CL_INVALID_ARG_INDEX;
    }

  if(arg_size != sizeof(cl_ulong))
    {
      return CL_INVALID_ARG_SIZE;
    }

  (kernel->kernel_spe_args)[arg_index] = *(cl_ulong *)arg_value;
  

  return CL_SUCCESS;
}


extern cl_int
clRetainKernel(cl_kernel kernel)
{
  if (kernel == (cl_kernel) 0)
    return CL_INVALID_KERNEL;
  else
    {
      (kernel->kernel_ref_count)++;
      clRetainProgram(kernel->kernel_program);
      clRetainContext(kernel->kernel_context);
      return CL_SUCCESS;
    }
}

extern cl_int
clReleaseKernel(cl_kernel kernel)
{
  (kernel->kernel_ref_count)--;
  if(kernel->kernel_ref_count < 1)
    {
      clReleaseProgram(kernel->kernel_program);
      clReleaseContext(kernel->kernel_context);
      free(kernel->kernel_function_name);
      free(kernel->kernel_spe_args);
      free(kernel);
    }

  return CL_SUCCESS;
}
