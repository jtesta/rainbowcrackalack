/*
 * Rainbow Crackalack: test_index_to_plaintext.c
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

#include "cpu_rt_functions.h"
#include "misc.h"
#include "opencl_setup.h"
#include "shared.h"
#include "test_shared.h"
#include "test_index_to_plaintext.h"

struct i2p_test {
  char charset[MAX_CHARSET_LEN];
  cl_uint plaintext_len_min;
  cl_uint plaintext_len_max;
  cl_ulong index;
  char expected_plaintext[MAX_PLAINTEXT_LEN];
};

struct i2p_test i2p_tests[] = {
  {CHARSET_ALPHA, 1, 7, 0,    "A"},
  {CHARSET_ALPHA, 1, 7, 13,   "N"},
  {CHARSET_ALPHA, 1, 7, 25,   "Z"},
  {CHARSET_ALPHA, 1, 7, 1234, "AUM"},
  {CHARSET_ALPHA, 1, 7, 9999, "NTP"},
  {CHARSET_ALPHA, 1, 7, 8675309,    "RYOGT"},
  {CHARSET_ALPHA, 1, 7, 123456789,  "JJDDJB"},
  {CHARSET_ALPHA, 1, 7, 2813308004, "IBTIHFA"},
  {CHARSET_ALPHA, 1, 7, 4216457714, "MPVVOLO"},
  {CHARSET_ALPHA, 1, 7, 9876543210, "EYFULZO"},
  {CHARSET_ALPHA, 1, 7, 10550285459311888740UL, "EWBFATI"},
  {CHARSET_ASCII_32_95, 1, 8, 666666666666666, "(Rv!f^-+"},
  {CHARSET_MIXALPHA_NUMERIC, 1, 8, 131313131313131313, "yUsFdo5P"},
  {CHARSET_ALPHA_NUMERIC_SYMBOL32_SPACE, 1, 8, 10450885951331886755UL, "36$&'L.."},
  {CHARSET_ASCII_32_95, 8, 8, 5222991064626285, "jk5(J-f\\"},
  {CHARSET_ASCII_32_95, 9, 9, 381435424925352145, "3!u]YO*f%"}
};


int cpu_test_index_to_plaintext(char *charset, cl_uint plaintext_len_min, cl_uint plaintext_len_max, cl_ulong index, char *expected_plaintext) {
  uint64_t plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};
  char computed_plaintext[MAX_PLAINTEXT_LEN] = {0};
  unsigned int computed_plaintext_len = 0;


  fill_plaintext_space_table(strlen(charset), plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

  index_to_plaintext(index, charset, strlen(charset), plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index, computed_plaintext, &computed_plaintext_len);
  if (strcmp(computed_plaintext, expected_plaintext) == 0)
    return 1;
  else {
    printf("\n\nCPU error:\n\tIndex: %"PRIu64"\n\tExpected: [%"PRIu64"][%s]\n\tCalculated: [%u][%s]\n\n", index, strlen(expected_plaintext), expected_plaintext, computed_plaintext_len, computed_plaintext);
    return 0;
  }
}


int gpu_test_index_to_plaintext(cl_device_id device, cl_context context, cl_kernel kernel, char *charset, cl_uint plaintext_len_min, cl_uint plaintext_len_max, cl_ulong index, char *expected_plaintext) {
  CLMAKETESTVARS();
  int test_passed = 0;

  cl_mem charset_buffer = NULL, charset_len_buffer = NULL, plaintext_len_min_buffer = NULL, plaintext_len_max_buffer = NULL, index_buffer = NULL, plaintext_buffer = NULL, plaintext_len_buffer = NULL, debug_buffer = NULL;

  unsigned char *plaintext = NULL;
  unsigned char *debug_ptr = NULL;

  cl_uint charset_len = strlen(charset);
  cl_uint plaintext_len = MAX_PLAINTEXT_LEN;


  queue = CLCREATEQUEUE(context, device);

  plaintext = calloc(MAX_PLAINTEXT_LEN, sizeof(unsigned char));
  if (plaintext == NULL) {
    fprintf(stderr, "Failed to create I/O buffers.\n");
    exit(-1);
  }

  CLCREATEARG_ARRAY(0, charset_buffer, CL_RO, charset, strlen(charset) + 1);
  CLCREATEARG(1, charset_len_buffer, CL_RO, charset_len, sizeof(cl_uint));
  CLCREATEARG(2, plaintext_len_min_buffer, CL_RO, plaintext_len_min, sizeof(cl_uint));
  CLCREATEARG(3, plaintext_len_max_buffer, CL_RO, plaintext_len_max, sizeof(cl_uint));
  CLCREATEARG(4, index_buffer, CL_RO, index, sizeof(cl_ulong));
  CLCREATEARG_ARRAY(5, plaintext_buffer, CL_WO, plaintext, MAX_PLAINTEXT_LEN);
  CLCREATEARG(6, plaintext_len_buffer, CL_WO, plaintext_len, sizeof(cl_uint));
  CLCREATEARG_DEBUG(7, debug_buffer, debug_ptr);

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue);

  CLREADBUFFER(plaintext_buffer, MAX_PLAINTEXT_LEN, plaintext);
  CLREADBUFFER(plaintext_len_buffer, sizeof(cl_uint), &plaintext_len);
  CLREADBUFFER(debug_buffer, DEBUG_LEN, debug_ptr);

  /*
  printf("\ndebug: ");
  for (i = 0; i < DEBUG_LEN; i++) {
    printf("%x ", debug_ptr[i]);
  }
  printf("\n\n");
  */

  if ((plaintext_len == strlen(expected_plaintext)) && (strcmp(expected_plaintext, (char *)plaintext) == 0))
    test_passed = 1;
  else {
    printf("\n\nGPU error:\n\tIndex: %"PRIu64"\n\tExpected: [%"PRIu64"][%s]\n\tCalculated: [%u][%s]\n\n", index, strlen(expected_plaintext), expected_plaintext, plaintext_len, plaintext);
  }

  CLFREEBUFFER(charset_buffer);
  CLFREEBUFFER(charset_len_buffer);
  CLFREEBUFFER(plaintext_len_min_buffer);
  CLFREEBUFFER(plaintext_len_max_buffer);
  CLFREEBUFFER(index_buffer);
  CLFREEBUFFER(plaintext_buffer);
  CLFREEBUFFER(plaintext_len_buffer);
  CLFREEBUFFER(debug_buffer);

  CLRELEASEQUEUE(queue);

  FREE(plaintext);
  FREE(debug_ptr);
  return test_passed;
}


int test_index_to_plaintext(cl_device_id device, cl_context context, cl_kernel kernel) {
  int tests_passed = 1;
  unsigned int i = 0;

  for (i = 0; i < (sizeof(i2p_tests) / sizeof(struct i2p_test)); i++) {
    tests_passed &= gpu_test_index_to_plaintext(device, context, kernel, i2p_tests[i].charset, i2p_tests[i].plaintext_len_min, i2p_tests[i].plaintext_len_max, i2p_tests[i].index, i2p_tests[i].expected_plaintext);

    tests_passed &= cpu_test_index_to_plaintext(i2p_tests[i].charset, i2p_tests[i].plaintext_len_min, i2p_tests[i].plaintext_len_max, i2p_tests[i].index, i2p_tests[i].expected_plaintext);
  }

  return tests_passed;
}
