/*
 * Rainbow Crackalack: test_des.c
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

#include "opencl_setup.h"
#include "test_des.h"


#define DES_TEST_INPUT_SIZE 8
#define DES_TEST_KEY_SIZE 8
#define DES_TEST_OUTPUT_SIZE 8

struct des_test {
  unsigned char key[16];
  unsigned char plaintext[16];
  unsigned char ciphertext[16];
};

/* From https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nbsspecialpublication500-20e1980.pdf, page 33: */
struct des_test des_tests[] = {
  /*{"4FB05E1515AB73A7", "072D43A077075292", "2F22E49BAB7CA1AC"},
  {"49E95D6D4CA229BF", "02FE55778117F12A", "5A6B612CC26CCE4A"},
  {"018310DC409B26D6", "1D9D5C5018F728C2", "5F4C038ED12B2E41"},
  {"1C587F1C13924FEF", "305532286D6F295A", "63FAC0D034D9F793"}*/
  {"0000000000000000", "unused", "aad3b435b51404ee"}
};


int _test_des(cl_device_id device, cl_context context, cl_kernel kernel, unsigned char *plaintext, unsigned char *testkey, unsigned char *ciphertext) {
  CLMAKETESTVARS();
  int test_passed = 0;

  cl_mem input_buffer = NULL, key_buffer = NULL, output_buffer = NULL;
  cl_mem debug_buffer = NULL;

  unsigned char *input = NULL, *key = NULL, *output = NULL;
  unsigned int *debug = NULL;
  unsigned int i = 0, u = 0;

  unsigned char expected_output[8] = {0};
  char hex[3] = {0};


  memset(expected_output, 0, sizeof(expected_output));
  memset(hex, 0, sizeof(hex));

  queue = CLCREATEQUEUE(context, device);

  input = calloc(DES_TEST_INPUT_SIZE, sizeof(unsigned char));
  key = calloc(DES_TEST_KEY_SIZE, sizeof(unsigned char));
  output = calloc(DES_TEST_OUTPUT_SIZE, sizeof(unsigned char));
  #define DEBUG_LEN 16
  debug = calloc(DEBUG_LEN, sizeof(unsigned int));
  if ((input == NULL) || (key == NULL) || (output == NULL) || (debug == NULL)) {
    fprintf(stderr, "Failed to create I/O arrays.\n");
    exit(-1);
  }

  /* Convert the string hex into bytes. */
  for (i = 0; i < 8; i++) {
    memcpy(hex, plaintext + (i * 2), 2);
    sscanf(hex, "%2x", &u);
    input[i] = u;

    memcpy(hex, testkey + (i * 2), 2);
    sscanf(hex, "%2x", &u);
    key[i] = u;

    memcpy(hex, ciphertext + (i * 2), 2);
    sscanf(hex, "%2x", &u);
    expected_output[i] = u;
  }

  CLCREATEARG_ARRAY(0, input_buffer, CL_RO, input, DES_TEST_INPUT_SIZE * sizeof(unsigned char));
  CLCREATEARG_ARRAY(1, key_buffer, CL_RO, key, DES_TEST_KEY_SIZE * sizeof(unsigned char));
  CLCREATEARG_ARRAY(2, output_buffer, CL_WO, output, DES_TEST_OUTPUT_SIZE * sizeof(unsigned char));
  CLCREATEARG_ARRAY(3, debug_buffer, CL_WO, debug, DEBUG_LEN * sizeof(unsigned int)); 
  
  CLRUNKERNEL(queue, kernel, &global_work_size);
  CLFLUSH(queue);
  CLWAIT(queue);

  CLREADBUFFER(output_buffer, DES_TEST_OUTPUT_SIZE * sizeof(unsigned char), output);
  CLREADBUFFER(debug_buffer, DEBUG_LEN * sizeof(unsigned int), debug);

  printf("\nDEBUG: %x %x\n\n", debug[0], debug[1]);
  
  if (memcmp(output, expected_output, 8) == 0)
    test_passed = 1;
  else {
    test_passed = 0;

    printf("\n\n\tExpected: ");
    for(i = 0; i < 8; i++)
      printf("%02x", expected_output[i]);
    printf("\n\tComputed: ");
    for(i = 0; i < DES_TEST_OUTPUT_SIZE; i++)
      printf("%02x", output[i]);
    printf("\n\n");
  }

  CLFREEBUFFER(input_buffer);
  CLFREEBUFFER(key_buffer);
  CLFREEBUFFER(output_buffer);

  CLRELEASEQUEUE(queue);

  FREE(input);
  FREE(key);
  FREE(output);
  FREE(debug);
  return test_passed;
}


int test_des(cl_device_id device, cl_context context, cl_kernel kernel) {
  int tests_passed = 1;
  unsigned int i = 0;

  for (i = 0; i < (sizeof(des_tests) / sizeof(struct des_test)); i++) {
    tests_passed &= _test_des(device, context, kernel, des_tests[i].plaintext, des_tests[i].key, des_tests[i].ciphertext);
  }

  return tests_passed;
}
