/**********************************************************************************
 * Copyright (c) 2008 The Khronos Group Inc.
 * Copyright (c) 2009 Robbie McMahon, Loyola University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 **********************************************************************************/

#ifndef __CL_PS3_H
#define __CL_PS3_H

//#include <OpenCL/cl.h>
#include "cl.h"
#include <libspe2.h>
#include <pthread.h>

#ifndef __kernel
#define __kernel
#endif

//
//Helper functions and data types
//

#define CELL_API_CALL

extern cl_context CELL_API_CALL createCellContext (cl_int *);

typedef struct _event_list *event_list;

struct _event_list
{
  cl_event event;
  event_list next;
};

//
//OpenCL data types
//

// Inferred from Table 4.3 of OpenCL Spec
struct _cl_device_id
{
  cl_device_type device_type;
  cl_uint device_vendor_id;
  cl_uint device_max_compute_units;
  cl_uint device_max_work_item_dimensions;
  size_t *device_max_work_item_sizes;
  size_t device_max_work_group_size;
  cl_uint device_preferred_vector_width;
  cl_uint device_max_clock_frequency;
  cl_bitfield device_address_bits;
  cl_ulong device_max_mem_alloc_size;
  cl_bool device_image_support;
  cl_uint device_max_read_image_args;
  cl_uint device_max_write_image_args;

  // Image handling info (for GPUs)
  size_t device_image2d_max_width;
  size_t device_image2d_max_height;
  size_t device_image3d_max_width;
  size_t device_image3d_max_height;
  size_t device_image3d_max_depth;

  cl_uint device_max_samplers;
  size_t device_max_parameter_size;
  cl_uint device_mem_base_addr_align;
  cl_uint device_min_data_type_align_size;
  cl_device_fp_config device_single_fp_config;
  cl_device_mem_cache_type device_global_mem_cache_type;
  cl_uint device_global_mem_cacheline_size;
  cl_ulong device_global_mem_cache_size;
  cl_ulong device_global_mem_size;
  cl_ulong device_max_constant_buffer_size;
  cl_uint device_max_constant_args;
  cl_device_local_mem_type device_local_mem_type;
  cl_ulong device_local_mem_size;
  cl_bool device_error_correction_support;
  size_t device_profiling_timer_resolution;
  cl_bool device_endian_little;
  cl_bool device_available;
  cl_bool device_compiler_available;
  cl_device_exec_capabilities device_execution_capabilities;
  cl_command_queue_properties device_queue_properties;

  // General device information
  char *device_name;
  char *device_vendor;
  char *driver_version;
  char *device_profile;
  char *device_version;
  char *device_extensions;

};


// Inferred from Table 4.4 of OpenCL Spec
struct _cl_context
{
  cl_uint ref_count;
  cl_uint device_count;
  cl_device_id *device_list;
  cl_context_properties *prop;
};


// Inferred from Table 5.2 of OpenCL Spec
struct _cl_command_queue
{
  cl_context queue_context;
  cl_device_id queue_device;
  cl_uint queue_ref_count;
  cl_command_queue_properties queue_properties;

  pthread_t *queue_thread;

  pthread_mutex_t *dataMutex;
  pthread_cond_t *dataPresentCondition;
  cl_bool stayAlive;

  event_list list;
  event_list last_element;
};


//Inferred from table 5.8 and 5.9 of OpenCL Spec
struct _cl_mem
{
  cl_mem_object_type mem_type;
  cl_mem_flags mem_flags;
  size_t mem_size;
  void *mem_host_ptr;
  cl_uint mem_map_count;
  cl_uint mem_ref_count;
  cl_context mem_context;

  //Image properties
  cl_image_format image_format;
  size_t image_element_size;
  size_t image_row_pitch;
  size_t image_slice_pitch;
  size_t image_width;
  size_t image_height;
  size_t image_depth;
};


//Inferred from table 5.10 of OpenCL Spec
struct _cl_sampler
{
  cl_uint sampler_reference_count;
  cl_context sampler_context;
  cl_addressing_mode sampler_addressing_mode;
  cl_filter_mode sampler_filter_mode;
  cl_bool sampler_normalized_coords;
};


//Inferred from table 5.11 and 5.12 of OpenCL Spec
struct _cl_program
{
  cl_uint program_ref_count;
  cl_context program_context;
  cl_uint program_num_devices;
  const cl_device_id *program_devices;
  char *program_source;

  // Actual binaries
  spe_program_handle_t *program_elfs;

  // Just names of binaries
  const size_t *program_binary_sizes;
  char **program_binaries;

  //build info
  cl_build_status program_build_status;
  char *program_build_options;
  char *program_buid_log;

};

//Inferred from table 5.13 of OpenCL Spec
struct _cl_kernel
{
  char *kernel_function_name;
  cl_uint kernel_num_args;
  cl_uint kernel_ref_count;
  cl_context kernel_context;
  cl_program kernel_program;

  cl_uint kernel_num_memobjs;
  cl_mem *kernel_memobjs;

  //SPE arguments
  cl_uint kernel_num_spe_args;
  cl_ulong *kernel_spe_args;

};



//Inferred from table 5.15 of OpenCL Spec
struct _cl_event
{
  cl_command_queue event_command_queue;
  cl_command_type event_command_type;
  cl_int event_command_execution_status;
  cl_uint event_reference_count;

  //Stored data for various event types
  cl_uint num_events_in_wait_list;
  cl_event *event_wait_list;

  //NDRangeKernel and Task
  cl_kernel kernel;
  cl_uint work_dim;
  const size_t *global_work_offset;
  const size_t *global_work_size;
  const size_t *local_work_size;

  //Read,Write,Copy buffer
  cl_mem src_buffer;
  cl_mem dst_buffer;
  cl_bool blocking;
  size_t src_offset;
  size_t dst_offset;
  size_t cb;
  void *ptr;

};




#endif // __CL_PS3_H
