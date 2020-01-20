/*
 * Rainbow Crackalack: misc.c
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

#ifdef _WIN32
#include <windows.h>
/*#include <versionhelpers.h>*/
#define STATUS_SUCCESS 0
#else
#include <string.h>
#include <sys/sysinfo.h>
#endif

#include <CL/cl.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "charset.h"
#include "misc.h"
#include "shared.h"


/* Given a rainbow table filename, delete its associated log, if any exists. */
void delete_rt_log(char *rt_filename) {
  char log_filename[256] = {0};

  get_rt_log_filename(log_filename, sizeof(log_filename), rt_filename);
  unlink(log_filename);
}


/* Joins two file paths together in a platform-independent way. */
void filepath_join(char *filepath_result, unsigned int filepath_result_size, const char *path1, const char *path2) {
  strncpy(filepath_result, path1, filepath_result_size);
#ifdef _WIN32
  strncat(filepath_result, "\\", filepath_result_size);
#else
  strncat(filepath_result, "/", filepath_result_size);
#endif
  strncat(filepath_result, path2, filepath_result_size);
}


/* Returns an open file's size. */
long get_file_size(FILE *f) {
  long ret = 0;
  long original_pos = ftell(f);  /* Save the file pointer's current position. */


  /* Seek to the end of the file. */
  if (fseek(f, 0, SEEK_END) < 0) {
    fprintf(stderr, "Failed to seeking in file.\n");
    exit(-1);
  }

  ret = ftell(f);

  /* Restore the file pointer to its original position. */
  if (fseek(f, original_pos, SEEK_SET) < 0) {
    fprintf(stderr, "Failed to seeking in file.\n");
    exit(-1);
  }

  return ret;
}


/* Returns the OS name. */
char *get_os_name() {
#ifdef _WIN32
  return "Windows";

  /* We can't get accurate info on Windows 10 without specifying an application XML manifest, which is too much of a pain at the moment... */
  /*
  if (IsWindowsVersionOrGreater(10, 0, 0))  * Ubuntu 18's MinGW doesn't have IsWindows10OrGreater(). *
    return "Windows 10";
  else if (IsWindows8Point1OrGreater())
    return "Windows 8.1";
  else if (IsWindows8OrGreater())
    return "Windows 8";
  else if (IsWindows7OrGreater())
    return "Windows 7";
  else
    return "An old version of Windows";
  */
#else
  return "Linux";
#endif
}


/* Returns the amount of system RAM, in bytes.  Returns zero on error. */
uint64_t get_total_memory() {
  uint64_t total_memory = 0;
#ifdef _WIN32
  MEMORYSTATUSEX ms = {0};

  ms.dwLength = sizeof(MEMORYSTATUSEX);
  if (!GlobalMemoryStatusEx(&ms)) {
    windows_print_error("GlobalMemoryStatusEx");
    return 0;
  }
  total_memory = ms.ullTotalPhys;
#else
  struct sysinfo si = {0};

  if (sysinfo(&si) < 0) {
    fprintf(stderr, "\nFailed to call sysinfo(): %s (%d)\n", strerror(errno), errno);
    return 0;
  }
  total_memory = si.totalram;
#endif
  return total_memory;
}


/* Returns a random number between 0 and max - 1. */
uint64_t get_random(uint64_t max) {
  uint64_t ret = 0;
  unsigned int i = 0;
  unsigned char random_byte = 0;
#ifdef _WIN32
  BCRYPT_ALG_HANDLE hAlgorithm = NULL;


  /* Get a handle to the random number generator. */
  if (BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_RNG_ALGORITHM, NULL, 0) != STATUS_SUCCESS) {
    fprintf(stderr, "Error: failed to obtain handle to random number generator!\n");
    exit(-1);
  }

  for (i = 0; i < 8; i++) {
    /* Get a single random byte. */
    if (BCryptGenRandom(hAlgorithm, &random_byte, sizeof(unsigned char), 0) != STATUS_SUCCESS) {
      fprintf(stderr, "Error: failed to obtain random bytes from random number generator!\n");
      exit(-1);
    }

    /* Shift our return value up by 8 bits and OR in the new random byte. */
    ret <<= 8;
    ret |= random_byte;

    /* If we exceeded the max value wanted by the caller, we're done reading random bytes. */
    if (ret > max)
      break;
  }

  /* Close the RNG handle. */
  if (BCryptCloseAlgorithmProvider(hAlgorithm, 0) != STATUS_SUCCESS)
    fprintf(stderr, "Warning: failed to close handle to random number generator.\n");
#else
  FILE *urandom = fopen("/dev/urandom", "r");


  /* Ensure that we opened a handle to /dev/urandom. */
  if (urandom == NULL) {
    fprintf(stderr, "Error: failed to open /dev/urandom!\n");
    exit(-1);
  }

  for (i = 0; i < 8; i++) {

    /* Get a single random byte. */
    if (fread(&random_byte, sizeof(unsigned char), 1, urandom) != 1) {
      fprintf(stderr, "Error: failed to obtain random bytes from random number generator!\n");
      exit(-1);
    }

    /* Shift our return value up by 8 bits and OR in the new random byte. */
    ret <<= 8;
    ret |= random_byte;

    /* If we exceeded the max value wanted by the caller, we're done reading random bytes. */
    if (ret > max)
      break;
  }

  FCLOSE(urandom);
#endif

  return ret % max;
}


/* Given a rainbow table filename, get its associated log filename. */
void get_rt_log_filename(char *log_filename, size_t log_filename_size, char *rt_filename) {
  snprintf(log_filename, log_filename_size - 1, "%s.log", rt_filename);
}


/* Returns 1 if the parameters form the standard NTLM 8 set, otherwise 0. */
unsigned int is_ntlm8(unsigned int hash_type, char *charset, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int reduction_offset, unsigned int chain_len) {
  if ((hash_type == HASH_NTLM) && \
      (strcmp(charset, CHARSET_ASCII_32_95) == 0) && \
      (plaintext_len_min == 8) && \
      (plaintext_len_max == 8) && \
      (reduction_offset == 0) && \
      (chain_len == 422000))
    return 1;
  else
    return 0;
}


/* Returns 1 if the parameters form the standard NTLM 9 set, otherwise 0. */
unsigned int is_ntlm9(unsigned int hash_type, char *charset, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int reduction_offset, unsigned int chain_len) {
  if ((hash_type == HASH_NTLM) && \
      (strcmp(charset, CHARSET_ASCII_32_95) == 0) && \
      (plaintext_len_min == 9) && \
      (plaintext_len_max == 9) && \
      (reduction_offset == 0) && \
      (chain_len == 803000))
    return 1;
  else
    return 0;
}


/* Given a filename for a rainbow table, parse its parameters.  On success the
 * rt_parameters' parsed flag is set to 1, otherwise it is zero. */
void parse_rt_params(rt_parameters *rt_params, char *rt_filename_orig) {
  /* Filename is in the following format: "%s_%s#%u-%u_%u_%ux%u_%u" */
  char *hpos = NULL;
  char rt_filename[512] = {0};


  rt_params->parsed = 0;

  /* Skip the directory path, if this filename is absolute. */
#ifdef _WIN32
  hpos = strrchr(rt_filename_orig, '\\');
#else
  hpos = strrchr(rt_filename_orig, '/');
#endif
  if (hpos != NULL)
    strncpy(rt_filename, hpos + 1, sizeof(rt_filename));
  else
    strncpy(rt_filename, rt_filename_orig, sizeof(rt_filename));

  /* Ensure that the filename ends in .rt or .rtc. */
  if (!str_ends_with(rt_filename, ".rt") && !str_ends_with(rt_filename, ".rtc"))
    return;

  /* Manually pick out the strings from the filename.  sscanf() can't be used because
   * a buffer overflow can occur (note that the MinGW system doesn't support the
   * "m" format modifier, which would have been a good and portable solution...). */
  hpos = strchr(rt_filename, '#');
  if (hpos) {
    char *suffix = hpos + 1;
    char *upos = NULL;


    *hpos = '\0';
    upos = strchr(rt_filename, '_');
    if (upos) {
      char *hash_name_ptr = rt_filename;
      char *charset_name_ptr = upos + 1;


      *upos = '\0';
      strncpy(rt_params->hash_name, hash_name_ptr, sizeof(rt_params->hash_name));
      strncpy(rt_params->charset_name, charset_name_ptr, sizeof(rt_params->charset_name));

      /* Now parse the unsigned integers. */
      if (sscanf(suffix, "%u-%u_%u_%ux%u_%u", &rt_params->plaintext_len_min, &rt_params->plaintext_len_max, &rt_params->table_index, &rt_params->chain_len, &rt_params->num_chains, &rt_params->table_part) == 6) {


	/* Calculate the reduction offset from the table index. */
	rt_params->reduction_offset = TABLE_INDEX_TO_REDUCTION_OFFSET(rt_params->table_index);
	/* Validate the hash type. & character set name. */
	rt_params->hash_type = hash_str_to_type(rt_params->hash_name);

	/* Ensure that the hash type and character set is valid, the plaintext
	 * length min & max are set properly, and the chain length is set. */
	if ((rt_params->hash_type != HASH_UNDEFINED) && \
	    (validate_charset(rt_params->charset_name) != NULL) && \
	    (rt_params->plaintext_len_min > 0) && \
	    (rt_params->plaintext_len_min <= rt_params->plaintext_len_max) && \
	    (rt_params->plaintext_len_max < MAX_PLAINTEXT_LEN) && \
	    (rt_params->chain_len > 0) && \
	    (rt_params->num_chains > 0))
	  rt_params->parsed = 1;
      }
    }
  }
}


/* Combines realloc() with calloc(). */
void *recalloc(void *ptr, size_t new_size, size_t old_size) {
  ptr = realloc(ptr, new_size);
  if (ptr == NULL) {
    fprintf(stderr, "Failed to realloc buffer.\n");
    exit(-1);
  }

  memset(ptr + old_size, 0, new_size - old_size);
  return ptr;
}


/* Logs a message to the rainbow table log. */
size_t rt_log(rc_file f, const char *fmt, ...) {
  char buf[256] = {0};
  size_t len = 0;

  va_list args;
  va_start(args, fmt);
  len = vsnprintf(buf, sizeof(buf) - 1, fmt, args);
  va_end(args);

  if (len > 0)
    len = rc_fwrite(buf, len, 1, f);

  return len;
}


/* Returns 1 if the string ends with the specified suffix, otherwise 0. */
int str_ends_with(const char *str, const char *suffix) {
  size_t str_len;
  size_t suffix_len;


  if ((str == NULL) || (suffix == NULL))
    return 0;

  str_len = strlen(str);
  suffix_len = strlen(suffix);
  if (suffix_len > str_len)
    return 0;

  return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}


/* Converts a string to lowercase. */
void str_to_lowercase(char *s) {
  unsigned int i = 0;

  for (; i < strlen(s); i++)
    s[i] = tolower(s[i]);
}


/* On Windows, prints the last error. */
#ifdef _WIN32
void windows_print_error(char *func_name) {
  DWORD err_code = GetLastError();
  LPVOID err_str = NULL;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &err_str, 0, NULL);

  fprintf(stderr, "\n%s failed with error %lu: %s\n\n", func_name, err_code, (char *)err_str);
  fflush(stderr);
  LocalFree(err_str);
}
#endif
