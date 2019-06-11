#include "rt.cl"
/*#include "string.cl"*/

__kernel void test_hash(
    __global unsigned int *g_alg,
    __global char *g_input,
    __global unsigned int *g_input_len,
    __global unsigned char *g_output,
    __global unsigned int *g_output_len
    , __global unsigned char *g_debug) {

  unsigned int alg = *g_alg;
  unsigned char input[MAX_PLAINTEXT_LEN];
  unsigned char output[MAX_HASH_OUTPUT_LEN];
  unsigned int input_len = *g_input_len;
  unsigned int output_len = 0;

  input[0] = 0;
  for (int i = 0; i < input_len; i++)
    input[i] = g_input[i];

  do_hash(alg, input, input_len, output, &output_len /*, g_debug*/);

  *g_output_len = output_len;
  for (int i = 0; i < output_len; i++)
    g_output[i] = output[i];

  return;
}
