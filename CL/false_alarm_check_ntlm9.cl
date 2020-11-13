#include "ntlm9_functions.cl"


__kernel void false_alarm_check_ntlm9(
    __global unsigned int *unused1,
    __global char *unused2,
    __global unsigned int *unused3,
    __global unsigned int *unused4,
    __global unsigned int *unused5,
    __global unsigned long *unused6,
    __global unsigned long *unused7,
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

  unsigned char plaintext[9];
  unsigned long index = g_start_indices[index_pos], previous_index = 0;
  unsigned long hash_base_index = g_hash_base_indices[index_pos] % 630249409724609375UL;
  unsigned int endpoint = g_start_index_positions[index_pos];

  for (unsigned int pos = 0; pos < endpoint + 1; pos++) {
    index_to_plaintext_ntlm9(index, plaintext);

    previous_index = index;
    index = hash_to_index_ntlm9(hash_ntlm9(plaintext), pos);

    if ((index == (hash_base_index + pos)) || (index == (hash_base_index + pos - 630249409724609375UL))) {
      g_plaintext_indices[index_pos] = previous_index;
      return;
    }
  }
}
