#ifndef _OPENCL_SETUP_H
#define _OPENCL_SETUP_H

#define CL_TARGET_OPENCL_VERSION 200  /* OpenCL 2.0 */
#include <CL/cl.h>

#define MAX_NUM_PLATFORMS 32
#define MAX_NUM_DEVICES 32

#define CL_RO CL_MEM_READ_ONLY
#define CL_WO CL_MEM_WRITE_ONLY
#define CL_RW CL_MEM_READ_WRITE

/* Default build options for kernels. */
#define DEFAULT_BUILD_OPTIONS "-I. -ICL"

/* Enable USE_DES_BITSLICE to use the DES bitslice code from JohnTheRipper.  At this time, it somehow runs at half the speed of unoptimized DES on NVIDIA.  Anyone else want to look into what's going on? */
/*#define USE_DES_BITSLICE 1*/


void *rc_dlopen(char *library_name);
int rc_dlclose(void *module);
void *rc_dlsym(void *module, char *function_name);
char *rc_dlerror(void);

extern cl_int (*rc_clBuildProgram)(cl_program, cl_uint, const cl_device_id *, const char *, void (CL_CALLBACK *)(cl_program, void *), void *);
extern cl_mem (*rc_clCreateBuffer)(cl_context, cl_mem_flags, size_t, void *, cl_int *);
extern cl_context (*rc_clCreateContext)(cl_context_properties *, cl_uint, const cl_device_id *, void (CL_CALLBACK *)(const char *, const void *, size_t, void *), void *, cl_int *);
extern cl_command_queue (*rc_clCreateCommandQueueWithProperties)(cl_context, cl_device_id, const cl_queue_properties *, cl_int *);
extern cl_kernel (*rc_clCreateKernel)(cl_program, const char *, cl_int *);
extern cl_program (*rc_clCreateProgramWithSource)(cl_context, cl_uint, const char **, const size_t *, cl_int *);
extern cl_int (*rc_clEnqueueNDRangeKernel)(cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *);
extern cl_int (*rc_clEnqueueReadBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
extern cl_int (*rc_clEnqueueWriteBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
extern cl_int (*rc_clFinish)(cl_command_queue);
extern cl_int (*rc_clFlush)(cl_command_queue);
extern cl_int (*rc_clGetDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
extern cl_int (*rc_clGetDeviceInfo)(cl_device_id, cl_device_info, size_t, void *, size_t *);
extern cl_int (*rc_clGetKernelWorkGroupInfo)(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *);
extern cl_int (*rc_clGetPlatformIDs)(cl_uint, cl_platform_id *, cl_uint *);
extern cl_int (*rc_clGetPlatformInfo)(cl_platform_id, cl_platform_info, size_t, void *, size_t *);
extern cl_int (*rc_clGetProgramBuildInfo)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
extern cl_int (*rc_clReleaseCommandQueue)(cl_command_queue);
extern cl_int (*rc_clReleaseContext)(cl_context);
extern cl_int (*rc_clReleaseDevice)(cl_device_id);
extern cl_int (*rc_clReleaseKernel)(cl_kernel);
extern cl_int (*rc_clReleaseMemObject)(cl_mem);
extern cl_int (*rc_clReleaseProgram)(cl_program);
extern cl_int (*rc_clSetKernelArg)(cl_kernel, cl_uint, size_t, const void *);


#define CLMAKETESTVARS() \
  int err = 0; \
  size_t global_work_size = 1; \
  cl_command_queue queue = NULL;

#define _CLCREATEARG(_arg_index, _buffer, _flags, _arg_ptr, _arg_size)	\
  { _buffer = rc_clCreateBuffer(context, _flags, _arg_size, NULL, &err); \
  if (err < 0) { fprintf(stderr, "Error while creating buffer for \"%s\". Error code: %d\n", #_arg_ptr, err); exit(-1); } \
  err = rc_clEnqueueWriteBuffer(queue, _buffer, CL_TRUE, 0, _arg_size, _arg_ptr, 0, NULL, NULL); \
  if (err < 0) { fprintf(stderr, "Error while writing to buffer for \"%s\". Error code: %d\n", #_arg_ptr, err); exit(-1); } \
  err = rc_clSetKernelArg(kernel, _arg_index, sizeof(cl_mem), &_buffer); \
  if (err < 0) { fprintf(stderr, "Error setting kernel argument for %s at index %u.\n", #_arg_ptr, _arg_index); exit(-1); } }

#define CLCREATEARG_ARRAY(_arg_index, _buffer, _flags, _arg, _len) \
  _CLCREATEARG(_arg_index, _buffer, _flags, _arg, _len);

#define CLCREATEARG(_arg_index, _buffer, _flags, _arg, _arg_size) \
  _CLCREATEARG(_arg_index, _buffer, _flags, &_arg, _arg_size);

#define CLCREATEARG_DEBUG(_arg_index, _debug_buffer, _debug_ptr) \
  { _debug_ptr = calloc(DEBUG_LEN, sizeof(unsigned char)); \
    CLCREATEARG(_arg_index, _debug_buffer, CL_MEM_READ_WRITE, _debug_ptr, DEBUG_LEN); }

#define CLCREATECONTEXT(_context_callback, _device_ptr)	\
  rc_clCreateContext(NULL, 1, _device_ptr, _context_callback, NULL, &err); if (err < 0) { fprintf(stderr, "Failed to create context: %d\n", err); exit(-1); }

#define CLCREATEQUEUE(_context, _device) \
  rc_clCreateCommandQueueWithProperties(_context, _device, NULL, &err); if (err < 0) { fprintf(stderr, "clCreateCommandQueueWithProperties failed: %d\n", err); exit(-1); }

#define CLRUNKERNEL(_queue, _kernel, _gws_ptr) \
  { err = rc_clEnqueueNDRangeKernel(_queue, _kernel, 1, NULL, _gws_ptr, NULL, 0, NULL, NULL); if (err < 0) { fprintf(stderr, "clEnqueueNDRangeKernel failed: %d\n", err);  exit(-1); } }

#define CLFLUSH(_queue) \
  { err = rc_clFlush(_queue); if (err < 0) { fprintf(stderr, "clFlush failed: %d\n", err); exit(-1); } }

#define CLWAIT(_queue) \
  { err = rc_clFinish(_queue); if (err == CL_INVALID_COMMAND_QUEUE) { fprintf(stderr, "\nError: clFinish() returned CL_INVALID_COMMAND_QUEUE (%d).  This is often caused by running out of host memory.  Sometimes, it can be worked around by lowering the GWS setting (see command line options; hint: try setting it to a multiple of the max compute units reported at the beginning of the program output.  For example, if the MCU is 15, try setting the GWS parameter to 15 * 256 = 3840, 15 * 1024 = 15360, etc).\n", err); exit(-1); } else if (err < 0) { fprintf(stderr, "clFinish failed: %d\n", err); exit(-1); } }

#define CLREADBUFFER(_buffer, _len, _ptr) \
  { err = rc_clEnqueueReadBuffer(queue, _buffer, CL_TRUE, 0, _len, _ptr, 0, NULL, NULL); if (err < 0) { fprintf(stderr, "clEnqueueReadBuffer failed: %d\n", err); exit(-1); } }

#define CLFREEBUFFER(_buffer) \
  if (_buffer != NULL) { rc_clReleaseMemObject(_buffer); _buffer = NULL; }

#define CLRELEASEQUEUE(_queue) \
  if (_queue != NULL) { rc_clReleaseCommandQueue(_queue); _queue = NULL; }

#define CLRELEASECONTEXT(_context) \
  if (_context != NULL) { rc_clReleaseContext(_context); _context = NULL; }

#define CLRELEASEKERNEL(_kernel) \
  if (_kernel != NULL) { rc_clReleaseKernel(_kernel); _kernel = NULL; }

#define CLRELEASEPROGRAM(_program) \
  if (_program != NULL) { rc_clReleaseProgram(_program); _program = NULL; }

#define LOADFUNC(_ocl, _func_name) \
  { rc_##_func_name = rc_dlsym(_ocl, #_func_name);			\
    if (rc_##_func_name == NULL) { fprintf(stderr, "Error while loading function %s: %s\n", #_func_name, rc_dlerror()); exit(-1); } }


void context_callback(const char *errinfo, const void *private_info, size_t cb, void *user_data);
void get_device_bool(cl_device_id device, cl_device_info param, cl_bool *b);
void get_platforms_and_devices(int disable_platform, cl_uint platforms_buffer_size, cl_platform_id *platforms, cl_uint *num_platforms, cl_uint devices_buffer_size, cl_device_id *devices, cl_uint *num_devices, unsigned int verbose);
void get_device_str(cl_device_id device, cl_device_info param, char *buf, int buf_len);
void get_device_uint(cl_device_id device, cl_device_info param, cl_uint *u);
void get_device_ulong(cl_device_id device, cl_device_info param, cl_ulong *ul);
void get_platform_str(cl_platform_id device, cl_platform_info param, char *buf, size_t buf_len);
void load_kernel(cl_context context, cl_uint num_devices, const cl_device_id *devices, const char *path, const char *kernel_name, cl_program *program, cl_kernel *kernel, unsigned int hash_type);
void print_device_info(cl_device_id *devices, cl_uint num_devices);
void print_platform_info(cl_platform_id *platforms, cl_uint num_platforms);

#endif
