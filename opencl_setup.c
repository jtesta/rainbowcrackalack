/*
 * Rainbow Crackalack: opencl_setup.c
 * Copyright (C) 2018-2019  Joe Testa <jtesta@positronsecurity.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>


#ifdef _WIN32
#include <windows.h>
static char dlerror_buffer[256] = {0};
static HMODULE ocl = NULL; /* TODO: release on program exit. */
#else
#include <dlfcn.h>
static void *ocl = NULL;   /* TODO: release on program exit. */
#endif

#include "misc.h"
#include "opencl_setup.h"


/* Toggled when the OpenCL library is loaded and initialized. */
static unsigned int opencl_initialized = 0;


/* Pointers to OpenCL functions. */
cl_int (*rc_clBuildProgram)(cl_program, cl_uint, const cl_device_id *, const char *, void (CL_CALLBACK *)(cl_program, void *), void *) = NULL;
cl_mem (*rc_clCreateBuffer)(cl_context, cl_mem_flags, size_t, void *, cl_int *) = NULL;
cl_context (*rc_clCreateContext)(cl_context_properties *, cl_uint, const cl_device_id *, void (CL_CALLBACK *)(const char *, const void *, size_t, void *), void *, cl_int *) = NULL;
cl_command_queue (*rc_clCreateCommandQueueWithProperties)(cl_context, cl_device_id, const cl_queue_properties *, cl_int *) = NULL;
cl_kernel (*rc_clCreateKernel)(cl_program, const char *, cl_int *) = NULL;
cl_program (*rc_clCreateProgramWithSource)(cl_context, cl_uint, const char **, const size_t *, cl_int *) = NULL;
cl_int (*rc_clEnqueueNDRangeKernel)(cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *) = NULL;
cl_int (*rc_clEnqueueReadBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *) = NULL;
cl_int (*rc_clEnqueueWriteBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *) = NULL;
cl_int (*rc_clFinish)(cl_command_queue) = NULL;
cl_int (*rc_clFlush)(cl_command_queue) = NULL;
cl_int (*rc_clGetDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *) = NULL;
cl_int (*rc_clGetDeviceInfo)(cl_device_id, cl_device_info, size_t, void *, size_t *) = NULL;
cl_int (*rc_clGetKernelWorkGroupInfo)(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *) = NULL;
cl_int (*rc_clGetPlatformIDs)(cl_uint, cl_platform_id *, cl_uint *) = NULL;
cl_int (*rc_clGetProgramBuildInfo)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *) = NULL;
cl_int (*rc_clReleaseCommandQueue)(cl_command_queue) = NULL;
cl_int (*rc_clReleaseContext)(cl_context) = NULL;
cl_int (*rc_clReleaseDevice)(cl_device_id) = NULL;
cl_int (*rc_clReleaseKernel)(cl_kernel) = NULL;
cl_int (*rc_clReleaseMemObject)(cl_mem) = NULL;
cl_int (*rc_clReleaseProgram)(cl_program) = NULL;
cl_int (*rc_clSetKernelArg)(cl_kernel, cl_uint, size_t, const void *) = NULL;


#ifdef _WIN32

void *rc_dlopen(char *library_name) {
  return LoadLibraryA(library_name);
}

int rc_dlclose(void *module) {
  return FreeLibrary(module);
}

void *rc_dlsym(void *module, char *function_name) {
  return GetProcAddress(module, function_name);
}

char *rc_dlerror(void) {
  dlerror_buffer[0] = '\0';
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		dlerror_buffer, 0, NULL);
  return dlerror_buffer;
}

#else

void *rc_dlopen(char *library_name) {
  return dlopen(library_name, RTLD_NOW);
}

int rc_dlclose(void *module) {
  return dlclose(module);
}

void *rc_dlsym(void *module, char *function_name) {
  return dlsym(module, function_name);
}

char * rc_dlerror(void) {
  return dlerror();
}

#endif


void context_callback(const char *errinfo, const void *private_info, size_t cb, void *user_data) {
  printf("\n\n\tError callback invoked!\n\n\terrinfo: %s\n\n", errinfo);
  return;
}


void get_device_bool(cl_device_id device, cl_device_info param, cl_bool *b) {
  if (rc_clGetDeviceInfo(device, param, sizeof(cl_bool), b, NULL) < 0) {
    fprintf(stderr, "Error while getting device info.\n");
    exit(-1);
  }
}


void get_device_str(cl_device_id device, cl_device_info param, char *buf, int buf_len) {
  if (rc_clGetDeviceInfo(device, param, buf_len, buf, NULL) < 0) {
    fprintf(stderr, "Error while getting device info.\n");
    exit(-1);
  }
}


void get_device_uint(cl_device_id device, cl_device_info param, cl_uint *u) {
  if (rc_clGetDeviceInfo(device, param, sizeof(cl_uint), u, NULL) < 0) {
    fprintf(stderr, "Error while getting device info.\n");
    exit(-1);
  }
}


void get_device_ulong(cl_device_id device, cl_device_info param, cl_ulong *ul) {
  if (rc_clGetDeviceInfo(device, param, sizeof(cl_ulong), ul, NULL) < 0) {
    fprintf(stderr, "Error while getting device info.\n");
    exit(-1);
  }
}


/* Returns the array of platforms and devices. */
void get_platforms_and_devices(cl_uint platforms_buffer_size, cl_platform_id *platforms, cl_uint *num_platforms, cl_uint devices_buffer_size, cl_device_id *devices, cl_uint *num_devices, unsigned int verbose) {
  unsigned int i = 0;
  cl_int err = 0;
  cl_uint n = 0;
  cl_device_type device_type = CL_DEVICE_TYPE_GPU;


  *num_platforms = 0;
  *num_devices = 0;

  if (opencl_initialized == 0) {
#ifdef _WIN32
    ocl = rc_dlopen("OpenCL"); /* Windows */
#else
    ocl = rc_dlopen("libOpenCL.so"); /* Linux */
    if (ocl == NULL) /* See if the Intel OpenCL driver is available... */
      ocl = rc_dlopen("libOpenCL.so.1");
#endif
    if (ocl == NULL) {
      fprintf(stderr, "\nFailed to open OpenCL library.  Are the GPU drivers properly installed?\nError: %s\n\n", rc_dlerror());
      exit(-1);
    }
    rc_dlerror();  /* Clear error codes. */

    LOADFUNC(ocl, clBuildProgram);
    LOADFUNC(ocl, clCreateBuffer);
    LOADFUNC(ocl, clCreateCommandQueueWithProperties);
    LOADFUNC(ocl, clCreateContext);
    LOADFUNC(ocl, clCreateKernel);
    LOADFUNC(ocl, clCreateProgramWithSource);
    LOADFUNC(ocl, clEnqueueNDRangeKernel);
    LOADFUNC(ocl, clEnqueueReadBuffer);
    LOADFUNC(ocl, clEnqueueWriteBuffer);
    LOADFUNC(ocl, clFinish);
    LOADFUNC(ocl, clFlush);
    LOADFUNC(ocl, clGetDeviceIDs);
    LOADFUNC(ocl, clGetDeviceInfo);
    LOADFUNC(ocl, clGetKernelWorkGroupInfo);
    LOADFUNC(ocl, clGetPlatformIDs);
    LOADFUNC(ocl, clGetProgramBuildInfo);
    LOADFUNC(ocl, clReleaseCommandQueue);
    LOADFUNC(ocl, clReleaseContext);
    LOADFUNC(ocl, clReleaseDevice);
    LOADFUNC(ocl, clReleaseKernel);
    LOADFUNC(ocl, clReleaseMemObject);
    LOADFUNC(ocl, clReleaseProgram);
    LOADFUNC(ocl, clSetKernelArg);

    opencl_initialized = 1;
  }

  if (verbose)
    printf("Operating system: %s\n", get_os_name());

  if (rc_clGetPlatformIDs(platforms_buffer_size, platforms, num_platforms) < 0) {
    fprintf(stderr, "Failed to get platform IDs.  Are the OpenCL drivers installed?\n");
    exit(-1);
  } else if (*num_platforms < 1) {
    fprintf(stderr, "Number of platforms is < 1!\n");
    exit(-1);
  }
  if (verbose)
    printf("Found %u platforms.\n", *num_platforms);

#ifdef TRAVIS_BUILD
  device_type = CL_DEVICE_TYPE_CPU;
  printf("\n\n\t!!! WARNING !!!\n\nThis is a Travis build; it is only intended for developer testing.  If you're not seeing this message in a Travis build output, then something is very, very wrong...\n\n");
  fflush(stdout);
#endif

  for (i = 0; ((i < *num_platforms) && (*num_devices < devices_buffer_size)); i++) {
    err = rc_clGetDeviceIDs(platforms[i], device_type, devices_buffer_size - *num_devices, &(devices[*num_devices]), &n);
    if (err == CL_DEVICE_NOT_FOUND)
      printf("No GPUs found on platform #%u\n", i);
    else if (err < 0)
      fprintf(stderr, "Error while getting device IDs on platform #%u; clGetDeviceIDs() returned: %d.\n", i, err);
    else if (n < 1)
      fprintf(stderr, "Platform #%u has < 1 devices!\n", i);
    else {
      *num_devices += n;
      if (verbose)
	printf("Found %u devices on platform #%u.\n", *num_devices, i);
    }
  }

  if (verbose)
    print_device_info(devices, *num_devices);

  {  /* Workaround for Intel + AMD/NVIDIA setups. */
    char device_vendor[128] = {0};
    unsigned int intel_found = 0, amd_or_nvidia_found = 0;
    unsigned int intel_slot = 0;

    for (i = 0; i < *num_devices; i++) {
      get_device_str(devices[i], CL_DEVICE_VENDOR, device_vendor, sizeof(device_vendor) - 1);

      if (strstr(device_vendor, "Intel") != NULL) {
        intel_found = 1;
        intel_slot = i;
      } else if (strstr(device_vendor, "NVIDIA") != NULL)
        amd_or_nvidia_found = 1;
      else if (strstr(device_vendor, "Advanced Micro Devices") != NULL)
        amd_or_nvidia_found = 1;
    }

    /* If a mixed system was found, overwrite the Intel device ID in the array and decrement the number of devices. */
    if (intel_found && amd_or_nvidia_found) {
      printf("\nFound system with Intel + NVIDIA/AMD mix.  Disabling Intel GPU at %u.\n", intel_slot);
      for (i = intel_slot; i < *num_devices - 1; i++)
        devices[i] = devices[i + 1];

      *num_devices = *num_devices - 1;
    }
  }

  return;
}


/* Loads a kernel onto a device. */
void load_kernel(cl_context context, cl_uint num_devices, const cl_device_id *devices, const char *source_filename, const char *kernel_name, cl_program *program, cl_kernel *kernel, unsigned int hash_type) {
  FILE *f = NULL;
  char *source = NULL;
  int file_size = 0, bytes_read = 0, n = 0;
  int err = 0;
  char build_options[512] = {0};
  char device_vendor[128] = {0};
  char path[256] = {0};


  filepath_join(path, sizeof(path), "CL", source_filename);
  f = fopen(path, "r");
  if (f == NULL) {
    perror("Failed to open kernel.");
    exit(-1);
  }

  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  rewind(f);

  if (file_size < 1) {
    fprintf(stderr, "File size of kernel is invalid.\n");
    exit(-1);
  }

  source = calloc(file_size + 1, sizeof(char));
  if (source == NULL) {
    fprintf(stderr, "Failed to allocate file buffer.\n");
    exit(-1);
  }

  while (bytes_read < file_size) {
    n = fread(source + bytes_read, sizeof(char), file_size - bytes_read, f);
    if (n <= 0) {
      fprintf(stderr, "Error while reading kernel.\n");
      exit(-1);
    }

    bytes_read += n;
  }

  FCLOSE(f);

  *program = rc_clCreateProgramWithSource(context, 1, (const char **)&source, NULL, &err);
  if (err < 0) {
    fprintf(stderr, "clCreateProgramWithSource failed.\n");
    exit(-1);
  }

  snprintf(build_options, sizeof(build_options) - 1, "%s -DHASH_TYPE=%u", DEFAULT_BUILD_OPTIONS, hash_type);
#ifdef USE_DES_BITSLICE
  strncat(build_options, " -DUSE_DES_BITSLICE=1", sizeof(build_options) - 1);
#endif

  get_device_str(devices[0], CL_DEVICE_VENDOR, device_vendor, sizeof(device_vendor) - 1);

#ifdef _WIN32
  /* If we're on Windows, and the vendor is AMD, assume its the Catalyst driver. */
  if (strcmp(device_vendor, "Advanced Micro Devices, Inc.") == 0)
    strncat(build_options, " -DAMD_CATALYST=1", sizeof(build_options) - 1);
#else
  /* If we're not on Windows, and the vendor is AMD, assume its the ROCm driver. */
  if (strcmp(device_vendor, "Advanced Micro Devices, Inc.") == 0)
    strncat(build_options, " -DAMD_ROCM=1", sizeof(build_options) - 1);
#endif

  /*printf("Building program with options: %s\n", build_options);*/
  if (rc_clBuildProgram(*program, num_devices, devices, build_options, NULL, NULL) < 0) {
    size_t log_size = 0;
    char *error_str = NULL;

    fprintf(stderr, "clBuildProgram failed.\n");
    rc_clGetProgramBuildInfo(*program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    error_str = calloc(log_size + 1, sizeof(char));
    rc_clGetProgramBuildInfo(*program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, error_str, NULL);
    fprintf(stderr, "%s\n", error_str);
    FREE(error_str);
    exit(-1);
  }

  *kernel = rc_clCreateKernel(*program, kernel_name, &err);
  if (err < 0) {
    fprintf(stderr, "clCreateKernel failed.\n");
    exit(-1);
  }

  FREE(source);
  return;
}


/* Prints debugging information about devices.  Returns 0 - num_devices-1 if an Intel + AMD/NVIDIA setup is found; this signifies the index in devices[] where the Intel GPU is.  Otherwise returns -1. */
void print_device_info(cl_device_id *devices, cl_uint num_devices) {
  int i = 0;
  char device_name[64] = {0};
  char device_version[64] = {0};
  char device_vendor[128] = {0};
  char device_driver[128] = {0};
  cl_bool b = 0;
  cl_uint max_compute_units = 0;
  cl_ulong global_memsize = 0;
  cl_ulong max_work_group_size = 0;


  for (i = 0; i < num_devices; i++) {
    get_device_str(devices[i], CL_DEVICE_NAME, device_name, sizeof(device_name) - 1);
    get_device_str(devices[i], CL_DEVICE_VERSION, device_version, sizeof(device_version) - 1);
    get_device_str(devices[i], CL_DEVICE_VENDOR, device_vendor, sizeof(device_vendor) - 1);
    get_device_bool(devices[i], CL_DEVICE_AVAILABLE, &b);
    get_device_uint(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS, &max_compute_units);
    get_device_ulong(devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, &global_memsize);
    get_device_ulong(devices[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, &max_work_group_size);
    get_device_str(devices[i], CL_DRIVER_VERSION, device_driver, sizeof(device_driver) - 1);

    printf("Device #%d:\n", i);
    printf("\tVendor: %s\n", device_vendor);
    printf("\tName: %s\n", device_name);
    printf("\tVersion: %s\n", device_version);
    printf("\tDriver: %s\n", device_driver);
    printf("\tMax compute units: %u\n", max_compute_units);
    printf("\tMax work group size: %"PRIu64"\n", max_work_group_size);
    printf("\tGlobal memory size: %"PRIu64"\n", global_memsize);
    if (b == 0)
      printf("\t---> NOT AVAILABLE!\n");
    printf("\n");
  }
  fflush(stdout);
}
