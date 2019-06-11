#include "shared.h"
#include "rt.cl"

__kernel void false_alarm_check(
    __global unsigned int *g_hash_type,
    __global char *g_charset,
    __global unsigned int *g_plaintext_len_min,
    __global unsigned int *g_plaintext_len_max,
    __global unsigned int *g_reduction_offset,
    __global unsigned long *g_plaintext_space_total,
    __global unsigned long *g_plaintext_space_up_to_index,
    __global unsigned int *g_device_num,
    __global unsigned int *g_total_devices,
    __global unsigned int *g_num_start_indices,
    __global unsigned long *g_start_indices,
    __global unsigned int *g_start_index_positions,
    __global unsigned long *g_hash_base_indices,
    __global unsigned int *g_exec_block_scaler,
    __global unsigned long *g_plaintext_indices) {

  int index_pos = (*g_num_start_indices - *g_device_num) - ((get_global_id(0) + *g_exec_block_scaler) * *g_total_devices) - 1;
  if (index_pos < 0)
    return;

  char charset[MAX_CHARSET_LEN];
  unsigned char plaintext[MAX_PLAINTEXT_LEN];
  unsigned char hash[MAX_HASH_OUTPUT_LEN];
  unsigned int plaintext_len;
  unsigned int hash_len;

  unsigned int charset_len = g_strncpy(charset, g_charset, sizeof(charset));
  unsigned int hash_type = *g_hash_type;
  unsigned int plaintext_len_min = *g_plaintext_len_min;
  unsigned int plaintext_len_max = *g_plaintext_len_max;
  unsigned int reduction_offset = *g_reduction_offset;
  unsigned long plaintext_space_total = *g_plaintext_space_total;
  unsigned long plaintext_space_up_to_index[MAX_PLAINTEXT_LEN];

  copy_plaintext_space_up_to_index(plaintext_space_up_to_index, g_plaintext_space_up_to_index);

  unsigned long index = g_start_indices[index_pos], previous_index = 0;
  unsigned long hash_base_index = g_hash_base_indices[index_pos];
  unsigned int endpoint = g_start_index_positions[index_pos];


  for (unsigned int pos = 0; pos < endpoint + 1; pos++) {
    index_to_plaintext(index, charset, charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index, plaintext, &plaintext_len);
    do_hash(hash_type, plaintext, plaintext_len, hash, &hash_len);

    previous_index = index;
    index = hash_to_index(hash, hash_len, reduction_offset, plaintext_space_total, pos);

    if (index == ((hash_base_index + pos) % plaintext_space_total)) {
      g_plaintext_indices[index_pos] = previous_index;
      return;
    }
  }
}
