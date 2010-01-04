extern cl_context
createCellContext (cl_int * errcode_ret)
{

  //CBE Programmer's Guide - page 43
  setErrCode(errcode_ret, CL_SUCCESS);


  int node_count = spe_cpu_info_get (SPE_COUNT_PHYSICAL_CPU_NODES, -1);

  PRINT_DEBUG("Num nodes: %d\n", node_count);
  int phys_spes = spe_cpu_info_get (SPE_COUNT_PHYSICAL_SPES, -1);
  PRINT_DEBUG("Num physical spes: %d\n", phys_spes);
  int usable_spes = spe_cpu_info_get (SPE_COUNT_USABLE_SPES, -1);
  PRINT_DEBUG("Num usable spes: %d\n", usable_spes);
  

  if (node_count < 1)
    {
      setErrCode(errcode_ret, CL_DEVICE_NOT_AVAILABLE);
      return (cl_context) 0;
    }

  PRINT_DEBUG("sizeof(cl_context) == %d\n", sizeof (struct _cl_context));

  cl_context context = malloc (sizeof (struct _cl_context));

  
  return context;
}


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

  PRINT_DEBUG(stderr, "\n====\tCreating context from type\t====\n");

  if (properties != NULL)
    {
      setErrCode(errcode_ret, CL_INVALID_VALUE);
      return (cl_context) 0;
    }


  if (device_type == CL_DEVICE_TYPE_CPU)
    {
      PRINT_DEBUG(stderr, "Creating cell context\n");

      cl_context context = createCellContext (errcode_ret);

      if (errcode_ret != NULL && *errcode_ret != CL_SUCCESS)
	return (cl_context) 0;

      context->prop = properties;
      context->device_count = 1;
      context->ref_count = 0;
      
      cl_device_id *device_id = malloc(sizeof (cl_device_id *));

      cl_int err = clGetDeviceIDs(CL_DEVICE_TYPE_CPU, 1, device_id, NULL);

      if (err != CL_SUCCESS)
	{
	  setErrCode(errcode_ret, err);
	  return (cl_context) 0;
	}

      context->device_list = device_id;

      PRINT_DEBUG(stderr, "Performing implicit context retain\n");

      clRetainContext (context);

      PRINT_DEBUG(stderr, "====\tReturning context from type\t====\n");

      return context;
    }
  else
    {
      *errcode_ret = CL_INVALID_DEVICE;
      return (cl_context) 0;
    }

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
