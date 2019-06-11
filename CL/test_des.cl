#include "des.cl"

__kernel void test_des(
	 __global unsigned char *input,
	 __global unsigned char *key,
	 __global unsigned char *output,
	 __global unsigned int *debug) {
  //uint32_t SK[32];
  unsigned char my_key[8];
  unsigned char my_input[8];
  //unsigned char my_output[8];

  for (int i = 0; i < 8; i++) {
    my_key[i] = key[(get_global_id(0) * 8) + i];
    my_input[i] = input[(get_global_id(0) * 8) + i];
  }
/*
  des_setkey(SK, my_key);
  des_crypt_ecb(SK, my_input, my_output);
*/
  unsigned int k0, k1;

  //des_prep_key(&k0, &k1, my_key);

  k0 = 0; k1 = 0 ;

  unsigned char out[32];
  //des_encrypt(k0, k1, out);

  for(int i = 0; i < 8; i++)
    output[i] = out[i];
/*  
  for (int i = 0; i < 8; i++)
    output[ (get_global_id(0) * 8) + i ] = (unsigned char)(out >> (i * 8));  //my_output[i];
*/
  return;
}
