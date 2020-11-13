/*
 * Rainbow Crackalack: test_chain.c
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
#include "test_chain_ntlm9.h"


struct ntlm9_chain_test {
  uint64_t start;
  uint64_t end;
};

struct ntlm9_chain_test ntlm9_chain_tests[] = {
  {0UL,    95143244441886396UL},
  {666UL,  350988076771348002UL},
  {1001UL, 146797305889667796UL},
};


/* Test a chain using the CPU. */
int cpu_test_chain_ntlm9(uint64_t start, uint64_t expected_end) {
  uint64_t plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};
  uint64_t computed_end = 0, plaintext_space_total = 0;
  unsigned char hash[16] = {0};
  char plaintext[MAX_PLAINTEXT_LEN] = {0};
  unsigned int hash_len = sizeof(hash), plaintext_len = sizeof(plaintext);


  plaintext_space_total = fill_plaintext_space_table(CHARSET_ASCII_32_95_LEN, 9, 9, plaintext_space_up_to_index);

  computed_end = generate_rainbow_chain(HASH_NTLM, CHARSET_ASCII_32_95, CHARSET_ASCII_32_95_LEN, 9, 9, 0, 803000, start, plaintext_space_up_to_index, plaintext_space_total, plaintext, &plaintext_len, hash, &hash_len);

  if (computed_end != expected_end) {
    fprintf(stderr, "\n\nCPU error:\n\tExpected chain end: %"PRIu64"\n\tComputed chain end: %"PRIu64"\n\n", expected_end, computed_end);
    return 0;
  }

  return 1;
}


/* Test a chain using the GPU. */
int gpu_test_chain_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel, uint64_t start, uint64_t expected_end) {
  CLMAKETESTVARS();
  int test_passed = 0;
  cl_mem start_buffer = NULL, chain_len_buffer = NULL, pos_start_buffer = NULL;
  cl_ulong end = 0;
  cl_mem unused1 = NULL, unused2 = NULL, unused3 = NULL, unused4 = NULL, unused5 = NULL;
  cl_uint unused_int = 0;
  char unused_char[16] = {0};
  cl_ulong *start_array = NULL, *end_array = NULL;
  cl_uint chain_len = 803000, pos_start = 0;


  queue = CLCREATEQUEUE(context, device);

  /*
  CLCREATEARG(0, chain_len_buffer, CL_RO, chain_len, sizeof(chain_len));
  CLCREATEARG(1, start_buffer, CL_RO, start, sizeof(start));
  CLCREATEARG(2, end_buffer, CL_WO, end, sizeof(end));
  */

  start_array = calloc(1, sizeof(cl_ulong));
  end_array = calloc(1, sizeof(cl_ulong));
  if ((start_array == NULL) || (end_array == NULL)) {
    fprintf(stderr, "Error while allocating start & end arrays.\n");
    exit(-1);
  }

  start_array[0] = start;
  end_array[0] = end;

  CLCREATEARG(0, unused1, CL_RO, unused_int, sizeof(unused_int));
  CLCREATEARG_ARRAY(1, unused2, CL_RO, unused_char, sizeof(unused_char));
  CLCREATEARG(2, unused3, CL_RO, unused_int, sizeof(unused_int));
  CLCREATEARG(3, unused4, CL_RO, unused_int, sizeof(unused_int));
  CLCREATEARG(4, unused5, CL_RO, unused_int, sizeof(unused_int));
  CLCREATEARG(5, chain_len_buffer, CL_RO, chain_len, sizeof(cl_uint));
  CLCREATEARG_ARRAY(6, start_buffer, CL_RW, start_array, 1 * sizeof(cl_ulong));
  CLCREATEARG(7, pos_start_buffer, CL_RO, pos_start, sizeof(cl_uint));

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue); 

  CLREADBUFFER(start_buffer, 1 * sizeof(cl_ulong), end_array);


  if (end_array[0] == expected_end)
    test_passed = 1;
  else
    printf("\n\n\tStart:        %"PRIu64"\n\tExpected end: %"PRIu64"\n\tComputed end: %"PRIu64"\n\n", start, expected_end, end);


  CLFREEBUFFER(start_buffer);
  CLFREEBUFFER(chain_len_buffer);
  CLFREEBUFFER(pos_start_buffer);
  CLFREEBUFFER(unused1);
  CLFREEBUFFER(unused2);
  CLFREEBUFFER(unused3);
  CLFREEBUFFER(unused4);
  CLFREEBUFFER(unused5);
  CLRELEASEQUEUE(queue);
  FREE(start_array);
  FREE(end_array);
  return test_passed;
}


int test_chain_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel) {
  int tests_passed = 1;
  unsigned int i = 0;


  for (i = 0; i < (sizeof(ntlm9_chain_tests) / sizeof(struct ntlm9_chain_test)); i++) {
    tests_passed &= gpu_test_chain_ntlm9(device, context, kernel, ntlm9_chain_tests[i].start, ntlm9_chain_tests[i].end);
    tests_passed &= cpu_test_chain_ntlm9(ntlm9_chain_tests[i].start, ntlm9_chain_tests[i].end);
  }

  return tests_passed;
}
