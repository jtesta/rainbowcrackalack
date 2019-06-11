#include "rt.cl"

__kernel void test_hash_to_index(
    __global unsigned char *g_hash,
    __global unsigned int *g_hash_len,
    __global unsigned int *g_charset_len,
    __global unsigned int *g_plaintext_len_min,
    __global unsigned int *g_plaintext_len_max,
    __global unsigned int *g_table_index,
    __global unsigned int *g_pos,
    __global unsigned long *g_index,
    __global unsigned char *g_debug) {

  unsigned char hash[MAX_HASH_OUTPUT_LEN];
  unsigned int hash_len = *g_hash_len;
  unsigned int charset_len = *g_charset_len;
  unsigned int plaintext_len_min = *g_plaintext_len_min;
  unsigned int plaintext_len_max = *g_plaintext_len_max;
  unsigned int reduction_offset = TABLE_INDEX_TO_REDUCTION_OFFSET(*g_table_index);
  unsigned int pos = *g_pos;

  unsigned long plaintext_space_up_to_index[MAX_PLAINTEXT_LEN];

  for (int i = 0; i < hash_len; i++)
    hash[i] = g_hash[i];

  unsigned long plaintext_space_total = fill_plaintext_space_table(charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

  *g_index = hash_to_index(hash, hash_len, reduction_offset, plaintext_space_total, pos);
  return;
}
