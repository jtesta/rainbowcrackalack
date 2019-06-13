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
#include <CL/cl.h>

#include "cpu_rt_functions.h"
#include "misc.h"
#include "opencl_setup.h"
#include "shared.h"
#include "test_shared.h"
#include "test_chain.h"


struct chain_test {
  char charset[MAX_CHARSET_LEN];
  unsigned int plaintext_len_min;
  unsigned int plaintext_len_max;
  unsigned int table_index;
  unsigned int chain_len;
  uint64_t start;
  uint64_t end;
};

struct chain_test lm_chain_tests[] = {
  /* From open-source RainbowCrack v1.2. */
  {CHARSET_ALPHA, 1, 7, 0, 2,     4216457714UL, 4110420946UL},
  {CHARSET_ALPHA, 1, 7, 0, 10000, 5134134059UL, 4643897595UL},
  {CHARSET_ALPHA, 1, 7, 5, 25777, 1223762207UL, 3058691277UL},

  /* From closed-source RainbowCrack v1.7. */
  {CHARSET_ALPHA, 1, 7, 3, 999,  0UL,   6846105348UL},
  {CHARSET_ALPHA, 1, 7, 6, 5000, 999UL, 2849418373UL},
  {CHARSET_ALPHA_NUMERIC_SYMBOL32_SPACE, 1, 7, 1, 2, 0UL, 5175055677957UL},
  /*{CHARSET_ALPHA_NUMERIC_SYMBOL32_SPACE, 1, 7, 2, 10, 0, 4634515320952},*/
  {CHARSET_ALPHA_NUMERIC_SYMBOL32_SPACE, 1, 7, 2, 345, 9001UL, 6921712277323UL},
  {CHARSET_ASCII_32_65_123_4, 1, 7, 4, 666, 6969UL, 481794222594UL},

  /* Misc. */
  /*{CHARSET_ALPHA, 1, 7, 0, 100,  319,   2872914729},*/
  /*{CHARSET_ALPHA, 1, 7, 0, 100,  100,   1074664166},*/
  /*{CHARSET_ALPHA, 1, 7, 0, 100,  666,   6566110770},*/
};

struct chain_test ntlm_chain_tests[] = {
  {CHARSET_ASCII_32_65_123_4, 1, 8, 0, 1000, 0UL, 495620913785177UL},
  {CHARSET_ASCII_32_95, 8, 8, 0, 666, 456UL, 6003715575086450UL},
  {CHARSET_ASCII_32_95, 9, 9, 0, 1000000, 9999UL, 31278606226553517UL},
};


/* Test a chain using the CPU. */
int cpu_test_chain(char *charset, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int table_index, unsigned int chain_len, uint64_t start, uint64_t expected_end) {
  uint64_t plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};
  uint64_t computed_end = 0, plaintext_space_total = 0;
  unsigned char hash[16] = {0};
  char plaintext[MAX_PLAINTEXT_LEN] = {0};
  unsigned int hash_len = sizeof(hash), plaintext_len = sizeof(plaintext);


  plaintext_space_total = fill_plaintext_space_table(strlen(charset), plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

  computed_end = generate_rainbow_chain(HASH_NTLM, charset, strlen(charset), plaintext_len_min, plaintext_len_max, TABLE_INDEX_TO_REDUCTION_OFFSET(table_index), chain_len, start, plaintext_space_up_to_index, plaintext_space_total, plaintext, &plaintext_len, hash, &hash_len);

  if (computed_end != expected_end) {
    fprintf(stderr, "\n\nCPU error:\n\tExpected chain end: %"PRIu64"\n\tComputed chain end: %"PRIu64"\n\n", expected_end, computed_end);
    return 0;
  }

  return 1;
}


/* Test a chain using the GPU. */
int gpu_test_chain(cl_device_id device, cl_context context, cl_kernel kernel, char *charset, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned int table_index, unsigned int chain_len, uint64_t start, uint64_t expected_end) {
  CLMAKETESTVARS();

  int test_passed = 0;

  cl_mem charset_buffer = NULL, plaintext_len_min_buffer = NULL, plaintext_len_max_buffer = NULL, table_index_buffer = NULL, chain_len_buffer = NULL, start_buffer = NULL, end_buffer = NULL, debug_buffer = NULL;

  unsigned char *debug_ptr = NULL;
  cl_ulong *end_ptr = NULL;
  cl_ulong end = 0;


  queue = CLCREATEQUEUE(context, device);

  CLCREATEARG_ARRAY(0, charset_buffer, CL_RO, charset, strlen(charset) + 1);
  CLCREATEARG(1, plaintext_len_min_buffer, CL_RO, plaintext_len_min, sizeof(plaintext_len_min));
  CLCREATEARG(2, plaintext_len_max_buffer, CL_RO, plaintext_len_max, sizeof(plaintext_len_max));
  CLCREATEARG(3, table_index_buffer, CL_RO, table_index, sizeof(table_index));
  CLCREATEARG(4, chain_len_buffer, CL_RO, chain_len, sizeof(chain_len));
  CLCREATEARG(5, start_buffer, CL_RO, start, sizeof(start));
  CLCREATEARG(6, end_buffer, CL_WO, end, sizeof(end));
  CLCREATEARG_DEBUG(7, debug_buffer, debug_ptr);

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue); 

  end_ptr = calloc(1, sizeof(cl_ulong));
  if (end_ptr == NULL) {
    fprintf(stderr, "Error while creating output buffer.\n");
    exit(-1);
  }

  CLREADBUFFER(end_buffer, sizeof(cl_ulong), end_ptr);

  if (*end_ptr == expected_end)
    test_passed = 1;
  else
    printf("\n\n\tStart:        %"PRIu64"\n\tExpected end: %"PRIu64"\n\tComputed end: %"PRIu64"\n\n", start, expected_end, *end_ptr);

  /*
  READBUFFER(debug_buffer, DEBUG_LEN, debug_ptr);
  printf("debug: ");
  for (i = 0; i < DEBUG_LEN; i++)
    printf("%x ", debug_ptr[i]);
  printf("\n");
  */

  CLFREEBUFFER(charset_buffer);
  CLFREEBUFFER(plaintext_len_min_buffer);
  CLFREEBUFFER(plaintext_len_max_buffer);
  CLFREEBUFFER(table_index_buffer);
  CLFREEBUFFER(chain_len_buffer);
  CLFREEBUFFER(start_buffer);
  CLFREEBUFFER(end_buffer);
  CLFREEBUFFER(debug_buffer);

  CLRELEASEQUEUE(queue);

  FREE(end_ptr);
  FREE(debug_ptr);
  return test_passed;
}


int test_chain(cl_device_id device, cl_context context, cl_kernel kernel, unsigned int hash_type) {
  int tests_passed = 1;
  unsigned int i = 0;

  if (hash_type == HASH_LM) {
    for (i = 0; i < (sizeof(lm_chain_tests) / sizeof(struct chain_test)); i++) {
      tests_passed &= gpu_test_chain(device, context, kernel, lm_chain_tests[i].charset, lm_chain_tests[i].plaintext_len_min, lm_chain_tests[i].plaintext_len_max, lm_chain_tests[i].table_index, lm_chain_tests[i].chain_len, lm_chain_tests[i].start, lm_chain_tests[i].end);
    }
  } else if (hash_type == HASH_NTLM) {
    for (i = 0; i < (sizeof(ntlm_chain_tests) / sizeof(struct chain_test)); i++) {
      tests_passed &= gpu_test_chain(device, context, kernel, ntlm_chain_tests[i].charset, ntlm_chain_tests[i].plaintext_len_min, ntlm_chain_tests[i].plaintext_len_max, ntlm_chain_tests[i].table_index, ntlm_chain_tests[i].chain_len, ntlm_chain_tests[i].start, ntlm_chain_tests[i].end);
      tests_passed &= cpu_test_chain(ntlm_chain_tests[i].charset, ntlm_chain_tests[i].plaintext_len_min, ntlm_chain_tests[i].plaintext_len_max, ntlm_chain_tests[i].table_index, ntlm_chain_tests[i].chain_len, ntlm_chain_tests[i].start, ntlm_chain_tests[i].end);
    }
  }

  return tests_passed;
}
