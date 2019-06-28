#include "ntlm9_functions.cl"

__kernel void test_hash_to_index_ntlm9(__global unsigned long *g_hash, __global unsigned int *g_pos, __global unsigned long *g_index) {

  *g_index = hash_to_index_ntlm9((unsigned long)*g_hash, (unsigned int)*g_pos);

}
