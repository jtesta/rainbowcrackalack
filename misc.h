#ifndef _MISC_H
#define _MISC_H

#include <inttypes.h>

/* The quote format specifier (which on UNIX prints numbers with commas in the thousanth's place, i.e.: %'u") can cause crashes in Windows. */
#ifdef _WIN32
#define QUOTE ""
#else
#define QUOTE "'"
#endif

/* This is the longest chain length that a single kernel invokation can produce.  Beyond this, it must be split up into parts.  Linux drivers don't seem to have a problem with this larger chains, but Windows drivers end up getting killed by the watchdog timer. */
#define MAX_CHAIN_LEN 450000

#define CHAIN_SIZE (unsigned int)(sizeof(uint64_t) * 2)

#define FREE(_ptr) \
  { free(_ptr); _ptr = NULL; }

#define FCLOSE(_f) \
  { if (_f != NULL) { fclose(_f); _f = NULL; } }

#include "file_lock.h"

#ifdef _WIN32
#include <versionhelpers.h>

#define CHECK_MEMORY_SIZE() \
  /* Our code + the OpenCL library does NOT like to run on Windows systems with 4GB \
   * of RAM.  It tends to throw strange errors at strange times, so let's warn the \
   * user ahead of time... */ \
  if (get_total_memory() <= 4294967296) { /* Less than 4GB... */ \
    fprintf(stderr, "\n\n\n\t!! WARNING !!\n\n\nThis system has 4GB of RAM or less.  On Windows systems, this tends to result in strange errors from the OpenCL library.  While it is safe to continue anyway, this would be the prime suspect if any problems occur.  In that case, either run on a system with more memory, or boot this machine in Linux (which has been seen to be much more forgiving in low-memory conditions).\n\n\n\n"); \
    fflush(stderr); \
  }
#define PRINT_WIN7_LOOKUP_WARNING() \
  if (IsWindows7OrGreater() && !IsWindows8OrGreater()) { fprintf(stderr, "\n\n\n\t!! WARNING !!\n\n\nPerforming lookups on Windows 7 is known to be very unstable.  Crashes, screen flickering, and/or strange error messages may be observed.  If this happens, unfortunately, there is no solution.  However, a work-around would be to boot the machine into Linux, which does not show these problems.  Lookups on Windows 10 systems work without issue as well.\n\n\n\n"); fflush(stderr); }
#else
#define CHECK_MEMORY_SIZE() /* Do nothing: Linux systems don't seem to have memory issues */
#define PRINT_WIN7_LOOKUP_WARNING() /* Do nothing: Linux systems don't have lookup problems. */
#endif


/* Struct to track parameters for rainbow tables found in a target directory. */
struct _rt_parameters {
  char hash_name[16];
  unsigned int hash_type;
  char charset_name[32];
  unsigned int plaintext_len_min;
  unsigned int plaintext_len_max;
  unsigned int table_index;
  unsigned int reduction_offset;
  unsigned int chain_len;
  unsigned int num_chains;
  unsigned int table_part;

  unsigned int parsed; /* Set to 1 if parameters successfully parsed, otherwise 0. */
};
typedef struct _rt_parameters rt_parameters;


void delete_rt_log(char *rt_filename);
void filepath_join(char *filepath_result, unsigned int filepath_result_size, const char *path1, const char *path2);
long get_file_size(FILE *f);
char *get_os_name();
uint64_t get_random(uint64_t max);
void get_rt_log_filename(char *log_filename, size_t log_filename_size, char *rt_filename);
uint64_t get_total_memory();
unsigned int hash_str_to_type(char *hash_str);
unsigned int is_ntlm8(unsigned int hash_type, char *charset, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int reduction_offset, unsigned int chain_len);
unsigned int is_ntlm9(unsigned int hash_type, char *charset, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int reduction_offset, unsigned int chain_len);
void parse_rt_params(rt_parameters *rt_params, char *rt_filename);
void *recalloc(void *ptr, size_t new_size, size_t old_size);
size_t rt_log(rc_file f, const char *fmt, ...);
int str_ends_with(const char *str, const char *suffix);

#ifdef _WIN32
void windows_print_error(char *func_name);
#endif

#endif
