#include "rt.cl"
#include "string.cl"

__kernel void test_chain(
    __global char *g_charset,
    __global unsigned int *g_plaintext_len_min,
    __global unsigned int *g_plaintext_len_max,
    __global unsigned int *g_table_index,
    __global unsigned int *g_chain_len,
    __global unsigned long *g_start,
    __global unsigned long *g_end,
    __global unsigned char *g_debug) {

  char charset[MAX_CHARSET_LEN];
  unsigned int plaintext_len_min = *g_plaintext_len_min;
  unsigned int plaintext_len_max = *g_plaintext_len_max;
  unsigned int reduction_offset = TABLE_INDEX_TO_REDUCTION_OFFSET(*g_table_index);
  unsigned int chain_len = *g_chain_len;
  unsigned long start = *g_start;

  unsigned int charset_len = g_strncpy(charset, g_charset, sizeof(charset));
  unsigned long plaintext_space_up_to_index[MAX_PLAINTEXT_LEN];
  unsigned char plaintext[MAX_PLAINTEXT_LEN];
  unsigned int plaintext_len = 0;
  unsigned char hash[MAX_HASH_OUTPUT_LEN];
  unsigned int hash_len;

  unsigned long plaintext_space_total = fill_plaintext_space_table(charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index);

  *g_end = generate_rainbow_chain(
    HASH_TYPE,
    charset,
    charset_len,
    plaintext_len_min,
    plaintext_len_max,
    reduction_offset,
    chain_len,
    start,
    0,
    plaintext_space_up_to_index,
    plaintext_space_total,
    plaintext,
    &plaintext_len,
    hash,
    &hash_len);

  return;
}
