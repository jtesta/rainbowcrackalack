#include "ntlm9_functions.cl"

__kernel void test_hash_ntlm9(__global char *g_input, __global unsigned long *g_output) {
  unsigned char input[9];

  for (int i = 0; i < 9; i++)
    input[i] = g_input[i];

  *g_output = hash_ntlm9(input);
  return;
}
