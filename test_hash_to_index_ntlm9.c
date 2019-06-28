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
#include <CL/cl.h>

#include "cpu_rt_functions.h"
#include "misc.h"
#include "opencl_setup.h"
#include "shared.h"
#include "test_shared.h"
#include "test_hash_to_index_ntlm9.h"


struct ntlm9_h2i_test {
  char hash[MAX_HASH_OUTPUT_LEN * 2];
  unsigned int pos;
  uint64_t index;
};

struct ntlm9_h2i_test ntlm9_h2i_tests[] = {
  {"123456789abcdef0", 666,  339783322997918631UL},
  {"ffeeddccbbaa9988", 1234, 389345038298493248UL},
  {"deadbeefdeadbeef", 1337, 258702331091945490UL},
};


int cpu_test_h2i_ntlm9(char *hash_hex, uint64_t plaintext_space_total, unsigned int pos, uint64_t expected_index) {
  unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};
  unsigned int hash_len = hex_to_bytes(hash_hex, sizeof(hash), hash);
  uint64_t computed_index = hash_to_index(hash, hash_len, 0, plaintext_space_total, pos);


  if (computed_index == expected_index)
    return 1;
  else {
    printf("\n\nCPU error:\n\tExpected index: %"PRIu64"\n\tComputed index: %"PRIu64"\n", expected_index, computed_index);
    return 0;
  }
}


int gpu_test_h2i_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel, char *hash_hex, unsigned int pos, uint64_t expected_index) {
  CLMAKETESTVARS();
  int test_passed = 0;
  cl_mem hash_buffer = NULL, pos_buffer = NULL, index_buffer = NULL;
  cl_ulong hash_long = 0;
  cl_ulong index = 0;
  unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};


  queue = CLCREATEQUEUE(context, device);

  /*hash_len =*/ hex_to_bytes(hash_hex, sizeof(hash), hash);

  hash_long |= hash[7];
  hash_long <<= 8;
  hash_long |= hash[6];
  hash_long <<= 8;
  hash_long |= hash[5];
  hash_long <<= 8;
  hash_long |= hash[4];
  hash_long <<= 8;
  hash_long |= hash[3];
  hash_long <<= 8;
  hash_long |= hash[2];
  hash_long <<= 8;
  hash_long |= hash[1];
  hash_long <<= 8;
  hash_long |= hash[0];
  

  CLCREATEARG(0, hash_buffer, CL_RO, hash_long, sizeof(hash_long));
  CLCREATEARG(1, pos_buffer, CL_RO, pos, sizeof(pos));
  CLCREATEARG(2, index_buffer, CL_WO, index, sizeof(index));

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue); 

  CLREADBUFFER(index_buffer, sizeof(cl_ulong), &index);


  if (index == expected_index)
    test_passed = 1;
  else {
    printf("\n\nGPU Error:\n\tExpected index: %"PRIu64"\n\tComputed index: %"PRIu64"\n\n", expected_index, index);
  }


  CLFREEBUFFER(hash_buffer);
  CLFREEBUFFER(pos_buffer);
  CLFREEBUFFER(index_buffer);
  CLRELEASEQUEUE(queue);
  return test_passed;
}

int test_h2i_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel) {
  int tests_passed = 1;
  unsigned int i = 0;
  uint64_t plaintext_space_total = 0;
  uint64_t plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};


  for (i = 0; i < (sizeof(ntlm9_h2i_tests) / sizeof(struct ntlm9_h2i_test)); i++) {
    tests_passed &= gpu_test_h2i_ntlm9(device, context, kernel, ntlm9_h2i_tests[i].hash, ntlm9_h2i_tests[i].pos, ntlm9_h2i_tests[i].index);

    plaintext_space_total = fill_plaintext_space_table(95, 9, 9, plaintext_space_up_to_index);

    tests_passed &= cpu_test_h2i_ntlm9(ntlm9_h2i_tests[i].hash, plaintext_space_total, ntlm9_h2i_tests[i].pos, ntlm9_h2i_tests[i].index);
  }

  return tests_passed;
}
