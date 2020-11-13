/*
 * Rainbow Crackalack: test_hash_to_index.c
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

#include "opencl_setup.h"

#include "cpu_rt_functions.h"
#include "misc.h"
#include "shared.h"
#include "test_shared.h"
#include "test_hash_to_index.h"


struct h2i_test {
  char hash[MAX_HASH_OUTPUT_LEN * 2];
  unsigned int charset_len;
  unsigned int plaintext_len_min;
  unsigned int plaintext_len_max;
  unsigned int table_index;
  unsigned int pos;
  uint64_t index;
};

struct h2i_test lm_h2i_tests[] = {
  {"aabbccddeeff0011", CHARSET_ALPHA_LEN, 1, 7, 0, 0,       156475956UL},
  {"aabbccddeeff0011", CHARSET_ALPHA_LEN, 1, 7, 0, 100,     156476056UL},
  {"aabbccddeeff0011", CHARSET_ALPHA_LEN, 1, 7, 8, 0,       157000244UL},
  {"d663e6cf87a4a45c", CHARSET_ALPHA_LEN, 1, 7, 0, 0,      4110420946UL},
  {"0102030405060708", CHARSET_ALPHA_LEN, 1, 7, 5, 750,    8350441011UL},
  {"0102030405060708", CHARSET_ALPHA_LEN, 1, 7, 30, 20000, 8352098661UL},
  {"ffeeddccbbaa9988", CHARSET_ASCII_32_65_123_4_LEN, 1, 7, 0, 0,    1381910712028UL},
  {"123456789abcdef0", CHARSET_ASCII_32_65_123_4_LEN, 1, 7, 7, 9999,  281009513815UL},
};
struct h2i_test ntlm_h2i_tests[] = {
  /*{"", CHARSET_ASCII_32_65_123_4_LEN, 1, 8, 0, 0,       0},*/
  {"123456789abcdef0", CHARSET_ASCII_32_95_LEN, 8, 8, 0, 666,  1438903040496756UL},
  {"deadbeefdeadbeef", CHARSET_ASCII_32_95_LEN, 9, 9, 0, 1337, 258702331091945490UL},
};


int cpu_test_h2i(char *hash_hex, uint64_t plaintext_space_total, unsigned int table_index, unsigned int pos, uint64_t expected_index) {
  unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};
  unsigned int hash_len = hex_to_bytes(hash_hex, sizeof(hash), hash);
  uint64_t computed_index = hash_to_index(hash, hash_len, TABLE_INDEX_TO_REDUCTION_OFFSET(table_index), plaintext_space_total, pos);


  if (computed_index == expected_index)
    return 1;
  else {
    printf("\n\nCPU error:\n\tExpected index: %"PRIu64"\n\tComputed index: %"PRIu64"\n", expected_index, computed_index);
    return 0;
  }
}


int gpu_test_h2i(cl_device_id device, cl_context context, cl_kernel kernel, char *hash_hex, unsigned int charset_len, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int table_index, unsigned int pos, uint64_t expected_index) {
  CLMAKETESTVARS();

  int test_passed = 0;

  cl_mem hash_buffer = NULL, hash_len_buffer = NULL, charset_len_buffer = NULL, plaintext_len_min_buffer = NULL, plaintext_len_max_buffer = NULL, table_index_buffer = NULL, pos_buffer = NULL, index_buffer = NULL, debug_buffer = NULL;

  unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};
  unsigned int hash_len = 0;
  cl_ulong index = 0;

  unsigned char *debug_ptr = NULL;
  cl_ulong *index_ptr = NULL;

  queue = CLCREATEQUEUE(context, device);

  hash_len = hex_to_bytes(hash_hex, sizeof(hash), hash);
  CLCREATEARG_ARRAY(0, hash_buffer, CL_RO, hash, hash_len);
  CLCREATEARG(1, hash_len_buffer, CL_RO, hash_len, sizeof(hash_len));
  CLCREATEARG(2, charset_len_buffer, CL_RO, charset_len, sizeof(charset_len));
  CLCREATEARG(3, plaintext_len_min_buffer, CL_RO, plaintext_len_min, sizeof(plaintext_len_min));
  CLCREATEARG(4, plaintext_len_max_buffer, CL_RO, plaintext_len_max, sizeof(plaintext_len_max));
  CLCREATEARG(5, table_index_buffer, CL_RO, table_index, sizeof(table_index));
  CLCREATEARG(6, pos_buffer, CL_RO, pos, sizeof(pos));
  CLCREATEARG(7, index_buffer, CL_WO, index, sizeof(index));
  CLCREATEARG_DEBUG(8, debug_buffer, debug_ptr);

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue); 

  index_ptr = calloc(1, sizeof(cl_ulong));
  if (index_ptr == NULL) {
    fprintf(stderr, "Error while creating output buffer.\n");
    exit(-1);
  }

  CLREADBUFFER(index_buffer, sizeof(cl_ulong), index_ptr);

  if (*index_ptr == expected_index)
    test_passed = 1;
  else {
    printf("\n\nGPU Error:\n\tExpected index: %"PRIu64"\n\tComputed index: %"PRIu64"\n\n", expected_index, *index_ptr);
  }

  /*
  READBUFFER(debug_buffer, DEBUG_LEN, debug_ptr);
  printf("debug: ");
  for (i = 0; i < DEBUG_LEN; i++)
    printf("%x ", debug_ptr[i]);
  printf("\n");
  */

  CLFREEBUFFER(hash_buffer);
  CLFREEBUFFER(hash_len_buffer);
  CLFREEBUFFER(charset_len_buffer);
  CLFREEBUFFER(plaintext_len_min_buffer);
  CLFREEBUFFER(plaintext_len_max_buffer);
  CLFREEBUFFER(table_index_buffer);
  CLFREEBUFFER(pos_buffer);
  CLFREEBUFFER(index_buffer);
  CLFREEBUFFER(debug_buffer);
  CLRELEASEQUEUE(queue);

  FREE(index_ptr);
  FREE(debug_ptr);
  return test_passed;
}

int test_h2i(cl_device_id device, cl_context context, cl_kernel kernel, unsigned int hash_type) {
  int tests_passed = 1;
  unsigned int i = 0;
  uint64_t plaintext_space_total = 0;
  uint64_t plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};


  if (hash_type == HASH_LM) {
    for (i = 0; i < (sizeof(lm_h2i_tests) / sizeof(struct h2i_test)); i++) {

      /* For LM tests, ensure that the hex is 16 characters long (8 bytes).
       * Otherwise, the test is broken. */
      /*if ((lm_h2i_tests[i].hash_type == HASH_LM) &&
	  (strlen(lm_h2i_tests[i].hash) != 16)) {
	fprintf(stderr, "Error: h2i_test has invalid hash length: %zu [%s]\n", strlen(lm_h2i_tests[i].hash), lm_h2i_tests[i].hash);
	exit(-1);
	}*/

      tests_passed &= gpu_test_h2i(device, context, kernel, lm_h2i_tests[i].hash, lm_h2i_tests[i].charset_len, lm_h2i_tests[i].plaintext_len_min, lm_h2i_tests[i].plaintext_len_max, lm_h2i_tests[i].table_index, lm_h2i_tests[i].pos, lm_h2i_tests[i].index);

      plaintext_space_total = fill_plaintext_space_table(lm_h2i_tests[i].charset_len, lm_h2i_tests[i].plaintext_len_min, lm_h2i_tests[i].plaintext_len_max, plaintext_space_up_to_index);

      tests_passed &= cpu_test_h2i(lm_h2i_tests[i].hash, plaintext_space_total, lm_h2i_tests[i].table_index, lm_h2i_tests[i].pos, lm_h2i_tests[i].index);
    }
  } else if (hash_type == HASH_NTLM) {
    for (i = 0; i < (sizeof(ntlm_h2i_tests) / sizeof(struct h2i_test)); i++) {

      /* For NTLM tests, ensure that the hex is 32 characters long (16 bytes).
       * Otherwise, the test is broken. */
      /*if ((ntlm_h2i_tests[i].hash_type == HASH_LM) &&
	  (strlen(ntlm_h2i_tests[i].hash) != 32)) {
	fprintf(stderr, "Error: h2i_test has invalid hash length: %zu [%s]\n", strlen(ntlm_h2i_tests[i].hash), ntlm_h2i_tests[i].hash);
	exit(-1);
	}*/

      tests_passed &= gpu_test_h2i(device, context, kernel, ntlm_h2i_tests[i].hash, ntlm_h2i_tests[i].charset_len, ntlm_h2i_tests[i].plaintext_len_min, ntlm_h2i_tests[i].plaintext_len_max, ntlm_h2i_tests[i].table_index, ntlm_h2i_tests[i].pos, ntlm_h2i_tests[i].index);

      plaintext_space_total = fill_plaintext_space_table(ntlm_h2i_tests[i].charset_len, ntlm_h2i_tests[i].plaintext_len_min, ntlm_h2i_tests[i].plaintext_len_max, plaintext_space_up_to_index);

      tests_passed &= cpu_test_h2i(ntlm_h2i_tests[i].hash, plaintext_space_total, ntlm_h2i_tests[i].table_index, ntlm_h2i_tests[i].pos, ntlm_h2i_tests[i].index);
    }
  }

  return tests_passed;
}
