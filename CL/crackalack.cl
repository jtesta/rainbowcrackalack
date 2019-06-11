#include "rt.cl"
#include "string.cl"


__kernel void crackalack(
    __global unsigned int *g_hash_type,
    __global char *g_charset,
    __global unsigned int *g_plaintext_len_min,
    __global unsigned int *g_plaintext_len_max,
    __global unsigned int *g_reduction_offset,
    __global unsigned int *g_chain_len,
    __global unsigned long *g_indices,
    __global unsigned int *g_pos_start) {

  unsigned int hash_type = *g_hash_type;
  char charset[MAX_CHARSET_LEN];
  unsigned int plaintext_len_min = *g_plaintext_len_min;
  unsigned int plaintext_len_max = *g_plaintext_len_max;
  unsigned int reduction_offset = *g_reduction_offset;
  unsigned int chain_len = *g_chain_len;
  unsigned long start_index = g_indices[get_global_id(0)];
  unsigned int pos = *g_pos_start;

  unsigned int charset_len = g_strncpy(charset, g_charset, sizeof(charset));
  unsigned long plaintext_space_up_to_index[MAX_PLAINTEXT_LEN];
  unsigned char plaintext[MAX_PLAINTEXT_LEN];
  unsigned int plaintext_len = 0;
  unsigned char hash[MAX_HASH_OUTPUT_LEN];
  unsigned int hash_len;

  unsigned long plaintext_space_total = fill_plaintext_space_table(charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

  // Generate a chain, and store it in the local buffer.
  g_indices[get_global_id(0)] = generate_rainbow_chain(
        hash_type,
        charset,
        charset_len,
        plaintext_len_min,
        plaintext_len_max,
        reduction_offset,
        chain_len,
        start_index++,
	pos,
        plaintext_space_up_to_index,
        plaintext_space_total,
        plaintext,
        &plaintext_len,
        hash,
        &hash_len);
  return;
}
