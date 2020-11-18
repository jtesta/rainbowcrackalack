inline void _binary_search(unsigned int work_load, unsigned int work_load_ctr, __global unsigned long *rainbow_table,
    __global unsigned int *num_chains,
    __global unsigned long *precomputed_end_indices,
    __global unsigned int *num_precomputed_end_indices,
    __global unsigned long *g_output) {
    unsigned long search_index = (unsigned long)precomputed_end_indices[(get_global_id(0) * work_load) + work_load_ctr];
    unsigned int low = 0;
    unsigned int high = *num_chains;
    unsigned int chain;


    while(1) {
      if (high - low <= 8) {
        for (chain = low; chain < high; chain++) {
          if (search_index == rainbow_table[(chain * 2) + 1]) {
            g_output[(get_global_id(0) * work_load) + work_load_ctr] = rainbow_table[chain * 2];
	    return;
          }
        }
	return;
      } else {
        chain = ((high - low - 1) / 2) + low;
        if (search_index >= rainbow_table[(chain * 2) + 1])
          low = chain;
        else
          high = chain;
      }
    }
}

__kernel void binary_search(
    __global unsigned long *rainbow_table,
    __global unsigned int *num_chains,
    __global unsigned long *precomputed_end_indices,
    __global unsigned int *num_precomputed_end_indices,
    __global unsigned int *work_load,
    __global unsigned long *g_output) {

    /* This check doesn't seem to be working; a buffer over-run still somehow happens when the calling program passes precomputed_end_indices that isn't big enough... */
    if ((get_global_id(0) * (*work_load - 1)) >= *num_precomputed_end_indices)
      return;

    for (unsigned int i = 0; i < *work_load; i++)
      _binary_search(*work_load, i, rainbow_table, num_chains, precomputed_end_indices, num_precomputed_end_indices, g_output);
}
