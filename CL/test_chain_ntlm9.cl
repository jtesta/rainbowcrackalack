#include "ntlm9_functions.cl"

__kernel void test_chain_ntlm9(__global unsigned int *g_chain_len, __global unsigned long *g_start, __global unsigned long *g_end) {

  unsigned long index = *g_start;
  unsigned char plaintext[9];


  for (unsigned int pos = 0; pos < (*g_chain_len - 1); pos++) {
    index_to_plaintext_ntlm9(index, plaintext);
    index = hash_to_index_ntlm9(hash_ntlm9(plaintext), pos);
  }

  *g_end = index;
}
