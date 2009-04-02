//#ifndef __CELL_PLATFORM_C
//#define __CELL_PLATFORM_C

#include "cl.h"
#include "cl_ps3.h"
#include <libspe2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PS3_DEBUG 1


// Helper functions


extern cl_context
createCellContext (cl_int * errcode_ret)
{

  //CBE Programmer's Guide - page 43
  if (errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;

  int node_count = spe_cpu_info_get (SPE_COUNT_PHYSICAL_CPU_NODES, -1);

  if (PS3_DEBUG)
    {
      fprintf (stderr, "Num nodes: %d\n", node_count);
      int phys_spes = spe_cpu_info_get (SPE_COUNT_PHYSICAL_SPES, -1);
      fprintf (stderr, "Num physical spes: %d\n", phys_spes);
      int usable_spes = spe_cpu_info_get (SPE_COUNT_USABLE_SPES, -1);
      fprintf (stderr, "Num usable spes: %d\n", usable_spes);
    }

  if (node_count < 1)
    {
      *errcode_ret = CL_DEVICE_NOT_AVAILABLE;
      return (cl_context) 0;
    }

  if (PS3_DEBUG)
    fprintf (stderr, "sizeof(cl_context) == %ud\n",
	     sizeof (struct _cl_context));

  cl_context context = malloc (sizeof (struct _cl_context));

  if (PS3_DEBUG)
    fprintf (stderr, "Setting cell device ID\n");

  setCellDeviceID (context);

  return context;
}

extern void
setCellDeviceID (cl_context context)
{

  cl_device_id device = malloc (sizeof (struct _cl_device_id));
  cl_device_id *devices = malloc (sizeof (cl_device_id *));
  *devices = device;
  context->device_list = devices;


  if (PS3_DEBUG)
    printf ("Context: %p List: %p  Device: %p\n", context,
	    context->device_list, &device);


  //debug info
  if (0)
    {
      printf ("context: %p\n", context);
      printf ("*context: %p\n", *context);
      printf ("context->device_list: %p\n", (context->device_list));
      printf ("*(context->device_list): %p\n", *(context->device_list));
      printf ("**(context->device_list): %p\n", **(context->device_list));
      printf ("device: %p\n", device);
      printf ("*device: %p\n", *device);
    }


  // Set device info

  cl_uint max_units = spe_cpu_info_get (SPE_COUNT_PHYSICAL_SPES, -1);
  cl_uint clock = 3192;

  device->device_type = CL_DEVICE_TYPE_CPU;
  device->device_max_compute_units = max_units;
  device->device_max_clock_frequency = clock;

  cl_uint dim = 3;

  device->device_max_work_item_dimensions = dim;
  device->device_max_work_item_sizes = malloc (sizeof (size_t) * dim);
  device->device_max_work_item_sizes[0] = max_units;
  device->device_max_work_item_sizes[1] = 1;
  device->device_max_work_item_sizes[2] = 1;
  device->device_max_work_group_size = max_units;

  char *name = "Cell Broadband Engine";
  char *vendor = "IBM";

  device->device_name = malloc (strlen (name) + 1);
  strcpy (device->device_name, name);
  device->device_vendor = malloc (strlen (vendor) + 1);
  strcpy (device->device_vendor, vendor);

  if (PS3_DEBUG)
    fprintf (stderr, "Device name and Vendor: %s-%s\n",
	     device->device_name, device->device_vendor);

  device->driver_version = NULL;
  device->device_profile = NULL;
  device->device_version = NULL;
  device->device_extensions = NULL;

  // in bytes converted from 10^3 to 2^10
  device->device_global_mem_size = 216768000;
  device->device_local_mem_type = CL_LOCAL;
  device->device_local_mem_size = 250000;

  device->device_endian_little = CL_FALSE;
  device->device_available = CL_TRUE;

}

/******************************************************************************/

// OpenCL Functions

/**
 * 
 * Creates an OpenCL context from a device type that identifies
 * the specific device(s) to use
 * 
 * properties specifies a list of context property names and their
 * corresponding values. Each property name is immediately
 * followed by the corresponding desired value. The list is
 * terminated with 0. properties is currently is reserved and
 * must be NULL.
 * 
 * device_type is a bit-field that identifies the type of device
 * and is described in table 4.2 in section 4.2.
 * 
 * pfn_notify and user_data are described in clCreateContext.
 * 
 * errcode_ret will return an appropriate error code. If
 * errcode_ret is NULL, no error code is returned.
 * 
 */
extern cl_context clCreateContextFromType
  (cl_context_properties * properties,
   cl_device_type device_type,
   logging_fn pfn_notify, void *user_data, cl_int * errcode_ret)
{

  if (PS3_DEBUG)
    fprintf (stderr, "\n====\tCreating context from type\t====\n");

  if (properties != NULL)
    {
      *errcode_ret = CL_INVALID_VALUE;
      return (cl_context) 0;
    }


  if (device_type == CL_DEVICE_TYPE_CPU)
    {
      if (PS3_DEBUG)
	fprintf (stderr, "Creating cell context\n");

      cl_context context = createCellContext (errcode_ret);

      if (errcode_ret != NULL && *errcode_ret != CL_SUCCESS)
	return (cl_context) 0;

      context->prop = properties;
      context->device_count = 1;
      context->ref_count = 0;

      if (PS3_DEBUG)
	fprintf (stderr, "Performing implicit context retain\n");

      clRetainContext (context);

      if (PS3_DEBUG)
	fprintf (stderr, "====\tReturning context from type\t====\n");

      return context;
    }
  else
    {
      *errcode_ret = CL_INVALID_DEVICE;
      return (cl_context) 0;
    }

}


/**
 * Creates a command-queue on a specific device
 * 
 * context must be a valid OpenCL context
 *
 * device must be a device associated with context. It can either be in the
 * list of devices specified when context is created using clCreateContext
 * or have the same device type as the device type specified when the
 * context is created using clCreateContextFromType.
 *
 * properties specifies a list of properties for the command-queue. This
 * is a bit-field and is described in table 5.1.
 *
 * errcode_ret will return an appropriate error code. If errcode_ret is
 * NULL, no error code is returned.
 *
 */

extern cl_command_queue clCreateCommandQueue (cl_context context,
			  cl_device_id device,
			  cl_command_queue_properties properties,
			  cl_int * errcode_ret)
{

  //TODO
  return (cl_command_queue)0;

}


/**
 * Gets specific information about an OpenCL device.
 * 
 * device is a device returned by clGetDeviceIDs.
 * 
 * param_name is an enum that identifies the device information
 * being queried. It can be one of the following values as
 * specified in table 4.3.
 * 
 * param_value is a pointer to memory location where appropriate
 * values for a given param_name as specified in table 4.3 will
 * be returned. If param_value is NULL, it is ignored.
 * 
 * param_value_size specifies the size in bytes of memory pointed
 * to by param_value. This size in bytes must be >= size of
 * return type specified in table 4.3.
 * 
 * param_value_size_ret returns the actual size in bytes of
 * data being queried by param_value. If param_value_size_ret
 * is NULL, it is ignored.
 * 
 * Returns CL_SUCCESS if the function is executed successfully.
 * It returns CL_INVALID_CONTEXT if context is not a valid
 * context, returns CL_INVALID_VALUE if param_name is not one
 * of the supported values or if size in bytes specified by
 * param_value_size is < size of return type as specified in
 * table 4.3 and param_value is not a NULL value.
 */

cl_int
clGetDeviceInfo (cl_device_id device,
		 cl_device_info param_name,
		 size_t param_value_size,
		 void *param_value, size_t * param_value_size_ret)
{

  switch (param_name)
    {
    case CL_DEVICE_TYPE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_type;
      break;

    case CL_DEVICE_MAX_COMPUTE_UNITS:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_max_compute_units;
      break;

    case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_max_work_item_dimensions;
      break;

    case CL_DEVICE_MAX_WORK_ITEM_SIZES:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret =
	    sizeof (cl_uint) * device->device_max_work_item_dimensions;
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	memcpy (param_value, device->device_max_work_item_sizes,
		sizeof (device->device_max_work_item_sizes));
      //param_value = device->device_max_work_item_sizes;     //already pointer
      break;

    case CL_DEVICE_MAX_WORK_GROUP_SIZE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_max_work_group_size;
      break;

    case CL_DEVICE_MAX_CLOCK_FREQUENCY:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_max_clock_frequency;
      break;

    case CL_DEVICE_GLOBAL_MEM_SIZE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_global_mem_size;
      break;

    case CL_DEVICE_LOCAL_MEM_TYPE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_local_mem_type;
      break;

    case CL_DEVICE_LOCAL_MEM_SIZE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_local_mem_size;
      break;

    case CL_DEVICE_ENDIAN_LITTLE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_endian_little;
      break;

    case CL_DEVICE_AVAILABLE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = device->device_available;
      break;

    default:
      return CL_INVALID_VALUE;
    }

  return CL_SUCCESS;

}


/**
 * Can be used to query information about a context.
 * 
 * context specifies the OpenCL context being queried.
 * 
 * param_name is an enum that specifies the information to query.
 * param_value is a pointer to memory where the appropriate
 * result being queried is returned. If param_value is NULL,
 * it is ignored.
 * 
 * param_value_size specifies the size in bytes of memory
 * pointed to by param_value. This size must be greater than
 * or equal to the size of return type as described in table 4.4.
 * 
 * param_value_size_ret returns the actual size in bytes of
 * data being queried by param_value. If param_value_size_ret
 * is NULL, it is ignored.
 * 
 * Returns CL_SUCCESS if the function is executed successfully.
 * It returns CL_INVALID_CONTEXT if context is not a valid
 * context, returns CL_INVALID_VALUE if param_name is not one
 * of the supported values or if size in bytes specified by
 * param_value_size is < size of return type as specified in
 * table 4.4 and param_value is not a NULL value.
 * 
 * 
 */


extern cl_int clGetContextInfo
  (cl_context context,
   cl_context_info param_name,
   size_t param_value_size, void *param_value, size_t * param_value_size_ret)
{

  switch (param_name)

    {
    case CL_CONTEXT_REFERENCE_COUNT:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	param_value = &(context->ref_count);
      break;

    case CL_CONTEXT_NUM_DEVICES:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	param_value = &(context->device_count);
      break;

    case CL_CONTEXT_DEVICES:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret =
	    sizeof (cl_device_id) * context->device_count;
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	memcpy (param_value, context->device_list,
		sizeof (context->device_list));
      break;

    case CL_CONTEXT_PROPERTIES:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_context_properties);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;

	}
      if (param_value != NULL)
	param_value = context->prop;
      break;

    default:
      return CL_INVALID_VALUE;
    }

  return CL_SUCCESS;

}


/**
 * Increments the context reference count. clRetainContext
 * returns CL_SUCCESS if the function is executed
 * successfully. It returns CL_INVALID_CONTEXT if context
 * is not a valid OpenCL context.
 * 
 */
extern cl_int
clRetainContext (cl_context context)
{
  if (context == (cl_context) 0)
    return CL_INVALID_CONTEXT;
  else
    {
      (context->ref_count)++;
      return CL_SUCCESS;
    }
}

extern cl_int
clReleaseContext (cl_context context)
{
  if (context == (cl_context) 0)
    return CL_INVALID_CONTEXT;
  else
    {
      (context->ref_count)--;
      if (context->ref_count <= 0)
	{
	  cl_device_id device = *(context->device_list);
	  free (device->device_max_work_item_sizes);
	  free (device->device_name);
	  free (device->device_vendor);
	  free (device);
	  free (context->device_list);
	  free (context);
	}
      return CL_SUCCESS;
    }
}

extern cl_int
clRetainCommandQueue(cl_command_queue command_queue)
{
  if (command_queue == (cl_command_queue) 0)
    return CL_INVALID_COMMAND_QUEUE;
  else
    {
      (command_queue->queue_ref_count)++;
      return CL_SUCCESS;
    }

}

extern cl_int
clReleaseCommandQueue(cl_command_queue command_queue)
{
  if (command_queue == (cl_command_queue) 0)
    return CL_INVALID_COMMAND_QUEUE;
  else
    {
      (command_queue->queue_ref_count)--;
      if(command_queue->queue_ref_count <= 0)
	{
	  free(command_queue);
	}
      return CL_SUCCESS;
    }
}


//#endif // __CELL_PLATFORM_C
