//#ifndef __CELL_PLATFORM_C
//#define __CELL_PLATFORM_C

#include "cl.h"
#include "cl_ps3.h"
#include <libspe2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifndef PS3_DEBUG
#define PS3_DEBUG 0
#endif


#if PS3_DEBUG
#define PRINT_DEBUG(format, args...) printf(format, ##args);
#else
#define PRINT_DEBUG(format, args...)
#endif

// Helper functions

void
setErrCode(cl_int * errcode_ret, cl_int code)
{
  if(errcode_ret != NULL)
    *errcode_ret = code;
}


/**
 * struct for use with runKernelOnSPE()
 *
 */
typedef struct {
  spe_context_ptr_t spe;  
  cl_kernel kernel;
} kernel_spe_thread;

/**
 * Function for making SPE threads
 */
void *runKernelOnSPE(void *thread_arg)
{
  int ret;
  kernel_spe_thread *arg = (kernel_spe_thread *) thread_arg;
  unsigned int entry;
  spe_stop_info_t stop_info;
  cl_ulong *spe_args = arg->kernel->kernel_spe_args;

  entry = SPE_DEFAULT_ENTRY;
  ret = spe_context_run(arg->spe, &entry, 0, &spe_args[0], &spe_args[1], &stop_info);

  return 0;
}

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


cl_event createNewEvent(cl_command_queue command_queue,
			cl_command_type type,
			cl_uint num_events_in_wait_list,
			const cl_event *event_wait_list)
{

  cl_event new_event = malloc(sizeof (struct _cl_event));

  new_event->event_command_queue = command_queue;

  new_event->event_command_type = type;
  new_event->event_command_execution_status = CL_QUEUED;
  new_event->event_reference_count = 1;
  new_event->num_events_in_wait_list = num_events_in_wait_list;

  if(event_wait_list != NULL)
    {
      size_t list_size = num_events_in_wait_list * sizeof(cl_event);
      new_event->event_wait_list = malloc(list_size);
      
      memcpy(new_event->event_wait_list, event_wait_list, list_size);
    }else
    {
      new_event->event_wait_list = NULL;
    }

  //Default values
  new_event->kernel = (cl_kernel) 0;
  new_event->work_dim = 0;
  new_event->global_work_offset = 0;
  new_event->global_work_size = 0;
  new_event->local_work_size = 0;

  new_event->src_buffer = (cl_mem) 0;
  new_event->dst_buffer = (cl_mem) 0;
  new_event->blocking = CL_TRUE;
  new_event->src_offset = 0;
  new_event->dst_offset = 0;
  new_event->cb = 0;
  new_event->ptr = NULL;
  
  return new_event;
}

void setNDRangeEvent(cl_event event,
		     cl_kernel kernel,
		     cl_uint work_dim,
		     const size_t *global_work_offset,
		     const size_t *global_work_size,
		     const size_t *local_work_size)
{
  clRetainKernel(kernel);
  event->kernel = kernel;
  event->work_dim = work_dim;

  if(global_work_offset != NULL)
    event->global_work_offset = global_work_offset;
  if(global_work_size != NULL)
    event->global_work_size = global_work_size;
  if(local_work_size != NULL)
    event->local_work_size = local_work_size;

}


void *CommandQueueThread(void *cmd_queue_ptr)
{

  cl_command_queue cmd_queue = *((cl_command_queue *)cmd_queue_ptr);

  PRINT_DEBUG("CommandQueueThread: Command queue location %p\n", cmd_queue);
  PRINT_DEBUG("CommandQueueThread: Thread created. Locking mutex %p\n", cmd_queue->dataMutex);

  pthread_mutex_lock(cmd_queue->dataMutex);
  
  PRINT_DEBUG("CommandQueueThread: Mutex locked. Signalling thread is created\n");

  pthread_cond_signal(cmd_queue->dataPresentCondition);

  PRINT_DEBUG("CommandQueueThread: Unlocking mutex\n");

  pthread_mutex_unlock(cmd_queue->dataMutex);

  while(cmd_queue->stayAlive == CL_TRUE)
    {

      PRINT_DEBUG("CommandQueueThread: Locking mutex\n");

      pthread_mutex_lock(cmd_queue->dataMutex);

      PRINT_DEBUG("CommandQueueThread: Waiting on condition\n");

      pthread_cond_wait(cmd_queue->dataPresentCondition, cmd_queue->dataMutex);

      PRINT_DEBUG("CommandQueueThread: Condition Satisfied!\n");

      if(cmd_queue->stayAlive == CL_FALSE && cmd_queue->list == NULL)
	break;

      event_list list = cmd_queue->list;
      if(list == NULL)
	return 0;
      cl_event event = list->event;
      cmd_queue->list = list->next;

      pthread_mutex_unlock(cmd_queue->dataMutex);


      if(event->event_command_type == CL_COMMAND_NDRANGE_KERNEL)
	{

	  cl_kernel kernel = event->kernel;
	  clRetainKernel(kernel);
	  cl_uint work_dim = event->work_dim;
	  const size_t *global_work_size = event->global_work_size;

	  int ret;
	  int NUM_SPE = global_work_size[work_dim-1];
	  spe_context_ptr_t spe[NUM_SPE];
	  pthread_t thread[NUM_SPE];
	  kernel_spe_thread arg[NUM_SPE];
	  
	  int i;
	  for(i = 0; i < NUM_SPE; i++)
	    {
	      
	      spe[i] = spe_context_create(0, NULL);
	      
	      cl_program program = kernel->kernel_program;
	      clRetainProgram(program);

	      PRINT_DEBUG("CommandQueueThread: Loading spe program\n");
	      PRINT_DEBUG("CommandQueueThread: Program location %p\n", program);
	      PRINT_DEBUG("CommandQueueThread: Program elfs location %p\n",
			  program->program_elfs);
	      
	      spe_program_handle_t elf = *(program->program_elfs);
	      ret = spe_program_load(spe[i], &elf);

	      PRINT_DEBUG("CommandQueueThread: Program loaded\n");

	      arg[i].spe = spe[i];
	      arg[i].kernel = kernel;
	      
	      PRINT_DEBUG("CommandQueueThread: Creating spe threads\n");

	      ret = pthread_create(&thread[i], NULL, runKernelOnSPE, &arg[i]);
	      

	      clReleaseProgram(program);
	    }
	  
	  
	  //Should probably not do this here
	  PRINT_DEBUG(stderr, "Joining threads\n");
	  
	  for(i=0; i<NUM_SPE; i++)
	    {
	      pthread_join(thread[i], NULL);
	      ret = spe_context_destroy(spe[i]);
	    }
	  
	  PRINT_DEBUG(stderr, "All threads joined\n");
	  
	  clReleaseKernel(kernel);
	  
	}

    }

  PRINT_DEBUG("CommandQueueThread: Thread closing\n");

  return 0;
}


void addEventToCommandQueue(cl_event event, cl_command_queue cmd_queue)
{

  pthread_mutex_lock(cmd_queue->dataMutex);

  event_list last = cmd_queue->last_element;
  event_list newLast = malloc(sizeof(struct _event_list));
  newLast->event = event;
  newLast->next = NULL;

  if(last != NULL)
    {
      last->next = newLast;
      cmd_queue->last_element = newLast;
    }
  else
    {
      if(cmd_queue->list == NULL)
	{
	  cmd_queue->list = newLast;
	  cmd_queue->last_element = newLast;
	}
    }

  pthread_cond_broadcast(cmd_queue->dataPresentCondition);
  pthread_mutex_unlock(cmd_queue->dataMutex);

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
 * 1)errcode_ret returns CL_INVALID_CONTEXT if context is not a valid context.
 * 
 * 2)errcode_ret returns CL_INVALID_DEVICE if device is not a valid device or
 * is not associated with context.
 * 
 * 3)errcode_ret returns CL_INVALID_VALUE if values specified in properties
 * are not valid.
 * 
 * 4)errcode_ret returns CL_INVALID_QUEUE_PROPERTIES if values specified in
 * properties are valid but are not supported by the device.
 * 
 * 5)errcode_ret returns CL_OUT_OF_HOST_MEMORY if there is a failure to
 * allocate resources required by the OpenCL implementation on the host.
 *
 */

extern cl_command_queue clCreateCommandQueue (cl_context context,
			  cl_device_id device,
			  cl_command_queue_properties properties,
			  cl_int * errcode_ret)
{

  if(context == NULL || context == (cl_context)0)
    {
      *errcode_ret = CL_INVALID_CONTEXT;
      return (cl_command_queue)0; 
    }
  if(device == NULL || device == (cl_device_id)0 ||
     *(context->device_list) != device)
    {
      *errcode_ret = CL_INVALID_DEVICE;
      return (cl_command_queue)0;
    }

  PRINT_DEBUG("\n====\tCreating command queue\t====\n");

  cl_command_queue cmd_queue = malloc(sizeof(struct _cl_command_queue));

  if(cmd_queue == NULL)
    {
      *errcode_ret = CL_OUT_OF_HOST_MEMORY;
      return (cl_command_queue)0;
    }

  cmd_queue->queue_context = context;
  cmd_queue->queue_device = device;
  cmd_queue->queue_properties = properties;
  cmd_queue->queue_ref_count = 1;

  cmd_queue->dataMutex = malloc(sizeof(pthread_mutex_t));
  cmd_queue->dataPresentCondition = malloc(sizeof(pthread_cond_t));

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  *(cmd_queue->dataMutex) = mutex;

  pthread_cond_t con = PTHREAD_COND_INITIALIZER;
  *(cmd_queue->dataPresentCondition) = con;

  cmd_queue->stayAlive = CL_TRUE;

  cmd_queue->list = (event_list) 0;
  cmd_queue->last_element = (event_list) 0;

  PRINT_DEBUG("Create: Command queue location %p\n", cmd_queue);
  PRINT_DEBUG("Create: Mutex location %p\n", cmd_queue->dataMutex);

  pthread_mutex_lock(cmd_queue->dataMutex);

  PRINT_DEBUG("Create: Mutex locked\n");

  pthread_t *thread = malloc(sizeof(pthread_t));

  PRINT_DEBUG("Create: Creating thread\n");

  pthread_create(thread, NULL, CommandQueueThread, &cmd_queue);

  pthread_cond_wait(cmd_queue->dataPresentCondition, cmd_queue->dataMutex);    
  
  PRINT_DEBUG("Create: Thread created. Unlocking mutex\n");

  pthread_mutex_unlock(cmd_queue->dataMutex);

  cmd_queue->queue_thread = thread;

  PRINT_DEBUG("\n====\tReturning command queue\t====\n");

  return cmd_queue;

}


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


  return (cl_program)0;

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
  *(program->program_binaries) = malloc((*lengths)+1);

  strcpy(*(program->program_binaries),*((char **)binaries));

  char *name = *(program->program_binaries);
  name[(*lengths)] = '\0';
  program->program_elfs = spe_image_open(name);  

  if (!program->program_elfs) {
    *errcode_ret = CL_INVALID_BINARY;
    return (cl_program)0;
  }

  PRINT_DEBUG("\n====\tReturning program  \t====\n");

  return program;

}


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

  cl_kernel kernel = malloc(sizeof(struct _cl_kernel));

  if(kernel == NULL)
    {
      setErrCode(errcode_ret, CL_OUT_OF_HOST_MEMORY);
      return (cl_kernel)0;
    }

  int size = strlen(kernel_name);
  kernel->kernel_function_name = malloc(size+1);
  strcpy(kernel->kernel_function_name, kernel_name);
  kernel->kernel_function_name[size] = '\0';

  // SPE always has two arguments
  kernel->kernel_num_args = 2;

  kernel->kernel_ref_count = 1;
  kernel->kernel_context = program->program_context;
  kernel->kernel_program = program;

  kernel->kernel_num_spe_args = 2;
  kernel->kernel_spe_args = malloc(sizeof(cl_ulong)*2);

  PRINT_DEBUG("\n====\tReturning kernel  \t====\n");
  
  return kernel;

}


/**
 *
 * Enqueues a command to execute a kernel on a device.

 //TODO: Add the rest of the documentation

 */

cl_int clEnqueueNDRangeKernel (cl_command_queue command_queue,
                               cl_kernel kernel,
                               cl_uint work_dim,
                               const size_t *global_work_offset,
                               const size_t *global_work_size,
                               const size_t *local_work_size,
                               cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list,
                               cl_event *event)
{

  if(command_queue == NULL || command_queue == (cl_command_queue)0)
    {
      return CL_INVALID_COMMAND_QUEUE;
    }

  if(kernel == NULL || kernel == (cl_kernel)0)
    {
      return CL_INVALID_KERNEL;
    }

  PRINT_DEBUG("\n====\tCreating Event  \t====\n");

  cl_event new_event = createNewEvent(command_queue, CL_COMMAND_NDRANGE_KERNEL,
				      num_events_in_wait_list, event_wait_list);
  
  setNDRangeEvent(new_event, kernel, work_dim, global_work_offset,
		  global_work_size, local_work_size);

  
  //  clReleaseEvent(new_event);
  //  free(new_event);


  PRINT_DEBUG("\n====\tEnqueuing kernel  \t====\n");
  
  //Will only do one dimension
  if(work_dim != 1)
    {
      return CL_INVALID_WORK_DIMENSION;
    }

  cl_program program = kernel->kernel_program;


  PRINT_DEBUG("NDRange: Program location %p\n", program);
  PRINT_DEBUG("NDRange: Program elefs location %p\n",
	      program->program_elfs);

  addEventToCommandQueue(new_event, command_queue);

  if(num_events_in_wait_list > 0)
    {

      // Do appropriate stuff here for event wait list
    }

  PRINT_DEBUG("\n====\tKernel Enqueued \t====\n");

  return CL_SUCCESS;

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
 * Can be used to query information about a command-queue.
 *
 * command_queue specifies the command-queue being queried.
 *
 * param_name specifies the information to query.
 *
 * param_value is a pointer to memory where the appropriate result being
 * queried is returned. If param_value is NULL, it is ignored.
 *
 * param_value_size is used to specify the size in bytes of memory pointed
 * to by param_value. This size must be >= size of return type as
 * described in table 5.2. If param_value is NULL, it is ignored.
 *
 * param_value_size_ret returns the actual size in bytes of data being
 * queried by param_value. If param_value_size_ret is NULL, it is ignored. 
 * The list of supported param_name values and the information returned in
 * param_value by clGetCommandQueueInfo is described in table 5.2.
 *
 */

cl_int
clGetCommandQueueInfo (cl_command_queue cmd_queue,
		       cl_command_queue_info param_name,
		       size_t param_value_size,
		       void *param_value, size_t * param_value_size_ret)
{
  
  switch (param_name)
    {
    case CL_QUEUE_CONTEXT:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_context);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_context *) param_value = cmd_queue->queue_context;
      break;

    case CL_QUEUE_DEVICE:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_device_id);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_device_id *) param_value = cmd_queue->queue_device;
      break;

    case CL_QUEUE_REFERENCE_COUNT:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_uint);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_uint *) param_value = cmd_queue->queue_ref_count;
      break;

    case CL_QUEUE_PROPERTIES:
      if (param_value_size_ret != NULL)
	{
	  *param_value_size_ret = sizeof (cl_command_queue_properties);
	  if (param_value_size < *param_value_size_ret)
	    return CL_INVALID_VALUE;
	}
      if (param_value != NULL)
	*(cl_command_queue_properties *) param_value = cmd_queue->queue_properties;
      break;
      
      
    default:
      return CL_INVALID_VALUE;
    }

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
	  PRINT_DEBUG("Release: Locking mutex %p\n", command_queue->dataMutex);

	  pthread_mutex_lock(command_queue->dataMutex);
	  command_queue->stayAlive = CL_FALSE;

	  PRINT_DEBUG("Release: signalling thread\n");

	  pthread_cond_signal(command_queue->dataPresentCondition);

	  PRINT_DEBUG("Release: unlocking mutex\n");

	  pthread_mutex_unlock(command_queue->dataMutex);

	  PRINT_DEBUG("Waiting for thread to die\n");

	  pthread_join(*(command_queue->queue_thread), NULL);

	  PRINT_DEBUG("Thread dead. Freeing memory\n");

	  free(command_queue->dataMutex);
	  free(command_queue->dataPresentCondition);
	  free(command_queue);

	  PRINT_DEBUG("Memory freed\n");
	}
      return CL_SUCCESS;
    }
}


extern cl_int
clReleaseMemObject(cl_mem memObj)
{
  free(memObj);
  return CL_SUCCESS;
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

//#endif // __CELL_PLATFORM_C
