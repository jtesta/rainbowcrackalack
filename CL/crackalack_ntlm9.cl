#include "ntlm9_functions.cl"


/* TODO: specify array length in definition...somehow? */
__kernel void crackalack_ntlm9(
    __global unsigned int *unused1,
    __global char *unused2,
    __global unsigned int *unused3,
    __global unsigned int *unused4,
    __global unsigned int *unused5,
    __global unsigned int *g_chain_len,
    __global unsigned long *g_indices,
    __global unsigned int *g_pos_start) {
  unsigned long index = g_indices[get_global_id(0)];
  unsigned char plaintext[9];


  for (unsigned int pos = *g_pos_start; pos < (*g_chain_len - 1); pos++) {
    index_to_plaintext_ntlm9(index, plaintext);
    index = hash_to_index_ntlm9(hash_ntlm9(plaintext), pos);
  }

  g_indices[get_global_id(0)] = index;
  return;
}
