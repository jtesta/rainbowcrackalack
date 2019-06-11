#include "ntlm8_functions.cl"


/* TODO: specify array length in definition...somehow? */
__kernel void crackalack_ntlm8(
    __global unsigned int *unused1,
    __global char *unused2,
    __global unsigned int *unused3,
    __global unsigned int *unused4,
    __global unsigned int *unused5,
    __global unsigned int *unused6,
    __global unsigned long *g_indices,
    __global unsigned int *unused7) {
  unsigned long index = g_indices[get_global_id(0)];
  unsigned char plaintext[8];


  for (unsigned int pos = 0; pos < 421999; pos++) {
    index_to_plaintext_ntlm8(index, charset, plaintext);
    index = hash_to_index_ntlm8(hash_ntlm8(plaintext), pos);
  }

  g_indices[get_global_id(0)] = index;
  return;
}
