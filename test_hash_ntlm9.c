/*
 * Rainbow Crackalack: test_hash_ntlm9.c
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

#include <stdio.h>
#include <string.h>
#include <CL/cl.h>

#include "cpu_rt_functions.h"
#include "misc.h"
#include "opencl_setup.h"
#include "shared.h"
#include "test_shared.h"

#include "test_hash_ntlm9.h"


struct hash_test {
  char input[MAX_PLAINTEXT_LEN];
  char output[(MAX_HASH_OUTPUT_LEN * 2) + 1];
};


struct hash_test ntlm9_tests[] = {
  {"7aM52Ye6d", "aebe2979a603c3cb0eff8603062c5d86"},
  {"0mg!0mg!X", "2d0b2623bd2490347c1b804481ef327c"},
  {"012345678", "e60e5f326c6deb70eb71d8f12645a96f"},
  {"E?=>T:$Wq", "325bef087fe537b83612340e996065fc"},
};


/* Creates and tests a hash using the CPU. */
int cpu_test_hash_ntlm9(char *input, char *expected_output_hex) {
  unsigned char hash[16] = {0};
  char hash_hex[(sizeof(hash) * 2) + 1] = {0};


  ntlm_hash(input, strlen(input), hash);
  if (!bytes_to_hex(hash, sizeof(hash), hash_hex, sizeof(hash_hex)))
    return 0;

  if (strcmp(hash_hex, expected_output_hex) != 0) {
    printf("\n\nCPU Error:\n\tPlaintext:     %s\n\tExpected hash: %s\n\tComputed hash: %s\n\n", input, expected_output_hex, hash_hex);
    return 0;
  } else
    return 1;
}


/* Creates and tests a hash using the GPU. */
int gpu_test_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel, char *_input, char *expected_output_hex) {
  CLMAKETESTVARS();
  int test_passed = 0;
  int i = 0;

  unsigned char expected_output[32] = {0};
  /*unsigned int expected_output_len = 0;*/

  cl_mem input_buffer = NULL, output_buffer = NULL;

  char *input = NULL;
  cl_ulong output = 0;
  unsigned char actual_output[8] = {0};


  queue = CLCREATEQUEUE(context, device);

  input = strdup(_input);
  if (input == NULL) {
    fprintf(stderr, "Failed to allocate input buffer.\n");
    exit(-1);
  }

  CLCREATEARG_ARRAY(0, input_buffer, CL_RO, input, strlen(input) + 1);
  CLCREATEARG(1, output_buffer, CL_WO, output, sizeof(cl_ulong));
 
  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue);

  CLREADBUFFER(output_buffer, sizeof(cl_ulong), &output);

  /*
  hash[i++] = ((output[0] >> 0) & 0xff);
  hash[i++] = ((output[0] >> 8) & 0xff);
  hash[i++] = ((output[0] >> 16) & 0xff);
  hash[i++] = ((output[0] >> 24) & 0xff);
  hash[i++] = ((output[1] >> 0) & 0xff);
  hash[i++] = ((output[1] >> 8) & 0xff);
  hash[i++] = ((output[1] >> 16) & 0xff);
  hash[i++] = ((output[1] >> 24) & 0xff);

  return ((unsigned long)output[1]) << 32 | (unsigned long)output[0];
  */

  actual_output[0] = (output >> 0) & 0xff;
  actual_output[1] = (output >> 8) & 0xff;
  actual_output[2] = (output >> 16) & 0xff;
  actual_output[3] = (output >> 24) & 0xff;
  actual_output[4] = (output >> 32) & 0xff;
  actual_output[5] = (output >> 40) & 0xff;
  actual_output[6] = (output >> 48) & 0xff;
  actual_output[7] = (output >> 56) & 0xff;
  
  /*expected_output_len =*/ hex_to_bytes(expected_output_hex, sizeof(expected_output), expected_output);
  if (memcmp(actual_output, expected_output, 8) == 0)
    test_passed = 1;
  else {
    printf("\n\nGPU Error:\n\tPlaintext:     %s\n\tExpected hash: %s\n\tComputed hash: ", input, expected_output_hex);
    for (i = 0; i < 8; i++)
      printf("%02x", actual_output[i]);
    printf("\n\n");
  }


  CLFREEBUFFER(input_buffer);
  CLFREEBUFFER(output_buffer);

  CLRELEASEQUEUE(queue);

  FREE(input);
  return test_passed;
}


int test_hash_ntlm9(cl_device_id device, cl_context context, cl_kernel kernel) {
  int tests_passed = 1;
  unsigned int i = 0;


  for (i = 0; i < (sizeof(ntlm9_tests) / sizeof(struct hash_test)); i++) {
    tests_passed &= gpu_test_ntlm9(device, context, kernel, ntlm9_tests[i].input, ntlm9_tests[i].output);
    tests_passed &= cpu_test_hash_ntlm9(ntlm9_tests[i].input, ntlm9_tests[i].output);
  }

  return tests_passed;
}
