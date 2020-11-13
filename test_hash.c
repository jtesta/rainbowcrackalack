/*
 * Rainbow Crackalack: test_hash.c
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

#include "opencl_setup.h"

#include "cpu_rt_functions.h"
#include "misc.h"
#include "shared.h"
#include "test_shared.h"

#include "test_hash.h"


struct hash_test {
  char input[MAX_PLAINTEXT_LEN];
  char output[(MAX_HASH_OUTPUT_LEN * 2) + 1];
};

struct hash_test lm_hash_tests[] = {
  {"",        "aad3b435b51404ee"},
  {"passwor", "9b39717fb8d352de"},
  {"PASSWOR", "e52cac67419a9a22"},
  {"ABC",     "8c6f5d02deb21501"},
  {"ABCD",    "e165f0192ef85ebb"},
  {"MPVVOLO", "d663e6cf87a4a45c"},
  {"0123456", "5645f13f500882b2"},
  {"()",      "a31ffd349f83b8ca"},
  {"!!!!",    "844181211e2dd527"},
  {"Z1Y2X3:", "04c20b9a5e0c54ce"},
  {"!@#$%^&", "d0daebaf1cff9d12"},
  {"@#$%^&*", "60faab48f42dcd8b"}
};

struct hash_test ntlm_hash_tests[] = {
  {"",             "31d6cfe0d16ae931b73c59d7e0c089c0"},
  {"12345",        "7a21990fcd3d759941e45c490f143d5f"},
  {"abc123",       "f9e37e83b83c47a93c2f09f66408631b"},
  {"password",     "8846f7eaee8fb117ad06bdd830b7586c"},
  {"computer",     "2b2ac2d1c7c8fda6cea80b5fad7563aa"},
  {"123456",       "32ed87bdb5fdc5e9cba88547376818d4"},
  {"tigger",       "b7e0ea9fbffcf6dd83086e905089effd"},
  {"1234",         "7ce21f17c0aee7fb9ceba532d0546ad6"},
  {"Hockey7!",     "ff91fb3186204bbd651dc6d163d7f113"},
  {"C1t1z3n#",     "ff0bc475edd85a6af13afd6e4c5039a9"},
  {"London101#",   "fe8b0b8163e31388ed16680f5bc6a086"},
  {"Holiday!1234", "fddf95b2194203ddc84d53e822510005"},
};


/* Creates and tests a hash using the CPU. */
int cpu_test_hash_ntlm(char *input, char *expected_output_hex) {
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
int gpu_test_hash(cl_device_id device, cl_context context, cl_kernel kernel, char *_input, char *expected_output_hex) {
  CLMAKETESTVARS();
  int test_passed = 0;
  int i = 0;

  unsigned char expected_output[32] = {0};
  unsigned int expected_output_len = 0;

  cl_mem alg_buffer = NULL, input_buffer = NULL, input_len_buffer = NULL, output_buffer = NULL, output_len_buffer = NULL, debug_buffer = NULL;

  char *input = NULL;
  unsigned char *output = NULL, *debug_ptr = NULL;
  /*unsigned int *debug_ptr = NULL;*/
  
  cl_uint hash_type = HASH_UNDEFINED;
  cl_uint input_len = 0, output_len = 0;


  queue = CLCREATEQUEUE(context, device);

  output = calloc(MAX_HASH_OUTPUT_LEN, sizeof(unsigned char));
  debug_ptr = calloc(DEBUG_LEN, sizeof(unsigned char));
  if ((output == NULL) || (debug_ptr == NULL)) {
    fprintf(stderr, "Error creating I/O arrays.\n");
    exit(-1);
  }


  /* clCreateBuffer() doesn't like zero-length buffers... */
  if (strlen(_input) == 0) {
    input = strdup("X");
    input_len = 0;
  } else {
    input = strdup(_input);
    input_len = strlen(input);
    /*input = calloc(input_len + 1, sizeof(unsigned char));
    strncpy(input, _input, input_len);*/
  }

  CLCREATEARG(0, alg_buffer, CL_RO, hash_type, sizeof(hash_type));
  CLCREATEARG_ARRAY(1, input_buffer, CL_RO, input, strlen(input) + 1);
  CLCREATEARG(2, input_len_buffer, CL_RO, input_len, sizeof(cl_uint));
  CLCREATEARG_ARRAY(3, output_buffer, CL_WO, output, MAX_HASH_OUTPUT_LEN);
  CLCREATEARG(4, output_len_buffer, CL_WO, output_len, sizeof(cl_uint));
  CLCREATEARG_DEBUG(5, debug_buffer, debug_ptr);
  /*CLCREATEARG_ARRAY(5, debug_buffer, CL_WO, debug_ptr, DEBUG_LEN * sizeof(unsigned int));*/

  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue);

  CLREADBUFFER(output_buffer, MAX_HASH_OUTPUT_LEN, output);
  CLREADBUFFER(output_len_buffer, sizeof(cl_uint), &output_len);
  CLREADBUFFER(debug_buffer, DEBUG_LEN, debug_ptr);

  expected_output_len = hex_to_bytes(expected_output_hex, sizeof(expected_output), expected_output);
  if ((expected_output_len == output_len) && (memcmp(output, expected_output, expected_output_len) == 0))
    test_passed = 1;
  else {
    printf("\n\nGPU Error:\n\tPlaintext:     %s\n\tExpected hash: %s\n\tComputed hash: ", input, expected_output_hex);
    for (i = 0; i < output_len; i++)
      printf("%02x", output[i]);
    printf("\n\n");
  }
  /*
  printf("debug: ");
  for (i = 0; i < DEBUG_LEN; i++)
    printf("%x ", debug_ptr[i]);
  printf("\n");
  */
  CLFREEBUFFER(alg_buffer);
  CLFREEBUFFER(input_buffer);
  CLFREEBUFFER(input_len_buffer);
  CLFREEBUFFER(output_buffer);
  CLFREEBUFFER(output_len_buffer);
  CLFREEBUFFER(debug_buffer);

  CLRELEASEQUEUE(queue);

  FREE(input);
  FREE(debug_ptr);
  return test_passed;
}


int test_hash(cl_device_id device, cl_context context, cl_kernel kernel, unsigned int hash_type) {
  int tests_passed = 1;
  unsigned int i = 0;

  if (hash_type == HASH_LM) {
    for (i = 0; i < (sizeof(lm_hash_tests) / sizeof(struct hash_test)); i++)
      tests_passed &= gpu_test_hash(device, context, kernel, lm_hash_tests[i].input, lm_hash_tests[i].output);
  } else if (hash_type == HASH_NTLM) {
    for (i = 0; i < (sizeof(ntlm_hash_tests) / sizeof(struct hash_test)); i++) {
      tests_passed &= gpu_test_hash(device, context, kernel, ntlm_hash_tests[i].input, ntlm_hash_tests[i].output);
      tests_passed &= cpu_test_hash_ntlm(ntlm_hash_tests[i].input, ntlm_hash_tests[i].output);
    }
  } else {
    fprintf(stderr, "Error: unimplemented hash: %u\n", hash_type);
    tests_passed = 0;
  }    

  return tests_passed;
}
