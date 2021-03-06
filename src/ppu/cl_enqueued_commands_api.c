#include "cl.h"
#include "cl_ps3.h"

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
