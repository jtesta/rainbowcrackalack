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
#include "test_index_to_plaintext_ntlm9.h"

struct i2p_ntlm9_test {
  cl_ulong index;
  char expected_plaintext[MAX_PLAINTEXT_LEN];
};

struct i2p_ntlm9_test i2p_ntlm9_tests[] = {
  {381435424925352145, "YO$tJH3AC"}
};



int gpu_test_index_to_plaintext_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel, cl_ulong index, char *expected_plaintext) {
  CLMAKETESTVARS();
  int test_passed = 0;
  cl_mem index_buffer = NULL, plaintext_buffer = NULL;
  unsigned char *plaintext = NULL;


  queue = CLCREATEQUEUE(context, device);

  plaintext = calloc(MAX_PLAINTEXT_LEN, sizeof(unsigned char));
  if (plaintext == NULL) {
    fprintf(stderr, "Failed to create I/O buffers.\n");
    exit(-1);
  }

  CLCREATEARG(0, index_buffer, CL_RO, index, sizeof(cl_ulong));
  CLCREATEARG_ARRAY(1, plaintext_buffer, CL_WO, plaintext, MAX_PLAINTEXT_LEN);

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue);

  CLREADBUFFER(plaintext_buffer, MAX_PLAINTEXT_LEN, plaintext);

  if (strcmp(expected_plaintext, (char *)plaintext) == 0)
    test_passed = 1;
  else {
    printf("\n\nGPU error:\n\tIndex: %"PRIu64"\n\tExpected: [%"PRIu64"][%s]\n\tCalculated: [%s]\n\n", index, strlen(expected_plaintext), expected_plaintext, plaintext);
  }

  CLFREEBUFFER(index_buffer);
  CLFREEBUFFER(plaintext_buffer);

  CLRELEASEQUEUE(queue);

  FREE(plaintext);
  return test_passed;
}


int test_index_to_plaintext_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel) {
  int tests_passed = 1;
  unsigned int i = 0;


  for (i = 0; i < (sizeof(i2p_ntlm9_tests) / sizeof(struct i2p_ntlm9_test)); i++) {
    tests_passed &= gpu_test_index_to_plaintext_ntlm9(device, context, kernel, i2p_ntlm9_tests[i].index, i2p_ntlm9_tests[i].expected_plaintext);
  }

  return tests_passed;
}
