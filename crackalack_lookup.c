/*
 * Rainbow Crackalack: crackalack_lookup.c
 * Copyright (C) 2018-2020  Joe Testa <jtesta@positronsecurity.com>
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

/*
 * Performs GPU-accelerated password hash lookups on rainbow tables.
 */

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/sysinfo.h>
#define O_BINARY 0
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <CL/cl.h>

#include "charset.h"
#include "clock.h"
#include "cpu_rt_functions.h"
#include "hash_validate.h"
#include "misc.h"
#include "opencl_setup.h"
#include "rtc_decompress.h"
#include "shared.h"
#include "test_shared.h"  /* TODO: move hex_to_bytes() elsewhere. */
#include "verify.h"
#include "version.h"

#define VERBOSE 1
#define PRECOMPUTE_KERNEL_PATH "precompute.cl"
#define PRECOMPUTE_NTLM8_KERNEL_PATH "precompute_ntlm8.cl"
#define FALSE_ALARM_KERNEL_PATH "false_alarm_check.cl"
#define FALSE_ALARM_NTLM8_KERNEL_PATH "false_alarm_check_ntlm8.cl"



/* Struct to form a linked list of precomputed end indices, and potential start indices (which are usually false alarms). */
struct _precomputed_and_potential_indices {
  char *hash;
  cl_ulong *precomputed_end_indices;
  cl_uint num_precomputed_end_indices;

  cl_ulong *potential_start_indices;
  unsigned int num_potential_start_indices;
  unsigned int potential_start_indices_size;
  unsigned int *potential_start_index_positions; /* Buffer size is always num_potential_start_indices. */

  char *plaintext;        /* Set if hash is cracked. */
  char *index_filename;   /* File path containing the ".index" file. */
  struct _precomputed_and_potential_indices *next;
};
typedef struct _precomputed_and_potential_indices precomputed_and_potential_indices;


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
  unsigned int hash_type;
  char *hash_name;
  char *hash; /* In hex. */
  char *charset;
  char *charset_name;
  unsigned int plaintext_len_min;
  unsigned int plaintext_len_max;
  unsigned int table_index;
  unsigned int reduction_offset;
  unsigned int chain_len;

  unsigned int total_devices;
  uint64_t *results;
  unsigned int num_results;

  cl_ulong *potential_start_indices;
  unsigned int num_potential_start_indices;
  
  /* Buffer size is always num_potential_start_indices. */
  unsigned int *potential_start_index_positions;
  
  /* Length is always num_potential_start_indices. */
  cl_ulong *hash_base_indices;

  gpu_dev gpu;
} thread_args;


/* Struct to pass to binary search threads. */
typedef struct {
  cl_ulong *rainbow_table;
  unsigned int num_chains;
  precomputed_and_potential_indices *ppi_head;
  unsigned int thread_number;
  unsigned int total_threads;
} search_thread_args;


/* Struct to hold node in linked list of preloaded tables. */
struct _preloaded_table {
  char *filepath;
  cl_ulong *rainbow_table;
  unsigned int num_chains;
  struct _preloaded_table *next;
};
typedef struct _preloaded_table preloaded_table;

typedef struct {
  char *rt_dir;
} preloading_thread_args;


unsigned int count_tables(char *dir);
void find_rt_params(char *dir, rt_parameters *rt_params);
void free_loaded_hashes(char **hashes, unsigned int *num_hashes);
void *host_thread_false_alarm(void *ptr);
void *preloading_thread(void *ptr);
cl_ulong *search_precompute_cache(char *index_data, unsigned int *num_indices, char *filename, unsigned int filename_size);
void search_tables(unsigned int total_tables, precomputed_and_potential_indices *ppi, thread_args *args);
void save_cracked_hash(precomputed_and_potential_indices *ppi, unsigned int hash_type);


/* The path of the pot file to store cracked hashes in.  This can be overridden by
 * a command line arg. */
char jtr_pot_filename[128] = "rainbowcrackalack_jtr.pot";
char hashcat_pot_filename[128] = "rainbowcrackalack_hashcat.pot";

/* The number of seconds spent on precomputation, file I/O, searching, and false alarm
 * checking. */
double time_precomp = 0, time_io = 0, time_searching = 0, time_falsealarms = 0;

/* The total number of false alarms, chains processed, respectively. */
uint64_t num_falsealarms = 0, num_chains_processed = 0;

/* The total number of hashes cracked in this invokation and number of tables
 * processed, respectively. */
unsigned int num_cracked = 0, num_tables_processed = 0;

/* Mutex to protect the precomputed_and_potential_indices array. _*/
pthread_mutex_t ppi_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Barrier to ensure that kernels on multiple devices are all run at the same time.
 * The closed-source AMD driver on Windows effectively blocks other devices while
 * one kernel is running; this ensures parallelization in that environment, since
 * all kernels will run at once.  The open source AMD ROCm driver on Linux may or
 * may not get a very slight performance bump with this enabled. */
pthread_barrier_t barrier = {0};

/* Set to 1 if AMD GPUs found. */
unsigned int is_amd_gpu = 0;

/* The global work size, as over-ridden by the user on the command line. */
size_t user_provided_gws = 0;

/* The total number of precomputed indices loaded into memory.  Each one of these is
 * a cl_ulong (8 bytes). */
uint64_t total_precomputed_indices_loaded = 0;

/* Set to 1 if the NTLM8 message was printed.  This prevents console spam. */
unsigned int printed_precompute_ntlm8_message = 0;
unsigned int printed_false_alarm_ntlm8_message = 0;

/* The total number of tables in all subdirectories of the directory given
 * by the user. */
unsigned int total_tables = 0;

/* Set to 1 by the preloading thread to indicate that no more tables exist for loading. */
unsigned int table_loading_complete = 0;

/* The current size of the preloaded tables list. */
unsigned int num_preloaded_tables_available = 0;

/* A linked list of preloaded tables. */
preloaded_table *preloaded_table_list = NULL;

/* Condition for the main thread to wait for more tables on. */
pthread_cond_t condition_wait_for_tables = PTHREAD_COND_INITIALIZER;

/* Condition for the preloading thread to wait on (when the MAX_PRELOAD_NUM is reached). */
pthread_cond_t condition_continue_loading_tables = PTHREAD_COND_INITIALIZER;

/* The lock for the preloaded tables system. */
pthread_mutex_t preloaded_tables_lock = PTHREAD_MUTEX_INITIALIZER;

/* The time at which table searching begins. */
struct timespec search_start_time = {0};


/* The total number of tables to preload in memory while binary searching and false
 * alarm checking is done by the main thread. */
#define MAX_PRELOAD_NUM 2

#define LOCK_PPI() \
  if (pthread_mutex_lock(&ppi_mutex)) { perror("Failed to lock mutex"); exit(-1); }

#define UNLOCK_PPI() \
  if (pthread_mutex_unlock(&ppi_mutex)) { perror("Failed to unlock mutex"); exit(-1); }


/* Adds a potential start index (and position within the chain) to check for false
 * alarms. */
void add_potential_start_index_and_position(precomputed_and_potential_indices *ppi, cl_ulong start, unsigned int position) {
  #define POTENTIAL_START_INDICES_INITIAL_SIZE 16

  LOCK_PPI();

  /* Initialize the potential_start_indices buffer if it isn't already. */
  if (ppi->potential_start_indices == NULL) {
    ppi->potential_start_indices = calloc(POTENTIAL_START_INDICES_INITIAL_SIZE, sizeof(cl_ulong));
    ppi->potential_start_index_positions = calloc(POTENTIAL_START_INDICES_INITIAL_SIZE, sizeof(cl_ulong));
    if ((ppi->potential_start_indices == NULL) || (ppi->potential_start_index_positions == NULL)) {
      fprintf(stderr, "Failed to initialize potential_start_indices / potential_start_index_positions buffer.\n");
      exit(-1);
    }
    ppi->potential_start_indices_size = POTENTIAL_START_INDICES_INITIAL_SIZE;
  }

  /* If its time to re-size the array... */
  if (ppi->num_potential_start_indices == ppi->potential_start_indices_size) {
    unsigned int new_size_in_ulongs = ppi->potential_start_indices_size * 2;

    /*printf("Resizing array from %u to %u.\n", ppi->potential_start_indices_size, new_size_in_ulongs);*/
    ppi->potential_start_indices = recalloc(ppi->potential_start_indices, new_size_in_ulongs * sizeof(cl_ulong), ppi->potential_start_indices_size * sizeof(cl_ulong));
    ppi->potential_start_index_positions = recalloc(ppi->potential_start_index_positions, new_size_in_ulongs * sizeof(cl_ulong), ppi->potential_start_indices_size * sizeof(cl_ulong));
    if ((ppi->potential_start_indices == NULL) || (ppi->potential_start_index_positions == NULL)) {
      fprintf(stderr, "Failed to re-allocate potential_start_indices/potential_start_index_positions buffer to %u.\n", new_size_in_ulongs);
      exit(-1);
    }
    ppi->potential_start_indices_size = new_size_in_ulongs;
  }
  ppi->potential_start_indices[ppi->num_potential_start_indices] = start;
  ppi->potential_start_index_positions[ppi->num_potential_start_indices] = position;
  ppi->num_potential_start_indices++;

  UNLOCK_PPI();
}


void check_false_alarms(precomputed_and_potential_indices *ppi, thread_args *args) {
  pthread_t threads[MAX_NUM_DEVICES] = {0};
  char time_str[128] = {0};
  struct timespec start_time = {0};
  cl_ulong plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};

  unsigned int num_potential_start_indices = 0, i = 0, j = 0;
  unsigned int total_devices = args[0].total_devices;
  cl_ulong plaintext_space_total = 0;
  double time_delta = 0.0;

  precomputed_and_potential_indices *ppi_cur = ppi;
  cl_ulong *potential_start_indices = NULL, *hash_base_indices = NULL;
  unsigned int *potential_start_index_positions = NULL;
  precomputed_and_potential_indices **ppi_refs = NULL;


  /* First count all the potential start indices. */
  while(ppi_cur) {
    num_potential_start_indices += ppi_cur->num_potential_start_indices;
    ppi_cur = ppi_cur->next;
  }

  /* If no potential matches were found, there's nothing else to do. */
  if (num_potential_start_indices == 0) {
    printf("No matches found in table.\n");
    return;
  }
  printf("  Checking %u potential matches...\n", num_potential_start_indices);  fflush(stdout);
  num_falsealarms += num_potential_start_indices;

  /* Allocate a buffer to hold them all. */
  potential_start_indices = calloc(num_potential_start_indices, sizeof(cl_ulong));
  potential_start_index_positions = calloc(num_potential_start_indices, sizeof(cl_ulong));
  hash_base_indices = calloc(num_potential_start_indices, sizeof(cl_ulong));
  ppi_refs = calloc(num_potential_start_indices, sizeof(precomputed_and_potential_indices *));
  if ((potential_start_indices == NULL) || (potential_start_index_positions == NULL) || (hash_base_indices == NULL) || (ppi_refs == NULL)) {
    fprintf(stderr, "Error while creating buffer for potential start indices/positions/hash indices/ppi refs.\n");
    exit(-1);
  }

  plaintext_space_total = fill_plaintext_space_table(strlen(args->charset), args->plaintext_len_min, args->plaintext_len_max, plaintext_space_up_to_index);

  /* Collate all the start indices into one buffer. */
  ppi_cur = ppi;
  while(ppi_cur) {
    unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};
    unsigned int hash_len = hex_to_bytes(ppi_cur->hash, sizeof(hash), hash);
    cl_ulong hash_base_index = hash_to_index(hash, hash_len, args->reduction_offset, plaintext_space_total, 0);  /* We always use position 0 here.  When the GPU code is comparing indices, it will add in the current position. */


    if (ppi_cur->plaintext == NULL) {
      for (i = 0; i < ppi_cur->num_potential_start_indices; i++, j++) {
	potential_start_indices[j] = ppi_cur->potential_start_indices[i];
	potential_start_index_positions[j] = ppi_cur->potential_start_index_positions[i];
	hash_base_indices[j] = hash_base_index;

	/* For this index, hold a reference to the ppi struct.  This later lets us find
	 * the ppi, given a result index from the GPU. */
	ppi_refs[j] = ppi_cur;
      }
    }

    ppi_cur = ppi_cur->next;
  }

  /*for (i = 0; i < num_potential_start_indices; i++)
    printf("Start point: %lu; Chain position: %u; hash base index: %lu\n", potential_start_indices[i], potential_start_index_positions[i], hash_base_indices[i]);*/

  /* Start the timer false alarm checking. */
  start_timer(&start_time);

  /* Start one thread to control each GPU. */
  for (i = 0; i < total_devices; i++) {

    /* Each thread gets the same reference to the list of potential start indices. */
    args[i].potential_start_indices = potential_start_indices;
    args[i].num_potential_start_indices = num_potential_start_indices;
    args[i].potential_start_index_positions = potential_start_index_positions;
    args[i].hash_base_indices = hash_base_indices;

    if (pthread_create(&(threads[i]), NULL, &host_thread_false_alarm, &(args[i]))) {
      perror("Failed to create thread");
      exit(-1);
    }
  }

  /* Wait for all threads to finish. */
  for (i = 0; i < total_devices; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("Failed to join with thread");
      exit(-1);
    }
  }

  /* Search for valid results, and update the ppi with the plaintext. */
  for (i = 0; i < total_devices; i++) {
    for (j = 0; j < args[i].num_results; j++) {
      if (args[i].results[j] != 0) {
	char plaintext[MAX_PLAINTEXT_LEN] = {0};
	unsigned int plaintext_len = 0;


	index_to_plaintext(args[i].results[j], args[i].charset, strlen(args[i].charset), args[i].plaintext_len_min, args[i].plaintext_len_max, plaintext_space_up_to_index, plaintext, &plaintext_len);

	/* Double check NTLM results to weed out super false alarms. */
	if (args[i].hash_type == HASH_NTLM) {
	  unsigned char hash[16] = {0};
	  char hash_hex[(sizeof(hash) * 2) + 1] = {0};


	  ntlm_hash(plaintext, plaintext_len, hash);
	  if (!bytes_to_hex(hash, sizeof(hash), hash_hex, sizeof(hash_hex)) || \
	      (strcmp(hash_hex, ppi_refs[j]->hash) != 0)) {
	    /*printf("Found super false positive!: NTLM('%s') != %s\n", plaintext, ppi_refs[j]->hash);*/
	    continue;
	  }
	} else
	  printf("WARNING: CPU code to double-check this cracked hash has not yet been added.  There is a 60%% chance this is a false positive!  A workaround is to use John The Ripper to validate this result(s).\n");

	/* Its official: we cracked a hash! */

	/* Save the plaintext, clear the precomputed end indices list (since its
	 * no longer useful, save the hash/plaintext combo into the pot file, and
	 * tell the user. */
	ppi_refs[j]->plaintext = strdup(plaintext);
	ppi_refs[j]->num_precomputed_end_indices = 0;
	FREE(ppi_refs[j]->precomputed_end_indices);

	save_cracked_hash(ppi_refs[j], args[i].hash_type);
	printf("HASH CRACKED => %s:%s\n", ppi_refs[j]->hash, plaintext);  fflush(stdout);
      }
    }
  }
  time_delta = get_elapsed(&start_time);

  time_falsealarms += time_delta;
  seconds_to_human_time(time_str, sizeof(time_str), (unsigned int)time_delta);
  printf("  Completed false alarm checks in %s.\n", time_str);  fflush(stdout);

  FREE(potential_start_indices);
  FREE(potential_start_index_positions);
  FREE(hash_base_indices);
  FREE(ppi_refs);
  FREE(args->results);
  args->num_results = 0;
}


/* Print a warning to the user if a lot of memory is used by the pre-computed indices. */
void check_memory_usage() {
  uint64_t total_memory = get_total_memory(), num_precompute_bytes = 0;
  double percent_memory_used = 0.0;


  if (total_memory == 0)
    return;

  num_precompute_bytes = total_precomputed_indices_loaded * sizeof(cl_ulong);
  percent_memory_used = ((double)num_precompute_bytes / (double)total_memory) * 100;
  if (percent_memory_used > 65) {
    printf("\n\n\n\t!! WARNING !!\n\n\tThe pre-computed indices take up more than 65%% of total RAM!  This may result in strange failures from clFinish() and other OpenCL functions.  If this happens, either run this lookup with a smaller number of hashes at a time, or do it on a machine with more memory.\n\n\tMemory used by pre-compute indices: %"QUOTE PRIu64"\n\tTotal RAM: %"QUOTE PRIu64"\n\tPercent used: %.1f%%\n\n\n\n", num_precompute_bytes, total_memory, percent_memory_used);
  }
}


/* Free all the potential start indices. */
void clear_potential_start_indices(precomputed_and_potential_indices *ppi) {
  precomputed_and_potential_indices *ppi_cur = ppi;


  while(ppi_cur) {
    FREE(ppi_cur->potential_start_indices);
    FREE(ppi_cur->potential_start_index_positions);
    ppi_cur->num_potential_start_indices = 0;

    ppi_cur = ppi_cur->next;
  }
}


/* Returns the total number of *.rt and *.rtc in all subdirectories of the
 * specified directory. */
unsigned int count_tables(char *dir) {
  DIR *d = NULL;
  struct dirent *de = NULL;
  unsigned int ret = 0, is_file = 0, is_dir = 0;


  d = opendir(dir);
  if (d == NULL)
    return 0;

  while ((de = readdir(d)) != NULL) {
#ifdef _WIN32
    struct stat st = {0};
    char path[256] = {0};

    /* The d_type field of the dirent struct is not a POSIX standard, and Windows
     * doesn't support it.  So we fall back to using stat(). */
    snprintf(path, sizeof(path) - 1, "%s\\%s", dir, de->d_name);
    if (stat(path, &st) < 0) {
      fprintf(stderr, "Error: failed to stat() %s: %s.  Continuing anyway...\n", path, strerror(errno));  fflush(stderr);
      is_file = 0;
      is_dir = 0;
    } else {
      is_file = S_ISREG(st.st_mode);
      is_dir = S_ISDIR(st.st_mode);
    }
#else
    /* Linux has the d_type field, which is much more efficient to use than doing
     * another stat(). */
    is_file = (de->d_type == DT_REG);
    is_dir = (de->d_type == DT_DIR);
#endif

    if (is_file && (str_ends_with(de->d_name, ".rt") || str_ends_with(de->d_name, ".rtc")))
      ret++;
    else if (is_dir && (strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0))
      ret += count_tables(de->d_name);
  }

  closedir(d);
  return ret;
}


/* Free the hashes we loaded from disk or command line. */
void free_loaded_hashes(char **hashes, unsigned int *num_hashes) {
  if (hashes != NULL) {
    unsigned int i = 0;
    for (; i < *num_hashes; i++) {
      FREE(hashes[i]);
    }
    FREE(hashes);
    *num_hashes = 0;
  }
}


/* Recursively searches the target directory for the first rainbow table file, and uses its filename to infer
 * the rainbow table parameters. */
void find_rt_params(char *dir_name, rt_parameters *rt_params) {
  char filepath[512] = {0};
  DIR *dir = NULL;
  struct dirent *de = NULL;
  struct stat st;


  dir = opendir(dir_name);
  if (dir == NULL)  /* This directory may not allow the current process permission. */
    return;

  while ((de = readdir(dir)) != NULL) {

    /* Create an absolute path to this entity. */
    filepath_join(filepath, sizeof(filepath), dir_name, de->d_name);

    /* If this is a directory, recurse into it. */
    if ((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0) && (stat(filepath, &st) == 0) && S_ISDIR(st.st_mode)) {
      find_rt_params(filepath, rt_params);

      /* If we're searching for rainbowtable parameters, and successfully parsed them
       * in the recursive call, we're done. */
      if ((rt_params != NULL) && rt_params->parsed) {
	closedir(dir); dir = NULL;
	return;
      }

    /* If this is a compressed or uncompressed rainbow table, process it! */
    } else if (str_ends_with(de->d_name, ".rt") || str_ends_with(de->d_name, ".rtc")) {

      /* Try to parse them from this file name.  On success, return immediately
       * (no further processing needed), otherwise continue searching until the
       * first valid set of parameters is found. */
      parse_rt_params(rt_params, de->d_name);
      if (rt_params->parsed) {
	closedir(dir); dir = NULL;
	return;
      }

    }
  }

  closedir(dir); dir = NULL;
}


/* Free the precomputed_hashes linked list. */
void free_precomputed_and_potential_indices(precomputed_and_potential_indices **ppi_head) {
  precomputed_and_potential_indices *ppi = *ppi_head, *ppi_next = NULL;


  while (ppi) {
    ppi_next = ppi->next;

    FREE(ppi->precomputed_end_indices);
    FREE(ppi->potential_start_indices);
    FREE(ppi->potential_start_index_positions);
    FREE(ppi->index_filename);
    ppi->num_potential_start_indices = 0;
    FREE(ppi->plaintext);
    FREE(ppi);

    ppi = ppi_next;
  }
  *ppi_head = NULL;
}


/* Returns the number of CPU cores on this machine. */
unsigned int get_num_cpu_cores() {
#ifdef _WIN32
  SYSTEM_INFO sysinfo = {0};

  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#else
  return get_nprocs();
#endif
}


/* A host thread which controls each GPU for false alarm checks. */
void *host_thread_false_alarm(void *ptr) {
  thread_args *args = (thread_args *)ptr;
  gpu_dev *gpu = &(args->gpu);
  cl_context context = NULL;
  cl_command_queue queue = NULL;
  cl_kernel kernel = NULL;
  int err = 0;
  char *kernel_path = FALSE_ALARM_KERNEL_PATH, *kernel_name = "false_alarm_check";

  cl_mem hash_type_buffer = NULL, charset_buffer = NULL, plaintext_len_min_buffer = NULL, plaintext_len_max_buffer = NULL, reduction_offset_buffer = NULL, plaintext_space_total_buffer = NULL, plaintext_space_up_to_index_buffer = NULL, device_num_buffer = NULL, total_devices_buffer = NULL, num_start_indices_buffer = NULL, start_indices_buffer = NULL, start_index_positions_buffer = NULL, hash_base_indices_buffer = NULL, output_block_buffer = NULL, exec_block_scaler_buffer = NULL;
  /*cl_mem debug_ulong_buffer = NULL;*/

  cl_ulong *start_indices = NULL, *hash_base_indices = NULL, *plaintext_indices = NULL, *output_block = NULL;
  unsigned int *start_index_positions = NULL;

  unsigned int num_start_indices = 0, num_start_index_positions = 0, num_hash_base_indices = 0, num_plaintext_indices = 0, num_exec_blocks = 0, output_block_len = 0, exec_block = 0, output_block_index = 0, plaintext_indicies_index = 0;
  uint64_t plaintext_space_total = 0;
  cl_ulong plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};
  size_t gws = 0, kernel_work_group_size = 0, kernel_preferred_work_group_size_multiple = 0;
  /*cl_ulong debug_ulong[128] = {0};*/


  plaintext_space_total = fill_plaintext_space_table(strlen(args->charset), args->plaintext_len_min, args->plaintext_len_max, plaintext_space_up_to_index);

  num_start_indices = num_start_index_positions = num_hash_base_indices = num_plaintext_indices = args->num_potential_start_indices;

  start_indices = args->potential_start_indices;
  start_index_positions = args->potential_start_index_positions;
  hash_base_indices = args->hash_base_indices;

  plaintext_indices = calloc(num_plaintext_indices, sizeof(cl_ulong));
  if (plaintext_indices == NULL) {
    fprintf(stderr, "Error while allocating buffers.\n");
    exit(-1);
  }

  /* If we're generating the standard NTLM 8-character tables, use the special
   * optimized kernel instead! */
  if (is_ntlm8(args->hash_type, args->charset, args->plaintext_len_min, args->plaintext_len_max, args->reduction_offset, args->chain_len)) {
    kernel_path = FALSE_ALARM_NTLM8_KERNEL_PATH;
    kernel_name = "false_alarm_check_ntlm8";
    if ((args->gpu.device_number == 0) && (printed_false_alarm_ntlm8_message == 0)) { /* Only the first thread prints this, and only prints it once. */
      printf("\nNote: optimized NTLM8 kernel will be used for false alarm checks.\n\n"); fflush(stdout);
      printed_false_alarm_ntlm8_message = 1;
    }
  }

  /* Load the kernel. */
  gpu->context = CLCREATECONTEXT(context_callback, &(gpu->device));
  gpu->queue = CLCREATEQUEUE(gpu->context, gpu->device);
  load_kernel(gpu->context, 1, &(gpu->device), kernel_path, kernel_name, &(gpu->program), &(gpu->kernel), args->hash_type);

  /* These variables are set so the CLCREATEARG* macros work correctly. */
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
  } else {
    /*gws = kernel_work_group_size * kernel_preferred_work_group_size_multiple;*/

    /* TODO: fix this so that false alarm checking is done in partitions instead of
     * all at once (this can improve speed).  Currently, when GWS != num_start_indices,
     * lookups don't succeed due to some bug. */
    gws = num_start_indices;

    /* Somehow, on AMD GPUs, the kernel crashes with a message like:
     *
     *   Memory access fault by GPU node-2 (Agent handle: 0x1bb5e00) on address
     *   0x7f4c80b27000. Reason: Page not present or supervisor privilege.
     *
     * A work-around is to set the GWS to the number of start indices and just do it in
     * one pass. */
    if (is_amd_gpu)
      gws = num_start_indices;

    /*printf("GPU #%u is using dynamic GWS: %"PRIu64" (work group) x %"PRIu64" (pref. multiple) = %"PRIu64"\n", gpu->device_number, kernel_work_group_size, kernel_preferred_work_group_size_multiple, gws);*/
  }
  fflush(stdout);


  /* Count the number of times we need to run the kernel. */
  num_exec_blocks = num_start_indices / gws;
  if (num_start_indices % gws != 0)
    num_exec_blocks++;

  output_block_len = gws;
  output_block = calloc(output_block_len, sizeof(cl_ulong));
  if (output_block == NULL) {
    fprintf(stderr, "Error while allocating output buffer(s).\n");
    exit(-1);
  }

  CLCREATEARG(0, hash_type_buffer, CL_RO, args->hash_type, sizeof(cl_uint));
  CLCREATEARG_ARRAY(1, charset_buffer, CL_RO, args->charset, strlen(args->charset) + 1);
  CLCREATEARG(2, plaintext_len_min_buffer, CL_RO, args->plaintext_len_min, sizeof(cl_uint));
  CLCREATEARG(3, plaintext_len_max_buffer, CL_RO, args->plaintext_len_max, sizeof(cl_uint));
  CLCREATEARG(4, reduction_offset_buffer, CL_RO, args->reduction_offset, sizeof(cl_uint));
  CLCREATEARG(5, plaintext_space_total_buffer, CL_RO, plaintext_space_total, sizeof(cl_ulong));
  CLCREATEARG_ARRAY(6, plaintext_space_up_to_index_buffer, CL_RO, plaintext_space_up_to_index, MAX_PLAINTEXT_LEN * sizeof(cl_ulong));
  CLCREATEARG(7, device_num_buffer, CL_RO, gpu->device_number, sizeof(cl_uint));
  CLCREATEARG(8, total_devices_buffer, CL_RO, args->total_devices, sizeof(cl_uint));
  CLCREATEARG(9, num_start_indices_buffer, CL_RO, num_start_indices, sizeof(cl_uint));
  CLCREATEARG_ARRAY(10, start_indices_buffer, CL_RO, start_indices, num_start_indices * sizeof(cl_ulong));
  CLCREATEARG_ARRAY(11, start_index_positions_buffer, CL_RO, start_index_positions, num_start_index_positions * sizeof(unsigned int));
  CLCREATEARG_ARRAY(12, hash_base_indices_buffer, CL_RO, hash_base_indices, num_hash_base_indices * sizeof(cl_ulong));
  CLCREATEARG_ARRAY(14, output_block_buffer, CL_WO, output_block, output_block_len * sizeof(cl_ulong));

  for (exec_block = 0; exec_block < num_exec_blocks; exec_block++) {
    unsigned int exec_block_scaler = exec_block * gws;

    CLCREATEARG(13, exec_block_scaler_buffer, CL_RO, exec_block_scaler, sizeof(cl_uint));

    if (is_amd_gpu) {
      int barrier_ret = pthread_barrier_wait(&barrier);
      if ((barrier_ret != 0) && (barrier_ret != PTHREAD_BARRIER_SERIAL_THREAD)) {
	fprintf(stderr, "pthread_barrier_wait() failed!\n"); fflush(stderr);
	exit(-1);
      }
    }

    /* Run the kernel and wait for it to finish. */
    CLRUNKERNEL(gpu->queue, gpu->kernel, &gws);
    CLFLUSH(gpu->queue);
    CLWAIT(gpu->queue);

    /* Read the results. */
    CLREADBUFFER(output_block_buffer, output_block_len * sizeof(cl_ulong), output_block);

    output_block_index = 0;
    while ((plaintext_indicies_index < num_plaintext_indices) && (output_block_index < output_block_len))
      plaintext_indices[plaintext_indicies_index++] = output_block[output_block_index++];

    CLFREEBUFFER(exec_block_scaler_buffer);
  }

  /* Set the results so the main thread can access them. */
  args->results = plaintext_indices;
  args->num_results = num_plaintext_indices;

  /*
  {
    unsigned int i = 0;

    printf("results: ");
    for (i = 0; i < args->num_results; i++)
      printf("%lu ", args->results[i]);

    printf("\n");
  }
  */

  FREE(output_block);

  CLFREEBUFFER(hash_type_buffer);
  CLFREEBUFFER(charset_buffer);
  CLFREEBUFFER(plaintext_len_min_buffer);
  CLFREEBUFFER(plaintext_len_max_buffer);
  CLFREEBUFFER(reduction_offset_buffer);
  CLFREEBUFFER(plaintext_space_total_buffer);
  CLFREEBUFFER(plaintext_space_up_to_index_buffer);
  CLFREEBUFFER(device_num_buffer);
  CLFREEBUFFER(total_devices_buffer);
  CLFREEBUFFER(num_start_indices_buffer);
  CLFREEBUFFER(start_indices_buffer);
  CLFREEBUFFER(start_index_positions_buffer);
  CLFREEBUFFER(hash_base_indices_buffer);
  CLFREEBUFFER(output_block_buffer);

  CLRELEASEKERNEL(gpu->kernel);
  CLRELEASEPROGRAM(gpu->program);
  CLRELEASEQUEUE(gpu->queue);
  CLRELEASECONTEXT(gpu->context);

  pthread_exit(NULL);
  return NULL;
}


/* A host thread which controls each GPU for hash pre-computation. */
void *host_thread_precompute(void *ptr) {
  thread_args *args = (thread_args *)ptr;
  gpu_dev *gpu = &(args->gpu);
  cl_context context = NULL;
  cl_command_queue queue = NULL;
  cl_kernel kernel = NULL;
  int err = 0;
  char *kernel_path = PRECOMPUTE_KERNEL_PATH, *kernel_name = "precompute";

  cl_mem hash_type_buffer = NULL, hash_buffer = NULL, hash_len_buffer = NULL, charset_buffer = NULL, plaintext_len_min_buffer = NULL, plaintext_len_max_buffer = NULL, table_index_buffer = NULL, chain_len_buffer = NULL, device_num_buffer = NULL, total_devices_buffer = NULL, exec_block_scaler_buffer = NULL, output_block_buffer = NULL/*, debug_buffer = NULL*/;

  size_t gws = 0;
  cl_ulong *output = NULL, *output_block = NULL;
  unsigned int output_len = 0, output_block_len = 0, num_exec_blocks = 0, exec_block = 0, output_index = 0, output_block_index = 0;
  /*unsigned int i = 0;*/

  unsigned char hash_binary[32] = {0};
  cl_uint hash_binary_len = 0;


  /* Convert the hash from a hex string to bytes.*/
  hash_binary_len = hex_to_bytes(args->hash, sizeof(hash_binary), hash_binary);

  /* The work size is the chain length divided among the total number of GPUs.  Round
   * up if it doesn't divide evenly; this results in slightly more work being done in
   * order to get complete coverage. */
  output_len = args->chain_len / args->total_devices;
  if ((args->chain_len % args->total_devices) != 0)
    output_len++;

  /* If we're generating the standard NTLM 8-character tables, use the special
   * optimized kernel instead! */
  if (is_ntlm8(args->hash_type, args->charset, args->plaintext_len_min, args->plaintext_len_max, args->reduction_offset, args->chain_len)) {
    kernel_path = PRECOMPUTE_NTLM8_KERNEL_PATH;
    kernel_name = "precompute_ntlm8";
    if ((args->gpu.device_number == 0) && (printed_precompute_ntlm8_message == 0)) { /* Only the first thread prints this, and only prints it once. */
      printf("\nNote: optimized NTLM8 kernel will be used for precomputation.\n\n"); fflush(stdout);
      printed_precompute_ntlm8_message = 1;
    }
  }

  /* Load the kernel. */
  gpu->context = CLCREATECONTEXT(context_callback, &(gpu->device));
  gpu->queue = CLCREATEQUEUE(gpu->context, gpu->device);
  load_kernel(gpu->context, 1, &(gpu->device), kernel_path, kernel_name, &(gpu->program), &(gpu->kernel), args->hash_type);

  /* These variables are set so the CLCREATEARG* macros work correctly. */
  context = gpu->context;
  queue = gpu->queue;
  kernel = gpu->kernel;

  if (rc_clGetKernelWorkGroupInfo(kernel, gpu->device, CL_KERNEL_WORK_GROUP_SIZE /*CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE*/, sizeof(size_t), &gws, NULL) != CL_SUCCESS) {
    fprintf(stderr, "Failed to get preferred work group size!\n");
    CLRELEASEKERNEL(gpu->kernel);
    CLRELEASEPROGRAM(gpu->program);
    CLRELEASEQUEUE(gpu->queue);
    CLRELEASECONTEXT(gpu->context);
    pthread_exit(NULL);
    return NULL;
  }
  gws = gws * gpu->num_work_units;

  /* In the event that the global work size is larger than the number of outputs we
   * need, cap the GWS. */
  if (gws > output_len) gws = output_len;

  /* Count the number of times we need to run the kernel. */
  num_exec_blocks = output_len / gws;
  if (output_len % gws != 0)
    num_exec_blocks++;

  /*printf("Host thread #%u started; GWS: %zu.\n", gpu->device_number, gws);*/

  /* This will hold the results from this one GPU. */
  output = calloc(output_len, sizeof(cl_ulong));

  /* Holds the results from one kernel exec. */
  output_block_len = gws;
  output_block = calloc(output_block_len, sizeof(cl_ulong));

  if ((output == NULL) || (output_block == NULL)) {
    fprintf(stderr, "Error while allocating output buffer(s).\n");
    exit(-1);
  }

  /* Get the number of compute units in this device. */
  /*get_device_uint(gpu->device, CL_DEVICE_MAX_COMPUTE_UNITS, &(gpu->num_work_units));*/


  CLCREATEARG(0, hash_type_buffer, CL_RO, args->hash_type, sizeof(cl_uint));
  CLCREATEARG_ARRAY(1, hash_buffer, CL_RO, hash_binary, hash_binary_len);
  CLCREATEARG(2, hash_len_buffer, CL_RO, hash_binary_len, sizeof(cl_uint));
  CLCREATEARG_ARRAY(3, charset_buffer, CL_RO, args->charset, strlen(args->charset) + 1);
  CLCREATEARG(4, plaintext_len_min_buffer, CL_RO, args->plaintext_len_min, sizeof(cl_uint));
  CLCREATEARG(5, plaintext_len_max_buffer, CL_RO, args->plaintext_len_max, sizeof(cl_uint));
  CLCREATEARG(6, table_index_buffer, CL_RO, args->table_index, sizeof(cl_uint));
  CLCREATEARG(7, chain_len_buffer, CL_RO, args->chain_len, sizeof(cl_uint));
  CLCREATEARG(8, device_num_buffer, CL_RO, gpu->device_number, sizeof(cl_uint));
  CLCREATEARG(9, total_devices_buffer, CL_RO, args->total_devices, sizeof(cl_uint));
  CLCREATEARG_ARRAY(11, output_block_buffer, CL_WO, output_block, output_block_len * sizeof(cl_ulong));
  /*CLCREATEARG_DEBUG(9, debug_buffer, debug_ptr);*/

  for (exec_block = 0; exec_block < num_exec_blocks; exec_block++) {
    unsigned int exec_block_scaler = exec_block * gws;


    CLCREATEARG(10, exec_block_scaler_buffer, CL_RO, exec_block_scaler, sizeof(cl_uint));

    if (is_amd_gpu) {
      int barrier_ret = pthread_barrier_wait(&barrier);
      if ((barrier_ret != 0) && (barrier_ret != PTHREAD_BARRIER_SERIAL_THREAD)) {
	fprintf(stderr, "pthread_barrier_wait() failed!\n"); fflush(stderr);
	exit(-1);
      }
    }

    /* Run the kernel and wait for it to finish. */
    CLRUNKERNEL(gpu->queue, gpu->kernel, &gws);
    CLFLUSH(gpu->queue);
    CLWAIT(gpu->queue);

    /* Read the results. */
    CLREADBUFFER(output_block_buffer, output_block_len * sizeof(cl_ulong), output_block);

    /* Append this block out output to the total output for this GPU. */
    output_block_index = 0;
    while ((output_index < output_len) && (output_block_index < output_block_len))
      output[output_index++] = output_block[output_block_index++];

    CLFREEBUFFER(exec_block_scaler_buffer);
  }

  /* Set the results so the main thread can access them. */
  args->results = output;
  args->num_results = output_len;

  /*
  printf("GPU %u: ", gpu->device_number);
  for (i = 0; i < output_len; i++) {
    printf("%"PRIu64" ", output[i]);
  }
  printf("\n");
  */

  FREE(output_block);

  CLFREEBUFFER(hash_type_buffer);
  CLFREEBUFFER(hash_buffer);
  CLFREEBUFFER(hash_len_buffer);
  CLFREEBUFFER(charset_buffer);
  CLFREEBUFFER(plaintext_len_min_buffer);
  CLFREEBUFFER(plaintext_len_max_buffer);
  CLFREEBUFFER(table_index_buffer);
  CLFREEBUFFER(chain_len_buffer);
  CLFREEBUFFER(device_num_buffer);
  CLFREEBUFFER(total_devices_buffer);
  CLFREEBUFFER(exec_block_scaler_buffer);
  CLFREEBUFFER(output_block_buffer);
  /*CLFREEBUFFER(debug_buffer);*/

  CLRELEASEKERNEL(gpu->kernel);
  CLRELEASEPROGRAM(gpu->program);
  CLRELEASEQUEUE(gpu->queue);
  CLRELEASECONTEXT(gpu->context);

  pthread_exit(NULL);
  return NULL;
}


void precompute_hash(unsigned int num_devices, thread_args *args, precomputed_and_potential_indices **ppi_head) {
  pthread_t threads[MAX_NUM_DEVICES] = {0};
  char filename[128] = {0}, time_str[128] = {0}, index_data[256] = {0};
  struct timespec start_time = {0};
  unsigned int i = 0, j = 0, output_index = 0;
  int k = 0;
  uint64_t *output = NULL;
  FILE *f = NULL;
  precomputed_and_potential_indices *ppi = NULL;


  /* Set the index data we're looking for (or will create later). */
  snprintf(index_data, sizeof(index_data) - 1, "%s_%s#%d-%d_%d_%d:%s\n", args->hash_name, args->charset_name, args->plaintext_len_min, args->plaintext_len_max, args->table_index, args->chain_len, args->hash); /*ntlm_loweralpha#8-8_0_100:49e5bfaab1be72a6c5236f15736a3e15*/

  /* Search through the cache and see if we already precomputed the indices for this
   * hash. */
  output = search_precompute_cache(index_data, &output_index, filename, sizeof(filename));

  /* Cache miss... */
  if (output == NULL) {
  
    /* Start the timer for this hash. */
    start_timer(&start_time);

    /* Start one thread to control each GPU. */
    for (i = 0; i < num_devices; i++) {
      if (pthread_create(&(threads[i]), NULL, &host_thread_precompute, &(args[i]))) {
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

    seconds_to_human_time(time_str, sizeof(time_str), get_elapsed(&start_time));

    printf("  Completed in %s.\n", time_str);  fflush(stdout);

    /* Create one output array to hold all the results. */
    output = calloc(args[0].num_results * num_devices, sizeof(uint64_t));
    if (output == NULL) {
      fprintf(stderr, "Error allocating buffer for GPU results.\n");
      exit(-1);
    }

    /*
      The results end up spread out like this across many GPUs:

      GPU 0: 100 94 88 82 76 70 64 58 52 46 40 34 28 22 16 10 4 
      GPU 1: 99 93 87 81 75 69 63 57 51 45 39 33 27 21 15 9 3 
      GPU 2: 98 92 86 80 74 68 62 56 50 44 38 32 26 20 14 8 2 
      GPU 3: 97 91 85 79 73 67 61 55 49 43 37 31 25 19 13 7 1 
      GPU 4: 96 90 84 78 72 66 60 54 48 42 36 30 24 18 12 6 0 
      GPU 5: 95 89 83 77 71 65 59 53 47 41 35 29 23 17 11 5 0 

      Below, we collate the results into a single array containing "100 99 98 [...]".
    */
    for (i = 0; i < args[0].num_results; i++) {
      for (j = 0; j < num_devices; j++) {
	output[output_index] = args[j].results[i];
	output_index++;
      }
    }

    /* Now that pulled all the GPU results into one array, free them. */
    for (i = 0; i < num_devices; i++) {
      FREE(args[i].results);
      args[i].num_results = 0;
    }

    /* We may have a few extra indices in the array at the end, if the chain length
     * is not divisible by the number of GPUs.  In that case, we simply truncate the
     * end of the array. */
    if (output_index >= args[0].chain_len - 1)
      output_index = args[0].chain_len -1;
    else { /* Sanity check: this should never happen... */
      fprintf(stderr, "Error: output_index < chain_len - 1!: %u < %u\n", output_index, args[0].chain_len - 1);
      exit(-1);
    }

    /* Reverse the output buffer.
     * TODO: this logic can be merged in, above, to simplify. */
    {
      uint64_t *tmp = calloc(output_index, sizeof(uint64_t));
      if (tmp == NULL) {
	fprintf(stderr, "Failed to create temp buffer.\n");
	exit(-1);
      }

      for (i = 0; i < output_index; i++)
	tmp[i] = output[output_index - i - 1];

      FREE(output);
      output = tmp;
    }

    /* Search for the first unused filename in the space of rcracki.precalc.[0-1048576]. */
    for (i = 0; i < 1048576; i++) {
      int fd = -1;

      snprintf(filename, sizeof(filename) - 1, "rcracki.precalc.%d", i);

      /* Create a file for writing with permissions of 0600. */
      fd = open(filename, O_CREAT | O_EXCL | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);

      if (fd != -1) { /* On success, convert to a file pointer. */
	f = fdopen(fd, "wb");
	break;
      }
    }

    if (f == NULL) {
      fprintf(stderr, "Error: could not create any precalc file (rcracki.precalc.[0-1048576])\n");
      exit(-1);
    }

    /* Ok, so it turns out that we generated the array backwards.  Oh well.  We will
     * just iterate backwards here to compensate. */
    /*for (k = output_index - 1; k >= 0; k--)
      fwrite(&(output[k]), sizeof(cl_ulong), 1, f);*/

    for (k = 0; k < output_index; k++)
      fwrite(&(output[k]), sizeof(cl_ulong), 1, f);

    FCLOSE(f);

    /* Now create the rcracki.precalc.?.index file. */
    strncat(filename, ".index", sizeof(filename) - 1);
    f = fopen(filename, "wb");
    if (f == NULL) {
      fprintf(stderr, "Error while creating file: %s\n", filename);
      exit(-1);
    } else {
      fwrite(index_data, sizeof(char), strlen(index_data), f);
      FCLOSE(f);
    }

  } else {
    printf("Using cached pre-computed indices for hash %s.\n", args->hash);  fflush(stdout);
  }

  total_precomputed_indices_loaded += output_index;

  /*
  printf("output_index: %u\nFinal array: ", output_index);

  for (i = 0; i < output_index; i++)
    printf("%"PRIu64" ", output[i]);
  printf("\n");

  printf("\nFinal array hex: ");

  for (i = 0; i < output_index; i++)
    printf("%08"PRIx64" ", output[i]);
  printf("\n");
  */

  /* Time to store the precomputed indices.  If no head exists in the linked list... */
  if (*ppi_head == NULL) {
    *ppi_head = calloc(1, sizeof(precomputed_and_potential_indices));
    if (*ppi_head == NULL) {
      fprintf(stderr, "Error allocating buffer for precomputed indices.\n");
      exit(-1);
    }
    ppi = *ppi_head;
  } else {
    ppi = *ppi_head;
    while (ppi->next != NULL)
      ppi = ppi->next;
    ppi->next = calloc(1, sizeof(precomputed_and_potential_indices));
    if (ppi->next == NULL) {
      fprintf(stderr, "Error allocating buffer for precomputed indices.\n");
      exit(-1);
    }
    ppi = ppi->next;
  }

  ppi->hash = args->hash;
  ppi->num_precomputed_end_indices = output_index;

  ppi->precomputed_end_indices = calloc(ppi->num_precomputed_end_indices, sizeof(cl_ulong));
  if (ppi->precomputed_end_indices == NULL) {
    fprintf(stderr, "Error allocating index buffer for precomputed indices.\n");
    exit(-1);
  }

  /* Store the precomputed indices into the array. */
  for (i = 0; i < ppi->num_precomputed_end_indices; i++)
    ppi->precomputed_end_indices[i] = output[i];

  /* Set the filename, so it can be deleted if the hash is cracked later. */
  ppi->index_filename = strdup(filename);

  FREE(output);
}


void _preloading_thread(char *rt_dir) {
  DIR *dir = NULL;
  struct dirent *de = NULL;
  struct stat st;
  char filepath[512];


  memset(&st, 0, sizeof(st));
  memset(filepath, 0, sizeof(filepath));

  dir = opendir(rt_dir);
  if (dir == NULL)  /* This directory may not allow the current process permission. */
    return;

  while ((de = readdir(dir)) != NULL) {

    /* Create an absolute path to this entity. */
    filepath_join(filepath, sizeof(filepath), rt_dir, de->d_name);

    /* If this is a directory, recurse into it. */
    if ((strcmp(de->d_name, ".") != 0) && (strcmp(de->d_name, "..") != 0) && (stat(filepath, &st) == 0) && S_ISDIR(st.st_mode)) {
      _preloading_thread(filepath);

    /* If this is a compressed or uncompressed rainbow table, load it! */
    } else if (str_ends_with(de->d_name, ".rt") || str_ends_with(de->d_name, ".rtc")) {
      cl_ulong *rainbow_table = NULL;
      unsigned int num_chains = 0, is_uncompressed_table = 0;
      struct timespec start_time_io = {0};


      if (str_ends_with(de->d_name, ".rtc")) {
	int ret = 0;

	start_timer(&start_time_io);    /* For loading the table only. */
	if ((ret = rtc_decompress(filepath, &rainbow_table, &num_chains)) != 0) {
	  fprintf(stderr, "Error while decompressing RTC table %s: %d\n", filepath, ret);
	  exit(-1);
	}
	time_io += get_elapsed(&start_time_io);
      } else {
	FILE *f = NULL;

	is_uncompressed_table = 1;
	start_timer(&start_time_io);    /* For loading the table only. */
	f = fopen(filepath, "rb");
	if (f != NULL) {
	  long file_size = get_file_size(f);

	  if ((file_size % (sizeof(cl_ulong) * 2) == 0) && (file_size > 0)) {
	    unsigned int num_longs = file_size / sizeof(cl_ulong);

	    rainbow_table = calloc(num_longs, sizeof(cl_ulong));
	    if (rainbow_table == NULL) {
	      fprintf(stderr, "Failed to allocate %"PRIu64" bytes for rainbow table!: %s\n", num_longs * sizeof(cl_ulong), filepath);
	      exit(-1);
	    }

	    if (fread(rainbow_table, sizeof(cl_ulong), num_longs, f) != num_longs) {
	      fprintf(stderr, "Error while reading rainbow table: %s\n", strerror(errno));
	      exit(-1);
	    }

	    time_io += get_elapsed(&start_time_io);
	    num_chains = num_longs / 2;
	  } else
	    fprintf(stderr, "Rainbow table size is not a multiple of %"PRIu64": %ld\n", sizeof(cl_ulong) * 2, file_size);

	  FCLOSE(f);
	} else
	  fprintf(stderr, "Could not open file for reading: %s", strerror(errno));
      }

      if (rainbow_table != NULL) {
	unsigned int skip_table = 0;


	/* If the table is uncompressed (*.rt), then there's a possibility its unsorted on accident.  We will
	 * verify them first to make sure. */
	if (is_uncompressed_table == 1) {
	  if (!verify_rainbowtable(rainbow_table, num_chains, VERIFY_TABLE_TYPE_LOOKUP, 0, 0, NULL)) {
	    fprintf(stderr, "\nError: %s is not a valid table suitable for lookups!  (Hint: it may not be sorted.)  Skipping...\n\n", filepath);  fflush(stderr);
	    FREE(rainbow_table);
	    skip_table = 1; /* Skip further processing on this table only. */
	  }
	}

	if (!skip_table) {
	  preloaded_table *pt = calloc(1, sizeof(preloaded_table));
	  if (pt == NULL) {
	    printf("Failed to allocate memory for preload_table.\n");
	    exit(-1);
	  }

	  /* Set the file path, rainbow table, and number of chains in the newest entry of the preload list. */
	  pt->filepath = strdup(filepath);
	  pt->rainbow_table = rainbow_table;
	  pt->num_chains = num_chains;

	  /* Lock the preloading system, since we're modifying shared structures. */
	  pthread_mutex_lock(&preloaded_tables_lock);

	  /* Increase the counter of preloaded tables. */
	  num_preloaded_tables_available++;

	  /* If the list is empty, add the newest entry as the head. */
	  if (preloaded_table_list == NULL)
	    preloaded_table_list = pt;
	  else { /* The list isn't empty, so traverse it to the end, and append this entry. */
	    preloaded_table *ptr = preloaded_table_list;
	    while (ptr->next != NULL)
	      ptr = ptr->next;

	    ptr->next = pt;
	  }

	  /* Tell the main thread that we have a table available. */
	  pthread_cond_signal(&condition_wait_for_tables);

	  /* If we preloaded the maximum number of tables, wait for the main thread to consume at least one
	   * before preloading more. */
	  while (num_preloaded_tables_available >= MAX_PRELOAD_NUM)
	    pthread_cond_wait(&condition_continue_loading_tables, &preloaded_tables_lock);

	  /* Release the preloading system lock. */
	  pthread_mutex_unlock(&preloaded_tables_lock);
	}
      }
    }
  }

  closedir(dir); dir = NULL;
}


/* The thread which preloads tables in the background while the main thread performs binary searching & false
 * alarm checks. */
void *preloading_thread(void *ptr) {
  char *xrt_dir = ((preloading_thread_args *)ptr)->rt_dir;
  char rt_dir[512];


  memset(rt_dir, 0, sizeof(rt_dir));

  /* Copy the rainbow table path from the heap to the local stack, then free the source. */
  strncpy(rt_dir, xrt_dir, sizeof(rt_dir) - 1);
  free(xrt_dir); xrt_dir = ((preloading_thread_args *)ptr)->rt_dir = NULL;

  _preloading_thread(rt_dir);

  /* We've reached the end of all the tables, so tell the main thread. */
  table_loading_complete = 1;

  /* If the main thread is still waiting on new tables, wake it up. */
  pthread_mutex_lock(&preloaded_tables_lock);
  pthread_cond_signal(&condition_wait_for_tables);
  pthread_mutex_unlock(&preloaded_tables_lock);
  return NULL;
}


/* Given the number of tables processed out of the total, prints the estimated time left to
 * completion. */
void print_eta(unsigned int num_tables_processed, unsigned int num_tables_total) {
  double seconds_per_table = (double)(get_elapsed(&search_start_time) / (double)num_tables_processed);
  unsigned int num_tables_left = num_tables_total - num_tables_processed;
  unsigned int num_seconds_left = num_tables_left * seconds_per_table;
  char eta_str[64] = {0};


  seconds_to_human_time(eta_str, sizeof(eta_str), num_seconds_left);
  printf("  Estimated time remaining (at most): %s\n\n", eta_str); fflush(stdout);
}


void print_usage_and_exit(char *prog_name, int exit_code) {
#ifdef _WIN32
  fprintf(stderr, "Usage: %s rainbow_table_directory (single_hash | filename_with_many_hashes.txt)\n\nExample:\n    %s D:\\rt_ntlm\\ 64f12cddaa88057e06a81b54e73b949b\n    %s D:\\rt_ntlm\\ C:\\Users\\jsmith\\Desktop\\hashes.txt [-gws GWS]\n\n", prog_name, prog_name, prog_name);
#else
  fprintf(stderr, "Usage: %s rainbow_table_directory (single_hash | filename_with_many_hashes.txt)\n\nExample:\n    %s /export/rt_ntlm/ 64f12cddaa88057e06a81b54e73b949b\n    %s /export/rt_ntlm/ /home/user/hashes.txt [-gws GWS]\n\n", prog_name, prog_name, prog_name);
#endif
  exit(exit_code);
}


/* Helper function for rt_binary_search(). */
unsigned int _rt_binary_search(cl_ulong *rainbow_table, unsigned int low, unsigned int high, cl_ulong search_index, cl_ulong *start) {
  unsigned int chain = 0;


  /*printf("_rt_binary_search(%u, %u, %lu)\n", low, high, search_index);*/
  if (high - low <= 8) {
    for (chain = low; chain < high; chain++) {
      if (search_index == rainbow_table[(chain * 2) + 1]) {
	*start = rainbow_table[chain * 2];
	/*printf("\nbinary search: found %lu at %u (between %u and %u)\n", *start, chain, low, high);*/
	return 1;
      }
    }
  } else {
    chain = ((high - low) / 2) + low;
    if (search_index >= rainbow_table[(chain * 2) + 1])
      return _rt_binary_search(rainbow_table, chain, high, search_index, start);
    else
      return _rt_binary_search(rainbow_table, low, chain, search_index, start);
  }

  return 0;
}


void *rt_binary_search_thread(void *ptr) {
  search_thread_args *args = (search_thread_args *)ptr;
  precomputed_and_potential_indices *ppi_cur = args->ppi_head;
  unsigned int i = 0;
  cl_ulong start = 0;


  while (ppi_cur != NULL) {
    if (ppi_cur->plaintext == NULL) { /* If this hash isn't cracked yet... */
      for (i = 0 + args->thread_number; i < ppi_cur->num_precomputed_end_indices; i += args->total_threads) {
	if (_rt_binary_search(args->rainbow_table, 0, args->num_chains, ppi_cur->precomputed_end_indices[i], &start)) {
	  add_potential_start_index_and_position(ppi_cur, start, i);
	}
      }
    }
    ppi_cur = ppi_cur->next;
  }

  pthread_exit(NULL);
  return NULL;
}


/* Rainbow table binary search.  Searches a table's end indices for any matches with
 * precomputed end indices.  If/when matches are found, the corresponding start indices
 * are added to the precomputed_and_potential_indices's potential_start_indices
 * array. */
void rt_binary_search(cl_ulong *rainbow_table, unsigned int num_chains, precomputed_and_potential_indices *ppi_head) {
  struct timespec start_time_searching = {0};
  char time_searching_str[64] = {0};
  unsigned int num_threads = get_num_cpu_cores();
  pthread_t *threads = NULL;
  search_thread_args *args = NULL;
  unsigned int i = 0;
  double s_time = 0;


  start_timer(&start_time_searching);
  args = calloc(num_threads, sizeof(search_thread_args));
  threads = calloc(num_threads, sizeof(pthread_t));
  if ((args == NULL) || (threads == NULL)) {
    fprintf(stderr, "Failed to create thread/args for searching.\n");
    exit(-1);
  }

  printf("  Searching table for matching endpoints...\n");  fflush(stdout);

  for (i = 0; i < num_threads; i++) {
    args[i].thread_number = i;
    args[i].total_threads = num_threads;
    args[i].rainbow_table = rainbow_table;
    args[i].num_chains = num_chains;
    args[i].ppi_head = ppi_head;

    if (pthread_create(&(threads[i]), NULL, &rt_binary_search_thread, &(args[i]))) {
      perror("Failed to create thread");
      exit(-1);
    }
  }

  /* Wait for all threads to finish. */
  for (i = 0; i < num_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("Failed to join with thread");
      exit(-1);
    }
  }

  s_time = get_elapsed(&start_time_searching);
  seconds_to_human_time(time_searching_str, sizeof(time_searching_str), s_time);
  printf("  Table searched in %s.\n", time_searching_str);  fflush(stdout);

  time_searching += s_time;
  FREE(args);
  FREE(threads);
}


void save_cracked_hash(precomputed_and_potential_indices *ppi, unsigned int hash_type) {
  FILE *jtr_file = fopen(jtr_pot_filename, "ab"), *hashcat_file = fopen(hashcat_pot_filename, "ab");
  unsigned int hash_len = strlen(ppi->hash), plaintext_len = strlen(ppi->plaintext);
  char *dot_pos = strrchr(ppi->index_filename, '.');


  if (jtr_file == NULL) {
    fprintf(stderr, "Error: could not open pot file for writing: %s: %s\n", jtr_pot_filename, strerror(errno));
    exit(-1);
  } else if (hashcat_file == NULL) {
    fprintf(stderr, "Error: could not open pot file for writing: %s: %s\n", hashcat_pot_filename, strerror(errno));
    exit(-1);
  }

  /* The JTR pot file format requires NTLM hashes to be prepended with "$NT$". */
  if ((hash_type == HASH_NTLM) && (fwrite("$NT$", sizeof(char), 4, jtr_file) != 4)) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  }

  if (fwrite(ppi->hash, sizeof(char), hash_len, jtr_file) != hash_len) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  } else if (fwrite(ppi->hash, sizeof(char), hash_len, hashcat_file) != hash_len) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  }

  if (fwrite(":", sizeof(char), 1, jtr_file) != 1) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  } else if (fwrite(":", sizeof(char), 1, hashcat_file) != 1) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  }

  if (fwrite(ppi->plaintext, sizeof(char), plaintext_len, jtr_file) != plaintext_len) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  } else if (fwrite(ppi->plaintext, sizeof(char), plaintext_len, hashcat_file) != plaintext_len) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  }

  if (fwrite("\n", sizeof(char), 1, jtr_file) != 1) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  } else if (fwrite("\n", sizeof(char), 1, hashcat_file) != 1) {
    fprintf(stderr, "Error while writing to pot file: %s\n", strerror(errno));
    exit(-1);
  }

  FCLOSE(jtr_file);
  FCLOSE(hashcat_file);

  /* Delete the index file containing information about the precomputed indices.  Since
   * this hash was cracked, this is no longer needed. */
  if (unlink(ppi->index_filename) != 0) {
    fprintf(stderr, "Error while deleting precompute index file: %s: %s\n", ppi->index_filename, strerror(errno));
    /*exit(-1);*/
  }

  /* Truncate the ".index" off the end of the filename; this forms the precomputation
   * filename. */
  *dot_pos = '\0';
  if (unlink(ppi->index_filename) != 0) {
    fprintf(stderr, "Error while deleting precompute file: %s: %s\n", ppi->index_filename, strerror(errno));
    /*exit(-1);*/
  }

  num_cracked++;
  num_falsealarms--;
}


/* Searches the precompute cache for matching index data.  If found, an array of
 * indices is returned, num_indices set to the array size, and the filename buffer
 * is set to the *.index cache file. */
cl_ulong *search_precompute_cache(char *index_data, unsigned int *num_indices, char *filename, unsigned int filename_size) {
  char buf[256] = {0};
  int file_size = 0;
  DIR *d = NULL;
  struct dirent *de = NULL;
  FILE *f = NULL;
  cl_ulong *ret = NULL;


  *num_indices = 0;
  memset(filename, 0, filename_size);


  /* Go through all *.index files in the current directory and find any that match
   * the hash passed to us.  If found, we already pre-computed the values. */
  d = opendir(".");
  if (d == NULL) {
    fprintf(stderr, "Can't open current directory.\n");
    exit(-1);
  }
  while ((de = readdir(d)) != NULL) {
    if (str_ends_with(de->d_name, ".index")) {
      /*printf("Looking at %s\n", de->d_name);*/

      /* Open this *.index file. */
      f = fopen(de->d_name, "rb");
      if (f == NULL) {
	fprintf(stderr, "Failed to open %s for reading.\n", de->d_name);
	exit(-1);
      }

      file_size = get_file_size(f);

      /* Read the index data.*/
      if ((file_size >= sizeof(buf)) || (fread(buf, sizeof(char), file_size, f) != file_size)) {
	fprintf(stderr, "Failed to read index data: %s\n", strerror(errno));
	exit(-1);
      }

      FCLOSE(f);

      /* We found an index file that matches all our parameters.  Open its related
       * file containing precomputed indices. */
      if (strcmp(index_data, buf) == 0) {

	/* Set the filename to the *.index file for the caller. */
	strncpy(filename, de->d_name, filename_size - 1);
	de->d_name[strlen(de->d_name) - 6] = '\0';

	f = fopen(de->d_name, "rb");
	if (f == NULL) {
	  fprintf(stderr, "Failed to open precomputed index file: %s\n", de->d_name);
	  exit(-1);
	}

	file_size = get_file_size(f);

	if (file_size % sizeof(cl_ulong) != 0) {
	  fprintf(stderr, "Precomputed indices file is not a multiple of %"PRIu64": %u\n", sizeof(cl_ulong), file_size);
	  exit(-1);
	}

	*num_indices = file_size / sizeof(cl_ulong);

	ret = calloc(*num_indices, sizeof(cl_ulong));
	if (ret == NULL) {
	  fprintf(stderr, "Failed to create indices buffer.\n");
	  exit(-1);
	}

	if (fread(ret, sizeof(cl_ulong), *num_indices, f) != *num_indices) {
	  fprintf(stderr, "Failed to read indices file.\n");
	  exit(-1);
	}
	FCLOSE(f);

	break;
      }
    }
  }
  closedir(d); d = NULL;  
  return ret;
}


/* Returns a preloaded_table entry, or NULL if no more tables are left to process.  The caller must
 * free it and all member variables. */
preloaded_table *get_preloaded_table() {
  preloaded_table *ret = NULL;

  pthread_mutex_lock(&preloaded_tables_lock);

  /* If no tables have been preloaded yet, wait until at least one becomes available. */
  while ((num_preloaded_tables_available == 0) && (table_loading_complete == 0))
    pthread_cond_wait(&condition_wait_for_tables, &preloaded_tables_lock);

  /* Return the head of the list. */
  ret = preloaded_table_list;

  /* If the head of the list isn't NULL, advance it by one. */
  if (preloaded_table_list != NULL) {
    preloaded_table_list = preloaded_table_list->next;

    if (num_preloaded_tables_available > 0)
      num_preloaded_tables_available--;

    /* Wake up the preloading thread if its waiting because it loaded the max.  Now that we're
     * consuming one table, it can load the next concurrently. */
    pthread_cond_signal(&condition_continue_loading_tables);
  }

  pthread_mutex_unlock(&preloaded_tables_lock);
  return ret;
}


void search_tables(unsigned int total_tables, precomputed_and_potential_indices *ppi, thread_args *args) {
  unsigned int num_uncracked = 0, current_table = 0;
  struct timespec start_time_table = {0};
  precomputed_and_potential_indices *ppi_cur = ppi;
  preloaded_table *pt = NULL;


  while (1) {

    /* Count the number of uncracked hashes we have left. */
    while (ppi_cur != NULL) {
      if (ppi_cur->plaintext == NULL)
	num_uncracked++;

      ppi_cur = ppi_cur->next;
    }

    /* If all the hashes were cracked, there's no need to continue processing
     * tables. */
    if (num_uncracked == 0) {
      printf("All hashes cracked.  Skipping rest of tables.\n");
      break;
    }

    /* Get the next preloaded table.  If NULL, we reached the end. */
    pt = get_preloaded_table();
    if (pt == NULL)
      break;

    current_table++;
    printf("[%u of %u] Processing table: %s...\n", current_table, total_tables, pt->filepath);  fflush(stdout);

    start_timer(&start_time_table);
    rt_binary_search(pt->rainbow_table, pt->num_chains, ppi);

    num_chains_processed += pt->num_chains;
    num_tables_processed++;

    /* Free the preloaded table. */
    FREE(pt->filepath);
    FREE(pt->rainbow_table);
    pt->num_chains = 0;
    FREE(pt);

    /* Check endpoint matches. */
    check_false_alarms(ppi, args);

    printf("  Table fully processed in %.1f seconds.\n", get_elapsed(&start_time_table)); fflush(stdout);
    print_eta(num_tables_processed, total_tables);

    /* We checked the potential matches above, so there's nothing else to do with
     * them. */
    clear_potential_start_indices(ppi);

  }

  /* Free any remaining preloaded tables (i.e.: if we cracked all the hashes and quit early). */
  /* Note: technically, this may not be a complete solution, if this is reached while the preloading
   * thread is still performing work... */
  pthread_mutex_lock(&preloaded_tables_lock);
  while (preloaded_table_list != NULL) {
    preloaded_table *pt_next = preloaded_table_list->next;

    FREE(preloaded_table_list->filepath);
    FREE(preloaded_table_list->rainbow_table);
    preloaded_table_list->num_chains = 0;
    FREE(preloaded_table_list);

    preloaded_table_list = pt_next;
  }
  pthread_mutex_unlock(&preloaded_tables_lock);
}


int main(int ac, char **av) {
  char *rt_dir = NULL, *single_hash = NULL, *filename = NULL, *file_data = NULL, **hashes = NULL, *line = NULL, *pot_file_data = NULL;
  unsigned int i = 0, j = 0, num_hashes = 0, err = 0;
  FILE *f = NULL;
  struct stat st = {0};
  thread_args *args = NULL;
  struct timespec start_time = {0};
  char time_precomp_str[64] = {0}, time_io_str[64] = {0}, time_searching_str[64] = {0}, time_falsealarms_str[64] = {0}, time_total_str[64] = {0}, time_per_table_str[64] = {0};

  rt_parameters rt_params = {0};

  cl_platform_id platforms[MAX_NUM_PLATFORMS] = {0};
  cl_device_id devices[MAX_NUM_DEVICES] = {0};

  cl_uint num_platforms = 0, num_devices = 0;

  precomputed_and_potential_indices *ppi_head = NULL, *ppi_cur = NULL;

  pthread_t preload_thread_id = {0};
  preloading_thread_args preload_thread_args = {0};


  ENABLE_CONSOLE_COLOR();
  PRINT_PROJECT_HEADER();
  setlocale(LC_NUMERIC, "");
  if ((ac < 3) || (ac > 5))
    print_usage_and_exit(av[0], -1);
  else if ((ac == 5) && (strcmp(av[3], "-gws") != 0))
    print_usage_and_exit(av[0], -1);


  /* Initialize the devices. */
  get_platforms_and_devices(MAX_NUM_PLATFORMS, platforms, &num_platforms, MAX_NUM_DEVICES, devices, &num_devices, VERBOSE);

  /* Check the device type and set flags.*/
  if (num_devices > 0) {
    char device_vendor[128] = {0};

    get_device_str(devices[0], CL_DEVICE_VENDOR, device_vendor, sizeof(device_vendor) - 1);
    if (strstr(device_vendor, "Advanced Micro Devices") != NULL)
      is_amd_gpu = 1;
  }

  /* Print a warning on Windows 7 systems, as they are observed to be highly
   * unstable for performing lookups on. */
  PRINT_WIN7_LOOKUP_WARNING();

  /* Check that this system has sufficient RAM. */
  CHECK_MEMORY_SIZE();

  /* Initialize the barrier.  This is used in some cases to ensure kernels across
   * multiple devices run concurrently. */
  if (pthread_barrier_init(&barrier, NULL, num_devices) != 0) {
    fprintf(stderr, "pthread_barrier_init() failed.\n");
    exit(-1);
  }

  printf("Binary searching will be done with %u threads.\n", get_num_cpu_cores());

  /* First arg is the directory (and/or sub-directories) containing rainbow tables. */
  rt_dir = av[1];

  /* The default rainbowcrackalack.pot file can be overridden with a third argument.
   * This is undocumented since its probably only useful for automated testing. */
  if (ac == 4) {
    strncpy(jtr_pot_filename, av[3], sizeof(jtr_pot_filename));
    strncpy(hashcat_pot_filename, av[3], sizeof(hashcat_pot_filename));
    strncat(hashcat_pot_filename, ".hashcat", sizeof(hashcat_pot_filename));
  } else if (ac == 5)
    user_provided_gws = (unsigned int)atoi(av[4]);


  /* Open the JTR pot file for reading.  We will check the hash(es) to see if any are
   * already cracked. */
  f = fopen(jtr_pot_filename, "rb");
  if (f) {
    unsigned long file_size = get_file_size(f);

    pot_file_data = calloc(file_size, sizeof(char));
    if (pot_file_data == NULL) {
      fprintf(stderr, "Failed to allocate buffer for pot file.\n");
      exit(-1);
    }

    if (fread(pot_file_data, sizeof(char), file_size, f) != file_size) {
      fprintf(stderr, "Error reading pot file: %s\n", strerror(errno));
      exit(-1);
    }
  } else {
    /* Allocate an empty string. */
    pot_file_data = calloc(1, sizeof(char));
    if (pot_file_data == NULL) {
      fprintf(stderr, "Failed to allocate buffer for pot file.\n");
      exit(-1);
    }
  }

  FCLOSE(f);

  /* Check if the second arg is a hash or a file containing hashes. */
  if (stat(av[2], &st) == 0)
    filename = av[2];
  else {
    single_hash = av[2];

    /* If this hash is already in the pot file, then there's nothing else to do. */
    if (pot_file_data && strstr(pot_file_data, single_hash)) {
      printf("Specified hash has already been cracked!  Check %s.\n", jtr_pot_filename);
      exit(0);
    }
  }

  if (filename) {
    FILE *f = fopen(filename, "rb");
    unsigned int previously_cracked = 0;


    if (f == NULL) {
      fprintf(stderr, "Error while opening file for reading: %s\n", filename);
      goto err;
    }

    file_data = calloc(st.st_size + 1, sizeof(char));
    if (file_data == NULL) {
      fprintf(stderr, "Error while allocating buffer for hash file.\n");
      goto err;
    }

    if (fread(file_data, sizeof(char), st.st_size, f) != st.st_size) {
      fprintf(stderr, "Error while reading hash file.\n");
      goto err;
    }

    FCLOSE(f);

    /* Count the number of newlines in the file so we know how large to make the
     * hash array. */
    for (i = 0; i < st.st_size; i++) {
      if (file_data[i] == '\n')
	num_hashes++;
    }
    num_hashes++;  /* In case the last line doesn't end with an LF. */

    hashes = calloc(num_hashes, sizeof(char *));
    if (hashes == NULL) {
      fprintf(stderr, "Error while allocating buffer for hashes.\n");
      goto err;
    }

    /* Tokenize the hash file by line.  Store each hash in the array. */
    num_hashes = 0;
    line = strtok(file_data, "\n");
    while (line) {

      /* Skip empty lines.  */
      if (strlen(line) > 0) {

	/* Skip previously-cracked hashes. */
	if (strstr(pot_file_data, line) != NULL)
	  previously_cracked++;
	else {
          /* If we're dealing with CRLF line endings, cut off the trailing CR. */
          if (line[strlen(line) - 1] == '\r')
            line[strlen(line) - 1] = '\0';

          /* Ensure that hash is lowercase. */
          str_to_lowercase(line);

	  hashes[num_hashes] = strdup(line);
	  if (hashes[num_hashes] == NULL) {
	    fprintf(stderr, "Error while allocating buffer for hashes.\n");
	    goto err;
	  }
	  num_hashes++;
	}

	line = strtok(NULL, "\n");
      }
    }

    FREE(file_data);

    if (num_hashes == 0) {
      printf("All hashes have already been cracked!  Check %s.\n", jtr_pot_filename);
      exit(0);
    } else {
      printf("Loaded %u of %u uncracked hashes from %s.\n", num_hashes, num_hashes + previously_cracked, filename);  fflush(stdout);
    }

  } else { /* A single hash was provided. */
    hashes = calloc(1, sizeof(char *));
    if (hashes == NULL) {
      fprintf(stderr, "Error while allocating buffer for hashes.\n");
      goto err;
    }

    /* Ensure that hash is lowercase. */
    str_to_lowercase(single_hash);
    hashes[0] = strdup(single_hash);
    num_hashes = 1;
  }

  /* We're done checking the pot file for previously-cracked hashes. */
  FREE(pot_file_data);

  /* Look through the supplied rainbow table directory, and infer the parameters via
   * the filenames. */
  find_rt_params(rt_dir, &rt_params);
  if (!rt_params.parsed) {
    fprintf(stderr, "Failed to infer rainbow table parameters from files in directory.  Ensure that valid rainbow table files are in %s (and/or its sub-directories).\n", rt_dir);
    exit(-1);
  }

  /* At this time, only NTLM hashes are supported. */
  if (rt_params.hash_type != HASH_NTLM) {
    fprintf(stderr, "Unfortunately, only NTLM hashes are supported at this time.  Terminating.\n");
    exit(-1);
  }

  /* Ensure that valid hashes were provided. */
  if (rt_params.hash_type == HASH_NTLM) {
    for (i = 0; i < num_hashes; i++) {
      if (strlen(hashes[i]) != 32) {
	fprintf(stderr, "Error: invalid NTLM hash (length is not 32!): %s\n", hashes[i]);
	exit(-1);
      }
    }
  }

  /* Issue a warning if more than 5,000 hashes were provided, as rainbow tables may
   * start to become not as efficient as brute-force. */
  if (num_hashes > 5000) {
    printf("\n\n\n\t!! WARNING !!\n\nA large group of hashes was provided (%u).  In general, rainbow tables are only effective to use for small numbers of hashes because there is a pre-computation step that must be done on *each hash*; eventually this pre-computation cost becomes high enough that brute-force would be a better strategy.  The point at which this happens depends on your specific GPU hardware.\n\nFor example, suppose the pre-computation step takes 2.8 seconds per hash, and brute-forcing takes 16 hours (57,600 seconds).  Not counting search time nor false alarm checking, the point at which brute-forcing becomes more efficient than rainbow tables is: 57,600 / 2.8 = ~20,571 hashes.  Trying to crack more than this number of hashes is clearly less effective than brute-force.\n\nPay attention to the pre-computation times below, and compare with the reported estimate that hashcat gives after a few minutes for brute-forcing 8-character NTLM (hint: ./hashcat -m 1000 -a 3 -w 3 -O ffffffffffffffffffffffffffffffff ?a?a?a?a?a?a?a?a).\n\n\n\n", num_hashes);  fflush(stdout);
  }

  args = calloc(num_devices, sizeof(thread_args));
  if (args == NULL) {
    fprintf(stderr, "Error while creating thread arg array.\n");
    goto err;
  }

  /* We set most of the args once, since all GPUs & hashes need all the same
   * parameters. */
  for (i = 0; i < num_devices; i++) {
    args[i].hash_type = rt_params.hash_type;
    args[i].hash_name = rt_params.hash_name;
    args[i].hash = NULL;  /* Filled in below. */
    args[i].charset = validate_charset(rt_params.charset_name);
    args[i].charset_name = rt_params.charset_name;
    args[i].plaintext_len_min = rt_params.plaintext_len_min;
    args[i].plaintext_len_max = rt_params.plaintext_len_max;
    args[i].table_index = rt_params.table_index;
    args[i].reduction_offset = rt_params.reduction_offset;
    args[i].chain_len = rt_params.chain_len;
    args[i].total_devices = num_devices;
    args[i].gpu.device_number = i;
    args[i].gpu.device = devices[i];
    get_device_uint(args[i].gpu.device, CL_DEVICE_MAX_COMPUTE_UNITS, &(args[i].gpu.num_work_units));
  }

  start_timer(&start_time);
  for (i = 0; i < num_hashes; i++) {
    printf("Pre-computing hash #%u: %s...\n", i + 1, hashes[i]);  fflush(stdout);

    for (j = 0; j < num_devices; j++)
      args[j].hash = hashes[i];

    precompute_hash(num_devices, args, &ppi_head);
  }
  time_precomp = get_elapsed(&start_time);
  seconds_to_human_time(time_precomp_str, sizeof(time_precomp_str), time_precomp);
  printf("\nPre-computation finished in %s.\n\n", time_precomp_str);  fflush(stdout);

  /* If too much memory is taken up by the pre-computed indices, print a warning to the
   * user.  Strange crashes in the OpenCL functions can occur when memory is exhausted,
   * and its not obvious that this is the culprit. */
  check_memory_usage();

  /* Start preloading tables into memory. */
  preload_thread_args.rt_dir = strdup(rt_dir);
  err = pthread_create(&preload_thread_id, NULL, preloading_thread, &preload_thread_args);
  if (err != 0) {
    printf("Failed to create thread: %d\n", err);
    return -1;
  }

  /* Using the pre-computed end indices, perform a binary search on all rainbow tables
   * in the target directory.  Any matching indices will trigger false alarm checks. */
  total_tables = count_tables(rt_dir);
  start_timer(&search_start_time);
  search_tables(total_tables, ppi_head, args);

  seconds_to_human_time(time_precomp_str, sizeof(time_precomp_str), time_precomp);
  seconds_to_human_time(time_io_str, sizeof(time_io_str), time_io);
  seconds_to_human_time(time_searching_str, sizeof(time_searching_str), time_searching);
  seconds_to_human_time(time_falsealarms_str, sizeof(time_falsealarms_str), time_falsealarms);
  seconds_to_human_time(time_total_str, sizeof(time_total_str), time_precomp + /*time_io +*/ time_searching + time_falsealarms);
  seconds_to_human_time(time_per_table_str, sizeof(time_per_table_str), (double)(time_precomp + time_io + time_searching + time_falsealarms) / (double)num_tables_processed);

  printf("\n\n        RAINBOW CRACKALACK LOOKUP REPORT\n\n");

  if (num_cracked == 0)
    printf("\nNo hashes were cracked.  :(\n\n\n");
  else {
    printf(" * Crack Summary *\n\n");
    printf("   Of the %u hashes loaded, %u were cracked, or %.2f%%.\n\n", num_hashes, num_cracked, ((double)num_cracked / (double)num_hashes) * 100);

    printf(" Results\n -------\n");
    ppi_cur = ppi_head;
    while(ppi_cur != NULL) {
      if (ppi_cur->plaintext != NULL)
	printf(" %s  %s\n", ppi_cur->hash, ppi_cur->plaintext);

      ppi_cur = ppi_cur->next;
    }
    printf(" -------\n\n");
    printf(" Results have been written in JTR format to:     %s\n", jtr_pot_filename);
    printf(" Results have been written in hashcat format to: %s\n\n\n", hashcat_pot_filename);
  }

  printf(" * Time Summary *\n\n      Precomputation: %s\n      I/O (parallel): %s\n           Searching: %s\n  False alarm checks: %s\n\n               Total: %s\n\n\n", time_precomp_str, time_io_str, time_searching_str, time_falsealarms_str, time_total_str);

  printf(" * Statistics *\n\n          Number of tables processed: %u\n              Number of false alarms: %" QUOTE PRIu64"\n          Number of chains processed: %" QUOTE PRIu64"\n\n                Time spent per table: %s\n     False alarms checked per second: %" QUOTE ".1f\n\n         False alarms per no. chains: %.5f%%\n  Successful cracks per false alarms: %.5f%%\n  Successful cracks per total chains: %.8f%%\n\n\n", num_tables_processed, num_falsealarms, num_chains_processed, time_per_table_str, (double)num_falsealarms / time_falsealarms, ((double)num_falsealarms / (double)num_chains_processed) * 100.0, ((double)num_cracked / (double)num_falsealarms) * 100.0, ((double)num_cracked / (double)num_chains_processed) * 100.0);

  free_precomputed_and_potential_indices(&ppi_head);
  free_loaded_hashes(hashes, &num_hashes);
  FREE(args);
  pthread_barrier_destroy(&barrier);
  return 0;

 err:
  FCLOSE(f);
  FREE(file_data);
  free_precomputed_and_potential_indices(&ppi_head);
  free_loaded_hashes(hashes, &num_hashes);
  FREE(args);
  pthread_barrier_destroy(&barrier);
  return -1;
}
