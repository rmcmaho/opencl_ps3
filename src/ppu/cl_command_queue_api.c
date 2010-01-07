#include "cl.h"
#include "cl_ps3.h"
#include <stdlib.h>

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
