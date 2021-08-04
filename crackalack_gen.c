/*
 * Rainbow Crackalack: crackalack_gen.c
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

#ifdef _WIN32
#include <windows.h>
#endif

#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "opencl_setup.h"

#include "charset.h"
#include "clock.h"
#include "cpu_rt_functions.h"
#include "file_lock.h"
#include "gws.h"
#include "hash_validate.h"
#include "misc.h"
#include "shared.h"
#include "terminal_color.h"
#include "verify.h"
#include "version.h"


#define CRACKALACK_KERNEL_PATH "crackalack.cl"
#define CRACKALACK_NTLM8_KERNEL_PATH "crackalack_ntlm8.cl"
#define CRACKALACK_NTLM9_KERNEL_PATH "crackalack_ntlm9.cl"

#define VERBOSE 1

#define UNDEFINED_INDEX 999

/* The initial number of chains each work unit should compute.  This scales up and down
 * at run-time based on the speed of execution (actually, this was disabled, because
 * the Windows drivers don't like it...). */
#define INITIAL_CHAINS_PER_EXECUTION 1

/* The interval, in seconds, that the user should be updated on the generation
 * progress. */
#define UPDATE_INTERVAL (1 * 60)  /* 1 minute */


#define LOCK_START_INDEX() \
  if (pthread_mutex_lock(&start_index_mutex)) { perror("Failed to lock mutex"); exit(-1); }

#define UNLOCK_START_INDEX() \
  if (pthread_mutex_unlock(&start_index_mutex)) { perror("Failed to unlock mutex"); exit(-1); }

#define ROUND(_x) ((unsigned int)(_x + 0.5))

void write_chains(char *filename, unsigned int chains_per_work_unit, cl_ulong *start_indices, unsigned int start_indices_size, cl_ulong *end_indices, unsigned int end_indices_size, unsigned int thread_id);


struct hash_names {
  char name[8];
  unsigned int type;
};
struct hash_names valid_hash_names[] = {
  {"lm", HASH_LM},
  /*{"ntlm", HASH_NTLM}*/
};


/* Struct to represent one GPU device. */
typedef struct {
  cl_uint device_number;
  cl_device_id device;
  cl_context context;
  cl_program program;
  cl_kernel kernel;
  cl_command_queue queue;
  cl_uint num_work_units;
} gpu_dev;

/* Struct to pass arguments to a host thread. */
typedef struct {
  unsigned int benchmark_mode;

  unsigned int hash_type;
  char *charset;
  unsigned int plaintext_len_min;
  unsigned int plaintext_len_max;
  unsigned int table_index;
  unsigned int reduction_offset;
  unsigned int chain_len;
  char *filename;

  unsigned int initial_chains_per_execution;

  gpu_dev gpu;
} thread_args;


/* Mutex to protect access to the start_index counter between threads. */
pthread_mutex_t start_index_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Barrier to ensure that kernels on multiple devices are all run at the same time.
 * The closed-source AMD driver on Windows effectively blocks other devices while
 * one kernel is running; this ensures parallelization in that environment, since
 * all kernels will run at once.  The open source AMD ROCm driver on Linux may or
 * may not get a very slight performance bump with this enabled. */
pthread_barrier_t barrier = {0};

/* The start index to use for the next chain to generate.  This is shared among the
 * host threads. */
uint64_t start_index = 0;

/* The number of chains to generate.  This doesn't necessarily equal the number of
 * chains entered on the command line, since we may be resuming a
 * partially-constructed table. */
unsigned int num_chains_to_generate = 0;

/* The first chain that we will generate (this will not be zero if we resume an
 * unfinished file or part index > 0).  We use this to track how many chains we
 * generated so far. */
uint64_t first_generated_chain = 0;

/* The time that the threads were started. */
struct timespec global_start_time = {0};

/* The last time that the generation rate was output to stdout. */
struct timespec last_update_time = {0};

/* Set to 1 if AMD GPUs found. */
unsigned int is_amd_gpu = 0;

/* The global work size, as over-ridden by the user on the command line. */
size_t user_provided_gws = 0;


void print_usage_and_exit(char *prog_name, int exit_code) {
  fprintf(stderr, "Usage: %s hash_algorithm charset_name plaintext_min_length plaintext_max_length table_index chain_length number_of_chains [part_index | -bench] [-gws GWS]\n\nExample: %s ntlm ascii-32-95 9 9 0 803000 67108864 0\n\n", prog_name, prog_name);
  exit(exit_code);
}


/* Outputs the number of chains created so far to stdout, along with the rate.
 * Optionally, an estimate of how much time is remaining is also given. */
void output_progress(unsigned int calculate_time_remaining) {

  uint64_t num_chains_generated = 0;
  double run_time = 0.0, rate = 0.0;
  char time_str[128];

  memset(time_str, 0, sizeof(time_str));


  LOCK_START_INDEX();
  num_chains_generated = start_index - first_generated_chain;
  UNLOCK_START_INDEX();

  start_timer(&last_update_time);
  run_time = get_elapsed(&global_start_time);
  if (run_time == 0.0)
    return;

  rate = num_chains_generated / run_time;
  seconds_to_human_time(time_str, sizeof(time_str), run_time);

#ifdef _WIN32
  printf("Run time: %s; Chains generated: %"PRIu64"; Rate: %s%u/s%s\n", time_str, num_chains_generated, WHITEB, (unsigned int)rate, CLR);
#else
  printf("Run time: %s; Chains generated: %'"PRIu64"; Rate: %s%'u/s%s\n", time_str, num_chains_generated, WHITEB, (unsigned int)rate, CLR);
#endif

  if (calculate_time_remaining && (rate > 0.0)) {
    seconds_to_human_time(time_str, sizeof(time_str), (num_chains_to_generate - num_chains_generated) / rate);
    printf("Estimated time remaining: %s\n", time_str);
  }
  fflush(stdout);
}


/* A host thread which controls each GPU. */
void *host_thread(void *ptr) {
  thread_args *args = (thread_args *)ptr;
  gpu_dev *gpu = &(args->gpu);

  char *kernel_path = CRACKALACK_KERNEL_PATH, *kernel_name = "crackalack";
  size_t gws = 0, kernel_work_group_size = 0, kernel_preferred_work_group_size_multiple = 0;
  uint64_t *start_indices = NULL, *end_indices = NULL;
  unsigned int i = 0, indices_size = 0, thread_complete = 0, num_passes = 0, pass = 0, chain_len = 0;
  /*time_t thread_start_time = 0;
  double elapsed = 0;*/
  int err = 0;

  cl_context context = NULL;
  cl_command_queue queue = NULL;
  cl_kernel kernel = NULL;

  cl_mem hash_type_buffer = NULL, charset_buffer = NULL, plaintext_len_min_buffer = NULL, plaintext_len_max_buffer = NULL, reduction_offset_buffer = NULL, chain_len_buffer = NULL, indices_buffer = NULL, pos_start_buffer = NULL;

  cl_uint pos_start = 0;


  /* If we're generating the standard NTLM 8- or 9-character tables, use the special
   * optimized kernel instead! */
  if (is_ntlm8(args->hash_type, args->charset, args->plaintext_len_min, args->plaintext_len_max, args->reduction_offset, args->chain_len)) {
    kernel_path = CRACKALACK_NTLM8_KERNEL_PATH;
    kernel_name = "crackalack_ntlm8";
    if (args->gpu.device_number == 0) { /* Only the first thread prints this. */
      printf("%sNote: optimized NTLM8 kernel will be used.%s\n", GREENB, CLR); fflush(stdout);
    }
  } else if (is_ntlm9(args->hash_type, args->charset, args->plaintext_len_min, args->plaintext_len_max, args->reduction_offset, args->chain_len)) {
    kernel_path = CRACKALACK_NTLM9_KERNEL_PATH;
    kernel_name = "crackalack_ntlm9";
    if (args->gpu.device_number == 0) { /* Only the first thread prints this. */
      printf("%sNote: optimized NTLM9 kernel will be used.%s\n", GREENB, CLR); fflush(stdout);
    }
  } else {
    printf("%sWARNING: non-optimized kernel will be used since non-standard options were given!  Generation will be much slower.  (Hint: use \"crackalack_gen ntlm ascii-32-95 8 8 0 422000 67108864 X\" for optimized NTLM8 generation, or \"crackalack_gen ntlm ascii-32-95 9 9 0 803000 67108864 X\" for optimized NTLM9 generation.)%s\n", YELLOWB, CLR); fflush(stdout);
  }

  /* Get the number of compute units in this device. */
  get_device_uint(gpu->device, CL_DEVICE_MAX_COMPUTE_UNITS, &(gpu->num_work_units));

  /* Load the kernel. */
  gpu->context = CLCREATECONTEXT(context_callback, &(gpu->device));
  gpu->queue = CLCREATEQUEUE(gpu->context, gpu->device);
  load_kernel(gpu->context, 1, &(gpu->device), kernel_path, kernel_name, &(gpu->program), &(gpu->kernel), args->hash_type);

  context = gpu->context;
  queue = gpu->queue;
  kernel = gpu->kernel;

  if ((rc_clGetKernelWorkGroupInfo(kernel, gpu->device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &kernel_work_group_size, NULL) != CL_SUCCESS) || \
      (rc_clGetKernelWorkGroupInfo(kernel, gpu->device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &kernel_preferred_work_group_size_multiple, NULL) != CL_SUCCESS)) {
    fprintf(stderr, "Failed to get preferred work group size!\n");
    CLRELEASEKERNEL(gpu->kernel);
    CLRELEASEPROGRAM(gpu->program);
    CLRELEASEQUEUE(gpu->queue);
    CLRELEASECONTEXT(gpu->context);
    pthread_exit(NULL);
    return NULL;
  }

  /* If the user provided a static GWS on the command line, use that.   Otherwise,
   * use the driver's work group size multiplied by the preferred multiple. */
  if (user_provided_gws > 0) {
    gws = user_provided_gws;
    printf("GPU #%u is using user-provided GWS value of %"PRIu64"\n", gpu->device_number, gws);
  } else if (get_optimal_gws(gpu->device) > 0) {
    gws = get_optimal_gws(gpu->device);
    printf("GPU #%u is using optimized GWS: %"PRIu64"\n", gpu->device_number, gws);
  } else {
    gws = kernel_work_group_size * kernel_preferred_work_group_size_multiple;
    printf("GPU #%u is using dynamic GWS: %"PRIu64" (work group) x %"PRIu64" (pref. multiple) = %"PRIu64"\n", gpu->device_number, kernel_work_group_size, kernel_preferred_work_group_size_multiple, gws);
  }
  fflush(stdout);

  /* AMD on Windows will hang if the number of chains to generate is less than the
   * GWS.  The open-source ROCm driver under Linux works fine, though. */
#if _WIN32
  if ((is_amd_gpu) && (gws >= num_chains_to_generate)) {
    printf("\n\n  !! WARNING !!\n\nThe GWS (global work size) is greater or equal to the number of chains to generate (%"PRId64" >= %u).  The closed-source AMD Windows driver has been observed to hang indefinitely in this case.  If this happens, either raise the number of chains to generate, or lower the GWS setting using the '-gws' parameter.\n\n", gws, num_chains_to_generate);  fflush(stdout);
  }
#endif

  indices_size = gws;
  start_indices = calloc(indices_size, sizeof(cl_ulong));
  end_indices = calloc(indices_size, sizeof(cl_ulong));
  if ((start_indices == NULL) || (end_indices == NULL)) {
    fprintf(stderr, "Failed to create start/end index buffers.\n");
    exit(-1);
  }


  num_passes = 1;
  if (args->chain_len > MAX_CHAIN_LEN) {
    num_passes = args->chain_len / MAX_CHAIN_LEN;
    if ((args->chain_len % MAX_CHAIN_LEN) > 0)
      num_passes++;
  }

  while(1) {
    LOCK_START_INDEX();

    /* Check if all chains were already created.  If so, release the mutex and
     * terminate the thread. */
    if ((start_index - first_generated_chain) >= num_chains_to_generate) {
      /*printf("Thread #%u complete!: %lu %u\n", gpu->device_number, start_index, num_chains);*/
      UNLOCK_START_INDEX();
      thread_complete = 1;
    } else {
      for (i = 0; i < indices_size; i++) {
	start_indices[i] = start_index;
	start_index++;
      }
      UNLOCK_START_INDEX();
    }

    /* All chains generated, so terminate the thread. */
    if (thread_complete) {
      CLFREEBUFFER(hash_type_buffer);
      CLFREEBUFFER(charset_buffer);
      CLFREEBUFFER(plaintext_len_min_buffer);
      CLFREEBUFFER(plaintext_len_max_buffer);
      CLFREEBUFFER(reduction_offset_buffer);
      CLFREEBUFFER(chain_len_buffer);
      CLFREEBUFFER(indices_buffer);
      CLFREEBUFFER(pos_start_buffer);

      CLRELEASEKERNEL(gpu->kernel);
      CLRELEASEPROGRAM(gpu->program);
      CLRELEASEQUEUE(gpu->queue);
      CLRELEASECONTEXT(gpu->context);

      FREE(start_indices);
      FREE(end_indices);
      pthread_exit(NULL);
      return NULL;
    }

    /* Most of the parameters need only be set once upon first invokation. */
    if (hash_type_buffer == NULL) {
      CLCREATEARG(0, hash_type_buffer, CL_RO, args->hash_type, sizeof(cl_uint));
      CLCREATEARG_ARRAY(1, charset_buffer, CL_RO, args->charset, strlen(args->charset) + 1);
      CLCREATEARG(2, plaintext_len_min_buffer, CL_RO, args->plaintext_len_min, sizeof(cl_uint));
      CLCREATEARG(3, plaintext_len_max_buffer, CL_RO, args->plaintext_len_max, sizeof(cl_uint));
      CLCREATEARG(4, reduction_offset_buffer, CL_RO, args->reduction_offset, sizeof(cl_uint));
    }

    /* The start_indices parameter must be set each block.  The start indices are loaded into this read/write buffer, and the end indices will be in it when finished. */
    CLCREATEARG_ARRAY(6, indices_buffer, CL_RW, start_indices, indices_size * sizeof(cl_ulong));

    /* If the chain length is greater than MAX_CHAIN_LEN, then the chains must be computed in multiple passes (otherwise Windows drivers crash). */
    for (pass = 0; pass < num_passes; pass++) {
      chain_len = args->chain_len;

      /* If we're doing multiple passes, and aren't handling the last pass, set the chain length to a multiple of MAX_CHAIN_LEN.  We add one at the end because the GPU code stops one short of the chain length. */
      if ((num_passes > 1) && (pass != (num_passes - 1)))
	chain_len = ((pass + 1) * MAX_CHAIN_LEN) + 1;

      /* Starting at 0, the position start increases by a multiple of MAX_CHAIN_LEN. */
      pos_start = pass * MAX_CHAIN_LEN;

      /*printf("Pass #%u: pos_start: %u; chain_len: %u\n", pass, pos_start, chain_len);*/
      CLCREATEARG(5, chain_len_buffer, CL_RO, chain_len, sizeof(cl_uint));
      CLCREATEARG(7, pos_start_buffer, CL_RO, pos_start, sizeof(cl_uint));

      /* For AMD GPUs, ensure that all kernels are running concurrently.  This is a
       * requirement for the closed-source Windows driver, and may or may not be
       * very slightly helpful under the open-source ROCm Linux driver. */
#ifdef _WIN32
      if (is_amd_gpu) {
	int barrier_ret = pthread_barrier_wait(&barrier);
	if ((barrier_ret != 0) && (barrier_ret != PTHREAD_BARRIER_SERIAL_THREAD)) {
	  fprintf(stderr, "pthread_barrier_wait() failed!\n"); fflush(stderr);
	  exit(-1);
	}
      }
#endif

      /* Run the kernel, wait for it to finish, and calculate its run time. */
      /*thread_start_time = time(NULL);*/
      CLRUNKERNEL(gpu->queue, gpu->kernel, &gws);
      CLFLUSH(gpu->queue);
      CLWAIT(gpu->queue);
      /*elapsed = difftime(time(NULL), thread_start_time);*/

      CLFREEBUFFER(chain_len_buffer);
      CLFREEBUFFER(pos_start_buffer);
    }

    /* Get the kernel output. */
    CLREADBUFFER(indices_buffer, indices_size * sizeof(cl_ulong), end_indices);
    CLFREEBUFFER(indices_buffer);

    /* If we are in benchmark mode, don't loop again, nor write to the output file. */
    if (args->benchmark_mode)
      thread_complete = 1;
    else {

      /* Write the chains to the file.  */
      write_chains(args->filename, 1, start_indices, indices_size, end_indices, indices_size, gpu->device_number);

      /* Thread #0 outputs the generation progress periodically. */
      if ((args->gpu.device_number == 0) && (get_elapsed(&last_update_time) >= UPDATE_INTERVAL) && (thread_complete == 0))
	output_progress(1);
    }
  }

  /* Never reached. */
  return NULL;
}


/* Writes the chains given by the kernel to the file. */
void write_chains(char *filename, unsigned int chains_per_work_unit, cl_ulong *start_indices, unsigned int start_indices_size, cl_ulong *end_indices, unsigned int end_indices_size, unsigned int thread_id) {
  int i = 0, j = 0;
  unsigned int file_size = 0;
  cl_ulong start = 0;
  rc_file f = rc_fopen(filename, 0), l = NULL;
  char log_filename[256] = {0};
  int empty_chains = 0;


  if (f == NULL)
    exit(-1);

  /* Get an exclusive lock on all bytes of the file, including those not yet written
   * (i.e.: another thread cannot write past the current end of the file).  */
  if (rc_flock(f) != 0)
    exit(-1);

  /* Get the filename of the rainbow table log to write to, then open it for appending.
   *  This is the same filename as the rainbow table, but with ".log" appended. */
  get_rt_log_filename(log_filename, sizeof(log_filename), filename);
  l = rc_fopen(log_filename, 1);
  if (l == NULL)
    exit(-1);

  /* Get a lock on the log.  Probably not strictly necessary, since the table is locked
   * first, and other threads are blocked at this point... */
  if (rc_flock(l) != 0)
    fprintf(stderr, "\nError while locking log file!\n");

  /* Go to the end of the table file. */
  if (rc_fseek(f, 0, RCSEEK_END) != 0) {
    fprintf(stderr, "Error seeking to end of output file.\n");
    exit(-1);
  }

  /* If we have results that extend past the end of the file, write zeros as
   * placeholders until we get to the point where our data starts. */
  file_size = rc_ftell(f);

  rt_log(l, "Thread #%u: file size at start is %u (%u chains)\n", thread_id, file_size, file_size / CHAIN_SIZE);

  empty_chains = (int)((((start_indices[0] - first_generated_chain) * CHAIN_SIZE) - file_size) / CHAIN_SIZE);

  if (empty_chains > 0)
    rt_log(l, "\tWriting %d empty chains (%u bytes)\n", empty_chains, empty_chains * CHAIN_SIZE);

  for (i = 0; i < empty_chains; i++) {
    rc_fwrite(&start, sizeof(start), 1, f);
    rc_fwrite(&start, sizeof(start), 1, f);
  }

  /* Otherwise, if another thread wrote placeholders already, seek to the point at which
   * we need to overwrite. */
  rt_log(l, "\tSeeking to position %lu (chain #%lu).\n", (start_indices[0] - first_generated_chain) * CHAIN_SIZE, start_indices[0] - first_generated_chain);
  if (rc_fseek(f, (start_indices[0] - first_generated_chain) * CHAIN_SIZE, RCSEEK_SET) != 0) {
    perror("Error seeking in file");
    exit(-1);
  }

  /* Write the chains. */
  for (i = 0; i < start_indices_size; i++) {
    start = start_indices[i];
    for (j = (i * chains_per_work_unit); (j < ((i * chains_per_work_unit) + chains_per_work_unit)) && (j < end_indices_size); j++) {
      rc_fwrite(&start, sizeof(cl_ulong), 1, f);
      rc_fwrite(&(end_indices[j]), sizeof(cl_ulong), 1, f);
      start++;
    }
  }

  if (start_indices_size > 0)
    rt_log(l, "\tWrote chains start indices from %"PRIu64" to %"PRIu64"\n", start_indices[0], start - 1);


  rc_fclose(l);
  rc_fclose(f);
}


int main(int ac, char **av) {
  cl_platform_id platforms[MAX_NUM_PLATFORMS] = {0};
  cl_device_id devices[MAX_NUM_DEVICES] = {0};
  pthread_t threads[MAX_NUM_DEVICES] = {0};
  char filename[256] = {0}, time_str[128] = {0};

  FILE *f = NULL;
  unsigned int file_size = 0;
  thread_args *args = NULL;
  char *hash_name = NULL, *charset_name = NULL, *charset = NULL;
  unsigned int plaintext_len_min = 0, plaintext_len_max = 0, total_chains_in_table = 0, table_index = 0, benchmark_mode = 0;
  unsigned int resuming_table = 0;  /* Set when a table gen is being resumed. */
  cl_uint hash_type = 0, chain_len = 0, num_platforms = 0, num_devices = 0;
  uint64_t part_index = 0;
  int i = 0;


  ENABLE_CONSOLE_COLOR();
  PRINT_PROJECT_HEADER();
#ifndef _WIN32
  /* Allows printf() to insert commas in thousandths place. */
  setlocale(LC_NUMERIC, "");
  /*setenv("CUDA_CACHE_DISABLE", "1", 1);*/ /* Disables kernel caching. */
  /*setenv("HSA_ENABLE_SDMA", "0", 1);*/ /* The ROCm driver on AMD Vega 64 doesn't work without this. */
#endif

  if ((ac != 9) && (ac != 11))
    print_usage_and_exit(av[0], -1);
  if ((ac == 11) && (strcmp(av[9], "-gws") != 0))
    print_usage_and_exit(av[0], -1);

  /* Read command-line arguments. */
  hash_name = av[1];
  charset_name = av[2];
  plaintext_len_min = (unsigned int)atoi(av[3]);
  plaintext_len_max = (unsigned int)atoi(av[4]);
  table_index = (unsigned int)atoi(av[5]);
  chain_len = (unsigned int)atoi(av[6]);
  total_chains_in_table = (unsigned int)atoi(av[7]);

  /* See if the user wants to run the benchmarks. */
  if (strcmp(av[8], "-bench") == 0) {
    benchmark_mode = 1;
    printf("Benchmarks have been disabled in this release due to inconsistent results.  They may be re-implemented in a future release.\n\nIn the meantime, a rough benchmark can be achieved by generating the following table:\n\n  %s ntlm ascii-32-95 8 8 0 422000 1000000 0\n\n", av[0]);
    exit(-1);
  } else
    part_index = (unsigned int)atoi(av[8]);

  /* Manually override the global work size. */
  if (ac == 11)
    user_provided_gws = (unsigned int)atoi(av[10]);


  /* Check that this system has sufficient RAM. */
  CHECK_MEMORY_SIZE();


  /* Format the filename based on the user options. */
  snprintf(filename, sizeof(filename) - 1, "%s_%s#%u-%u_%u_%ux%u_%"PRIu64".rt", hash_name, charset_name, plaintext_len_min, plaintext_len_max, table_index, chain_len, total_chains_in_table, part_index);


  /* If the user provided an invalid hash name, dump the valid options and
   * exit. */
  hash_type = hash_str_to_type(hash_name);
  if (hash_type == HASH_UNDEFINED) {
    fprintf(stderr, "Error: hash \"%s\" not supported.  Valid values are:\n", hash_name);
    for (i = 0; i < (sizeof(valid_hash_names) / sizeof(struct hash_names)); i++)
      fprintf(stderr, "%s\n", valid_hash_names[i].name);
    exit(-1);
  }


  charset = validate_charset(charset_name);
  if (charset == NULL) {
    char buf[256] = {0};

    get_valid_charsets(buf, sizeof(buf));
    fprintf(stderr, "Error: charset \"%s\" not supported.  Valid values are: %s", charset_name, buf);
    exit(-1);
  }


  /* Ensure that the plaintext max length is set and is less than 256.  Also
   * ensure that the max is greater than the min. */
  if ((plaintext_len_max == 0) || (plaintext_len_max > MAX_PLAINTEXT_LEN)) {
    fprintf(stderr, "Error: plaintext max length must be greater than 0 and less than 256.\n");
    exit(-1);
  } else if (plaintext_len_min > plaintext_len_max) {
    fprintf(stderr, "Error: plaintext min length must be less than plaintext max length.\n");
    exit(-1);
  } else if (plaintext_len_min < 8) {
    printf("\n!! Warning: the minimum plaintext length is less than 8.  In present day, it is not very efficient to use rainbow tables to crack passwords of length 1 through 7; GPU brute-forcing is much more effective in those cases.  Continuing...\n\n");
  }

  /* Ensure that the chain length and chain counts are set. */
  if ((chain_len == 0) || (total_chains_in_table == 0)) {
    fprintf(stderr, "Chain length and chain count must both be greater than 0.\n");
    exit(-1);
  }

  /* The original rcrack didn't support chain counts >= 128M, as that would
   * result in files greater than 2GB in size.  It may work with modern
   * rcrack/rcracki_mt, but its untested as of right now... */
  if (total_chains_in_table >= 134217728)
    printf("\nWARNING: chain counts >= 134217728 are untested.  Generated tables may not work in rcrack/rcracki_mt.  Continuing anyway...\n\n");

  /* Create the output file and test if it can be successfully locked. */
  f = rc_fopen(filename, 1);
  if (f == NULL) {
    fprintf(stderr, "Failed to create/open file: %s\n", filename);
    exit(-1);
  }

  if (rc_flock(f) != 0) {
    fprintf(stderr, "Error locking file: %s\n", filename);
    exit(-1);
  }

  file_size = rc_ftell(f);  /* File was opened for appending, so this holds the size. */
  rc_fclose(f);

  /* If the file size implies that it is already complete, run the verifier on it. */
  if (file_size == (total_chains_in_table * CHAIN_SIZE)) {
    if (verify_rainbowtable_file(filename, VERIFY_TABLE_TYPE_GENERATED, VERIFY_TABLE_IS_COMPLETE, VERIFY_TRUNCATE_ON_ERROR, -1)) {
      /* The table is complete, so tell the user and exit. */
      printf("Table in \"%s\" already appears to be complete.  Terminating...\n", filename);
      exit(0);
    } else {  /* The table was invalid, and was truncated, so we should continue... */
      struct stat st;
      memset(&st, 0, sizeof(st));

      /* Since the table was truncated above, update the file size. */
      if (stat(filename, &st) != 0) {
	perror("Error calling stat()");
	exit(-1);
      }
      file_size = st.st_size;
    }
  }

  /* If the file already exists and isn't empty, then verify the file, and update
   * the start_index so that we resume generation. */
  if (file_size > 0) {
    printf("\n  !! WARNING !!\n\nIt appears that the output table is partially generated.  An attempt to resume generation will be made, but know that this is experimental and may end up failing after hours of work.  A near-future release will further refine this feature.\n\n"); fflush(stdout);

    verify_rainbowtable_file(filename, VERIFY_TABLE_TYPE_GENERATED, VERIFY_TABLE_MAY_BE_INCOMPLETE, VERIFY_TRUNCATE_ON_ERROR, -1);

    /* fopen()'s modes are weird.  Its easier to just re-open the file for reading
     * at this point, rather than change the code above and re-use the open handle. */
    f = rc_fopen(filename, 0);
    if (f == NULL)
      exit(-1);

    /* The file size may be different now if the verification function, above,
     * truncated it due to errors.  Ensure that at least one chain is in the file. */
    rc_fseek(f, 0, RCSEEK_END);
    if (rc_ftell(f) >= CHAIN_SIZE) {

      /* Seek to the last starting index in the file and read it. */
      rc_fseek(f, CHAIN_SIZE, RCSEEK_END);
      rc_fread(&start_index, sizeof(start_index), 1, f);

      start_index++;  /* Increment the index to the next one needed. */
      first_generated_chain = start_index;

      /* The number of chains left to generate would be the total requested by the
       * user, minus the number of chains already in the file. */
      rc_fseek(f, 0, RCSEEK_END);
      num_chains_to_generate = total_chains_in_table - (rc_ftell(f) / CHAIN_SIZE);

      resuming_table = 1;
    }
    rc_fclose(f);
  } else {  /* This is a new table. */
    uint64_t plaintext_space_up_to_index[16] = {0};


    start_index = first_generated_chain = total_chains_in_table * part_index;
    num_chains_to_generate = total_chains_in_table;

    /* Ensure our plaintext_space_up_to_index array is large enough to call
     * fill_plaintext_space_table() with. */
    if (plaintext_len_max > (sizeof(plaintext_space_up_to_index) + 1)) {
      fprintf(stderr, "\n  !! Warning: plaintext length max is too large (%u > %"PRIu64").  Skipping start index safety check.\n\n", plaintext_len_max, sizeof(plaintext_space_up_to_index) + 1);  fflush(stderr);
    } else {
      uint64_t plaintext_space_total = fill_plaintext_space_table(strlen(charset), plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

      /* Ensure that the user didn't specify a part index so great that it
       * overflows the plaintext space total.  If so, calculate the largest
       * part index that can be used with this character set and tell the
       * user before terminating. */
      if (start_index + num_chains_to_generate > plaintext_space_total) {
	uint64_t highest_part_index = plaintext_space_total / num_chains_to_generate;
	if ((plaintext_space_total % num_chains_to_generate) != 0)
	  highest_part_index--;

	fprintf(stderr, "\n  !! Error: start index (%"PRIu64") + number of chains to generate (%u) > plaintext space total (%"PRIu64")!  The highest part index that can be generated without causing this overflow is %"PRIu64" (hint: you set the part index too high (%"PRIu64").\n\n", start_index, num_chains_to_generate, plaintext_space_total, highest_part_index, part_index); fflush(stderr);
	exit(-1);
      }
    }
  }

  /* Get the number of platforms and devices available. */
  get_platforms_and_devices(-1, MAX_NUM_PLATFORMS, platforms, &num_platforms, MAX_NUM_DEVICES, devices, &num_devices, VERBOSE);

  /* Check the device type and set flags.*/
  if (num_devices > 0) {
    char device_vendor[128] = {0};

    get_device_str(devices[0], CL_DEVICE_VENDOR, device_vendor, sizeof(device_vendor) - 1);
    if (strstr(device_vendor, "Advanced Micro Devices") != NULL)
      is_amd_gpu = 1;
  }

  /* Initialize the barrier.  This is used in some cases to ensure kernels across
   * multiple devices run concurrently. */
  if (pthread_barrier_init(&barrier, NULL, num_devices) != 0) {
    fprintf(stderr, "pthread_barrier_init() failed.\n");
    exit(-1);
  }

  args = calloc(num_devices, sizeof(thread_args));
  if (args == NULL) {
    fprintf(stderr, "Error while creating thread arg array.\n");
    exit(-1);
  }

  /* Print info about how we're generating the table. */
  printf("Output file:\t\t%s\nHash algorithm:\t\t%s\nCharset name:\t\t%s\nCharset:\t\t%s\nCharset length:\t\t%"PRIu64"\nPlaintext length range: %u - %u\nReduction offset:\t0x%x\nChain length:\t\t%u\nNumber of chains:\t%u\nPart index:\t\t%"PRIu64"\n\n", filename, hash_name, charset_name, charset, strlen(charset), plaintext_len_min, plaintext_len_max, TABLE_INDEX_TO_REDUCTION_OFFSET(table_index), chain_len, total_chains_in_table, part_index);

  /* If we found a file to append to, tell the user what's happening. */
  if (resuming_table)
    printf("Appending to existing file (%s) at chain #X.\n\n", filename);

  /* Print a time stamp of when the generation begins. */
  start_timer(&global_start_time);
  last_update_time = global_start_time;

  {
    time_t current_time = time(NULL);
    strftime(time_str, sizeof(time_str), "%b. %d, %Y at %I:%M %p", localtime(&current_time));
    printf("Table generation started on %s...\n\n", time_str);  fflush(stdout);
  }

  /* Spin up one host thread per GPU. */
  for (i = 0; i < num_devices; i++) {
    args[i].benchmark_mode = benchmark_mode;
    args[i].hash_type = hash_type;
    args[i].charset = charset;
    args[i].plaintext_len_min = plaintext_len_min;
    args[i].plaintext_len_max = plaintext_len_max;
    args[i].table_index = table_index;
    args[i].reduction_offset = TABLE_INDEX_TO_REDUCTION_OFFSET(table_index);
    args[i].chain_len = chain_len;
    args[i].filename = filename;
    args[i].initial_chains_per_execution = INITIAL_CHAINS_PER_EXECUTION;
    args[i].gpu.device_number = i;
    args[i].gpu.device = devices[i];

    if (benchmark_mode)
      args[i].initial_chains_per_execution = total_chains_in_table;

    if (pthread_create(&(threads[i]), NULL, &host_thread, &(args[i]))) {
      perror("Failed to create thread");
      exit(-1);
    }
  }

  /* Wait for all threads to finish. */
  for (i = 0; i < num_devices; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("Failed to join with thread");
      exit(-1);
    }
  }

  /*elapsed =  difftime(time(NULL), global_start_time);*/
  if (benchmark_mode) { /* Benchmark... */
    /*unsigned int total_chains_generated = total_chains_in_table * (num_devices * args[0].compute_unit_multiple);

      printf("Generated %u chains on each of %u devices (%u chains total) in %.1f seconds.\nRate: %.1f/s\n", (total_chains_in_table * args[0].compute_unit_multiple), num_devices, total_chains_generated, elapsed, total_chains_generated / elapsed);*/
  } else { /* Normal table generation... */
    struct stat st;

    memset(&st, 0, sizeof(struct stat));

    /* Output the run time, number of chains generated, and rate. */
    output_progress(0);
    printf("\nGeneration complete!\n");

    if (stat(filename, &st) == 0) {
      unsigned int actual_num_chains = st.st_size / CHAIN_SIZE;

      /* If we generated more chains than the user requested, rename the file to
       * reflect this. */
      if (actual_num_chains > total_chains_in_table) {
	if (VERBOSE)
	  printf("\nNote %u extra chains created.  Truncating...\n", actual_num_chains - total_chains_in_table);
	if (truncate(filename, total_chains_in_table * CHAIN_SIZE) != 0) {
	  fprintf(stderr, "Error while truncating file %s: %s (%d)\n", filename, strerror(errno), errno);
	}

	/*
	char new_filename[sizeof(filename)];
	memset(new_filename, 0, sizeof(new_filename));

	snprintf(new_filename, sizeof(new_filename) - 1, "%s_%s#%u-%u_%u_%ux%u_%u.rt", hash_name, charset_name, plaintext_len_min, plaintext_len_max, table_index, chain_len, actual_num_chains, part_index);
	if (!rename(filename, new_filename)) {
	  printf("\nNote: because extra chains were generated, the file name was renamed to reflect this (from \"%s\" to \"%s\").\n\n", filename, new_filename);
	  strncpy(filename, new_filename, sizeof(filename) - 1);
	} else
	  perror("Error while renaming file");
	*/
      }
    }

    /* Verify that the new table is valid. */
    printf("Now verifying rainbow table... ");
    fflush(stdout);
    if (!verify_rainbowtable_file(filename, VERIFY_TABLE_TYPE_GENERATED, VERIFY_TABLE_IS_COMPLETE, VERIFY_TRUNCATE_ON_ERROR, -1)) {
      char log_filename[256] = {0};

      get_rt_log_filename(log_filename, sizeof(log_filename), filename);
      printf("\n");
      fprintf(stderr, "Error while verifying rainbowtable!  It has been truncated to just before the point of error.  Please give the following file to the developer: %s\n\n", log_filename);
    } else {
      /* Delete the rainbow table generation log, since the table was verified to be
       * correct.  No need to keep this debugging info. */
      delete_rt_log(filename);
      printf("done!\n");
    }
  }

  for (i = 0; i < num_devices; i++)
    rc_clReleaseDevice(devices[i]);

  pthread_barrier_destroy(&barrier);
  FREE(args);
  return 0;
}
