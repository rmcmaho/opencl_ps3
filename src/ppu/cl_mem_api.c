#include "cl.h"
#include "cl_ps3.h"


/**
 * A buffer object is created using the following function
 *
 * context is a valid OpenCL context used to create the buffer object.
 *
 * flags is a bit-field that is used to specify allocation and usage information
 * such as the memory arena that should be used to allocate the buffer object
 * and how it will be used. Table 5.3 describes the possible values for flags
 *
 * size is the size in bytes of the buffer memory object to be allocated.
 *
 * host_ptr is a pointer to the buffer data that may already be allocated by the
 * application. The size of the buffer that host_ptr points to must be >= size
 * bytes. Passing in a pointer to an already allocated buffer on the host and
 * using it as a buffer object allows applications to share data efficiently
 * with kernels and the host.
 *
 * errcode_ret will return an appropriate error code. If errcode_ret is NULL, no
 * error code is returned.
 *
 * Returns a valid non-zero buffer object and errcode_ret is set to CL_SUCCESS if
 * the buffer object is created successfully. It returns a NULL value with one of
 * the following error values returned in errcode_ret:
 *
 * 1)errcode_ret returns CL_INVALID_CONTEXT if context is not a valid context.
 *
 * 2)errcode_ret returns CL_INVALID_VALUE if values specified in flags are not valid.
 *
 * 3)errcode_ret returns CL_INVALID_BUFFER_SIZE if size is 0 or is greater than
 * CL_DEVICE_MAX_MEM_ALLOC_SIZE value specified in table 4.3.
 *
 * 4)errcode_ret returns CL_INVALID_HOST_PTR if host_ptr is NULL and
 * CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or if host_ptr
 * is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set
 * in flags.
 *
 * 5)errorcode_ret returns CL_MEM_OBJECT_ALLOCATION_FAILURE if there is a failure to
 * allocate memory for buffer object.
 *
 * 6)errcode_ret returns CL_INVALID_OPERATION if the buffer object cannot be created for
 * all devices in context.
 *
 * 7)errcode_ret returns CL_OUT_OF_HOST_MEMORY if there is a failure to allocate
 * resources required by the OpenCL implementation on the host.
 *
 */

extern cl_mem clCreateBuffer (cl_context context,
                       cl_mem_flags flags,
                       size_t size,
		       void *host_ptr,
		       cl_int *errcode_ret)
{

  if(errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;

  if(context == NULL || context == (cl_context)0)
    {
      if(errcode_ret != NULL)
	*errcode_ret = CL_INVALID_CONTEXT;
      return (cl_mem)0;
    }

  if(size < 1)
    {
      if(errcode_ret != NULL)
	*errcode_ret = CL_INVALID_BUFFER_SIZE;
      return (cl_mem)0;
    }

  PRINT_DEBUG("\n====\tCreating buffer  \t====\n");

  cl_mem memobj = malloc(sizeof(struct _cl_mem));

  if(memobj == NULL)
    {
      if(errcode_ret != NULL)
	*errcode_ret = CL_OUT_OF_HOST_MEMORY;
      return (cl_mem)0;
    }

  PRINT_DEBUG("memobj location %p\n", memobj);

  memobj->mem_type = CL_MEM_OBJECT_BUFFER;
  memobj->mem_flags = flags;

  if(host_ptr == NULL && 
     ((flags & CL_MEM_USE_HOST_PTR) == CL_MEM_USE_HOST_PTR ||
      (flags & CL_MEM_COPY_HOST_PTR) == CL_MEM_COPY_HOST_PTR))
    {
      if(errcode_ret != NULL)
	*errcode_ret = CL_INVALID_HOST_PTR;
      return (cl_mem)0;
    }

  if(host_ptr != NULL && 
     ((flags & CL_MEM_USE_HOST_PTR) != CL_MEM_USE_HOST_PTR &&
      (flags & CL_MEM_COPY_HOST_PTR) != CL_MEM_COPY_HOST_PTR))
    {
      if(errcode_ret != NULL)
	*errcode_ret = CL_INVALID_HOST_PTR;
      return (cl_mem)0;
    }

  memobj->mem_size = size;
  memobj->mem_host_ptr = host_ptr;
  memobj->mem_context = context;
  memobj->mem_ref_count = 1;

  PRINT_DEBUG("\n====\tReturning buffer\t====\n");


  return memobj;

}

extern cl_int
clRetainMemObject(cl_mem memObj)
{
  if (memObj == (cl_mem)0)
    return CL_INVALID_COMMAND_QUEUE;
  else
    {
      (memObj->ref_count)++;
      return CL_SUCCESS;
    }
}


extern cl_int
clReleaseMemObject(cl_mem memObj)
{
  if (memObj == (cl_mem)0)
    return CL_INVALID_COMMAND_QUEUE;
  else
    {
      (memObj->ref_count)--;
      if(memObj->ref_count <= 0)
	{
	  free(memObj);
	}
      return CL_SUCCESS;
    }
}
