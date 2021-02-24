/*
 * Rainbow Crackalack: crackalack_unit_tests.c
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

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

#include "opencl_setup.h"

#include "shared.h"
#include "test_chain.h"
#include "test_chain_ntlm9.h"
#include "test_hash.h"
#include "test_hash_ntlm9.h"
#include "test_hash_to_index.h"
#include "test_hash_to_index_ntlm9.h"
#include "test_index_to_plaintext.h"
#include "test_index_to_plaintext_ntlm9.h"
#include "version.h"


#define PRINT_PASSED() printf("%spassed.%s\n", GREEN, CLR);
#define PRINT_FAILED() printf("%sFAILED!%s\n", RED, CLR);


int main(int ac, char **av) {
  cl_platform_id platforms[MAX_NUM_PLATFORMS];
  cl_device_id devices[MAX_NUM_DEVICES];
  cl_context context;
  cl_program program;
  cl_kernel kernel;
  int err = 0;
  cl_uint num_platforms = 0, num_devices = 0;

  int ret = 0;
  unsigned int hash_type = HASH_UNDEFINED, all_tests_passed = 1;


  ENABLE_CONSOLE_COLOR();
  PRINT_PROJECT_HEADER();
#ifndef _WIN32
  setenv("CUDA_CACHE_DISABLE", "1", 1); /* Disables kernel caching. */
  setenv("HSA_ENABLE_SDMA", "0", 1); /* The ROCm driver on AMD Vega 64 doesn't work without this. */
#endif
  get_platforms_and_devices(-1, MAX_NUM_PLATFORMS, platforms, &num_platforms, MAX_NUM_DEVICES, devices, &num_devices, 1);

  context = rc_clCreateContext(NULL, num_devices, devices, context_callback, NULL, &err);
  if (err < 0) {
    fprintf(stderr, "Failed to create context: %d\n", err);
    exit(-1);
  }


  /* PRNG test */
  /*
  load_kernel(context, num_devices, devices, "test_prng.cl", "test_prng", &program, &kernel);

  printf("Running PRNG test... ");
  if (!test_prng(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    printf("FAILED!\n");
  } else
    printf("passed.\n");

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);
  */

  /* DES test */
  /*
  load_kernel(context, num_devices, devices, "test_des.cl", "test_des", &program, &kernel);

  printf("Running DES tests... ");
  if (!test_des(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    printf("FAILED!\n");
  } else
    printf("passed.\n");

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);
  */


  /* index_to_plaintext() tests. */
  hash_type = HASH_NTLM;
  load_kernel(context, num_devices, devices, "test_index_to_plaintext.cl", "test_index_to_plaintext", &program, &kernel, hash_type);
  printf("Running NTLM index_to_plaintext() tests... "); fflush(stdout);
  if (!test_index_to_plaintext(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  /* index_to_plaintext_ntlm9() tests. */
  load_kernel(context, num_devices, devices, "test_index_to_plaintext_ntlm9.cl", "test_index_to_plaintext_ntlm9", &program, &kernel, hash_type);
  printf("Running NTLM9 index_to_plaintext_ntlm9() tests... "); fflush(stdout);
  if (!test_index_to_plaintext_ntlm9(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  /* Hash tests. */
  /*
  printf("Running LM hash tests... "); fflush(stdout);
  hash_type = HASH_LM;
  load_kernel(context, num_devices, devices, "test_hash.cl", "test_hash", &program, &kernel, hash_type);
  if (!test_hash(devices[0], context, kernel, hash_type)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);
  */

  
  printf("Running NTLM hash tests... "); fflush(stdout);
  hash_type = HASH_NTLM;
  load_kernel(context, num_devices, devices, "test_hash.cl", "test_hash", &program, &kernel, hash_type);
  if (!test_hash(devices[0], context, kernel, hash_type)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  printf("Running NTLM9 hash tests... "); fflush(stdout);
  load_kernel(context, num_devices, devices, "test_hash_ntlm9.cl", "test_hash_ntlm9", &program, &kernel, hash_type);
  if (!test_hash_ntlm9(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  /* hash_to_index() tests. */
  /*
  printf("Running LM hash_to_index() tests... "); fflush(stdout);
  hash_type = HASH_LM;
  load_kernel(context, num_devices, devices, "test_hash_to_index.cl", "test_hash_to_index", &program, &kernel, hash_type);
  if (!test_h2i(devices[0], context, kernel, hash_type)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);
  */

  printf("Running NTLM hash_to_index() tests... "); fflush(stdout);
  hash_type = HASH_NTLM;
  load_kernel(context, num_devices, devices, "test_hash_to_index.cl", "test_hash_to_index", &program, &kernel, hash_type);
  if (!test_h2i(devices[0], context, kernel, hash_type)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  printf("Running NTLM9 hash_to_index() tests... "); fflush(stdout);
  load_kernel(context, num_devices, devices, "test_hash_to_index_ntlm9.cl", "test_hash_to_index_ntlm9", &program, &kernel, hash_type);
  if (!test_h2i_ntlm9(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  /* Chain tests. */
  /*
  printf("Running LM chain tests... "); fflush(stdout);
  hash_type = HASH_LM;
  load_kernel(context, num_devices, devices, "test_chain.cl", "test_chain", &program, &kernel, hash_type);
  if (!test_chain(devices[0], context, kernel, hash_type)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);
  */


  printf("Running NTLM chain tests... "); fflush(stdout);
  hash_type = HASH_NTLM;
  load_kernel(context, num_devices, devices, "test_chain.cl", "test_chain", &program, &kernel, hash_type);
  if (!test_chain(devices[0], context, kernel, hash_type)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  printf("Running NTLM9 chain tests... "); fflush(stdout);
  hash_type = HASH_NTLM;
  /*load_kernel(context, num_devices, devices, "test_chain_ntlm9.cl", "test_chain_ntlm9", &program, &kernel, hash_type);*/
  load_kernel(context, num_devices, devices, "crackalack_ntlm9.cl", "crackalack_ntlm9", &program, &kernel, hash_type);
  if (!test_chain_ntlm9(devices[0], context, kernel)) {
    ret = -1;
    all_tests_passed = 0;
    PRINT_FAILED();
  } else
    PRINT_PASSED();

  CLRELEASEKERNEL(kernel);
  CLRELEASEPROGRAM(program);


  if (all_tests_passed)
    printf("\n\t%sALL UNIT TESTS PASS!%s\n\n", GREENB, CLR);
  else
    printf("\n\t%sSome unit tests failed!%s  :(\n\n", REDB, CLR);

  CLRELEASECONTEXT(context);
  return ret;
}
