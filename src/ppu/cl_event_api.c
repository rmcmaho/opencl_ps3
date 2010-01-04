#include "cl.h"
#include "cl_ps3.h"

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
