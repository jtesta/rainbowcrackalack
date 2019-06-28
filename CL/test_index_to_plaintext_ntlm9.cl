#include "ntlm9_functions.cl"
#include "shared.h"

__kernel void test_index_to_plaintext_ntlm9(__global unsigned long *g_index, __global unsigned char *g_plaintext) {
  unsigned char plaintext[9];

  index_to_plaintext_ntlm9((unsigned long)*g_index, plaintext);

  for (int i = 0; i < 9; i++)
    g_plaintext[i] = plaintext[i];

  return;
}
