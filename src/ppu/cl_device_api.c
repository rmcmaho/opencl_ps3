
/**
 * The list of devices available can be obtained using the following function.
 * 
 * device_type is a bitfield that identifies the type of OpenCL device. The
 * device_type can be used to query specific OpenCL devices or all OpenCL
 * devices available. The valid values for device_type are specified in table 4.2.
 *
 * num_entries is the number of cl_device entries that can be added to devices.
 * If devices is not NULL, the num_entries must be greater than zero.
 *
 * devices returns a list of OpenCL devices found. The cl_device_id values
 * returned in devices can be used to identify a specific OpenCL device. If
 * devices argument is NULL, this argument is ignored. The number of OpenCL
 * devices returned is the mininum of the value specified by num_entries or
 * the number of OpenCL devices whose type matches device_type.
 * 
 * num_devices returns the number of OpenCL devices available that match
 * device_type. If num_devices is NULL, this argument is ignored.
 *
 * clGetDeviceIDs returns CL_INVALID_DEVICE_TYPE if device_type is not a
 * valid value, returns CL_INVALID_VALUE if num_entries is equal to zero
 * and devices is not NULL or if both num_devices and devices are NULL,
 * returns CL_DEVICE_NOT_FOUND if no OpenCL devices that matched device_type
 * were found, and returns CL_SUCCESS if the function is executed successfully.
 *
 */


extern
cl_int clGetDeviceIDs (cl_device_type device_type,
                       cl_uint num_entries,
                       cl_device_id *devices,
                       cl_uint *num_devices)
{

  //TODO: This should replace the helper functions at the
  //      beginning of the file

  if((num_entries < 1 && devices == NULL) ||
     (devices == NULL && num_devices == NULL))
    return CL_INVALID_VALUE;
  
  int spe_count = spe_cpu_info_get (SPE_COUNT_PHYSICAL_CPU_NODES, -1);
  
  if(spe_count < 1)
    return CL_DEVICE_NOT_FOUND;
  
  if(num_devices != NULL)
    *num_devices = 1;
  
  
  if(device_type == CL_DEVICE_TYPE_CPU)
    {

      //TODO
      cl_device_id device = malloc (sizeof (struct _cl_device_id));
      *devices = device;
  

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
      
      PRINT_DEBUG("Device name and Vendor: %s-%s\n",
		  device->device_name, device->device_vendor);
      
      device->driver_version = NULL;
      device->device_profile = NULL;
      device->device_version = NULL;
      device->device_extensions = NULL;
      
      // in bytes converted from 10^3 to 2^10
      device->device_global_mem_size = 216768000;
      device->device_local_mem_type = CL_LOCAL;
      device->device_local_mem_size = 250000;
      
      
      spe_context_ptr_t spe;
      spe = spe_context_create(0, NULL);

      PRINT_DEBUG("SPE Local Store size: %d\n", spe_ls_size_get(spe));
      
      device->device_max_mem_alloc_size = spe_ls_size_get(spe);
      
      device->device_endian_little = CL_FALSE;
      device->device_available = CL_TRUE;
      
    }
  else
    return CL_INVALID_DEVICE_TYPE;
  
  
  return CL_SUCCESS;
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
