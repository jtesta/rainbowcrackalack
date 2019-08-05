/*
 * MD4 OpenCL kernel based on Solar Designer's MD4 algorithm implementation at:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md4
 * This code is in public domain.
 *
 * This software is Copyright (c) 2010, Dhiru Kholia <dhiru.kholia at gmail.com>
 * and Copyright (c) 2012, magnum
 * and Copyright (c) 2015, Sayantan Datta <std2048@gmail.com>
 * and it is hereby released to the general public under the following terms:
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted.
 *
 * Useful References:
 * 1  nt_opencl_kernel.c (written by Alain Espinosa <alainesp at gmail.com>)
 * 2. http://tools.ietf.org/html/rfc1320
 * 3. http://en.wikipedia.org/wiki/MD4
 */

#include "string.cl"

#undef MD4_LUT3 /* No good for this format, just here for reference */

/* The basic MD4 functions */
#if MD4_LUT3
#define F(x, y, z)	lut3(x, y, z, 0xca)
#elif USE_BITSELECT
#define F(x, y, z)	bitselect((z), (y), (x))
#elif HAVE_ANDNOT
#define F(x, y, z)	((x & y) ^ ((~x) & z))
#else
#define F(x, y, z)	(z ^ (x & (y ^ z)))
#endif

#if MD4_LUT3
#define G(x, y, z)	lut3(x, y, z, 0xe8)
#else
#define G(x, y, z)	(((x) & ((y) | (z))) | ((y) & (z)))
#endif

#if MD4_LUT3
#define H(x, y, z)	lut3(x, y, z, 0x96)
#define H2 H
#else
#define H(x, y, z)	(((x) ^ (y)) ^ (z))
#define H2(x, y, z)	((x) ^ ((y) ^ (z)))
#endif

/* The MD4 transformation for all three rounds. */
#define STEP(f, a, b, c, d, x, s)	  \
	(a) += f((b), (c), (d)) + (x); \
	(a) = rotate((a), (uint)(s)) //(a) = ((a << s) | (a >> (32 - s))) 

inline void md4_encrypt(__private uint *hash, __private uint *W)
{
	hash[0] = 0x67452301;
	hash[1] = 0xefcdab89;
	hash[2] = 0x98badcfe;
	hash[3] = 0x10325476;

	/* Round 1 */
	STEP(F, hash[0], hash[1], hash[2], hash[3], W[0], 3);
	STEP(F, hash[3], hash[0], hash[1], hash[2], W[1], 7);
	STEP(F, hash[2], hash[3], hash[0], hash[1], W[2], 11);
	STEP(F, hash[1], hash[2], hash[3], hash[0], W[3], 19);
	STEP(F, hash[0], hash[1], hash[2], hash[3], W[4], 3);
	STEP(F, hash[3], hash[0], hash[1], hash[2], W[5], 7);
	STEP(F, hash[2], hash[3], hash[0], hash[1], W[6], 11);
	STEP(F, hash[1], hash[2], hash[3], hash[0], W[7], 19);
	STEP(F, hash[0], hash[1], hash[2], hash[3], W[8], 3);
	STEP(F, hash[3], hash[0], hash[1], hash[2], W[9], 7);
	STEP(F, hash[2], hash[3], hash[0], hash[1], W[10], 11);
	STEP(F, hash[1], hash[2], hash[3], hash[0], W[11], 19);
	STEP(F, hash[0], hash[1], hash[2], hash[3], W[12], 3);
	STEP(F, hash[3], hash[0], hash[1], hash[2], W[13], 7);
	STEP(F, hash[2], hash[3], hash[0], hash[1], W[14], 11);
	STEP(F, hash[1], hash[2], hash[3], hash[0], W[15], 19);

	/* Round 2 */
	STEP(G, hash[0], hash[1], hash[2], hash[3], W[0] + 0x5a827999, 3);
	STEP(G, hash[3], hash[0], hash[1], hash[2], W[4] + 0x5a827999, 5);
	STEP(G, hash[2], hash[3], hash[0], hash[1], W[8] + 0x5a827999, 9);
	STEP(G, hash[1], hash[2], hash[3], hash[0], W[12] + 0x5a827999, 13);
	STEP(G, hash[0], hash[1], hash[2], hash[3], W[1] + 0x5a827999, 3);
	STEP(G, hash[3], hash[0], hash[1], hash[2], W[5] + 0x5a827999, 5);
	STEP(G, hash[2], hash[3], hash[0], hash[1], W[9] + 0x5a827999, 9);
	STEP(G, hash[1], hash[2], hash[3], hash[0], W[13] + 0x5a827999, 13);
	STEP(G, hash[0], hash[1], hash[2], hash[3], W[2] + 0x5a827999, 3);
	STEP(G, hash[3], hash[0], hash[1], hash[2], W[6] + 0x5a827999, 5);
	STEP(G, hash[2], hash[3], hash[0], hash[1], W[10] + 0x5a827999, 9);
	STEP(G, hash[1], hash[2], hash[3], hash[0], W[14] + 0x5a827999, 13);
	STEP(G, hash[0], hash[1], hash[2], hash[3], W[3] + 0x5a827999, 3);
	STEP(G, hash[3], hash[0], hash[1], hash[2], W[7] + 0x5a827999, 5);
	STEP(G, hash[2], hash[3], hash[0], hash[1], W[11] + 0x5a827999, 9);
	STEP(G, hash[1], hash[2], hash[3], hash[0], W[15] + 0x5a827999, 13);

	/* Round 3 */
	STEP(H, hash[0], hash[1], hash[2], hash[3], W[0] + 0x6ed9eba1, 3);
	STEP(H2, hash[3], hash[0], hash[1], hash[2], W[8] + 0x6ed9eba1, 9);
	STEP(H, hash[2], hash[3], hash[0], hash[1], W[4] + 0x6ed9eba1, 11);
	STEP(H2, hash[1], hash[2], hash[3], hash[0], W[12] + 0x6ed9eba1, 15);
	STEP(H, hash[0], hash[1], hash[2], hash[3], W[2] + 0x6ed9eba1, 3);
	STEP(H2, hash[3], hash[0], hash[1], hash[2], W[10] + 0x6ed9eba1, 9);
	STEP(H, hash[2], hash[3], hash[0], hash[1], W[6] + 0x6ed9eba1, 11);
	STEP(H2, hash[1], hash[2], hash[3], hash[0], W[14] + 0x6ed9eba1, 15);
	STEP(H, hash[0], hash[1], hash[2], hash[3], W[1] + 0x6ed9eba1, 3);
	STEP(H2, hash[3], hash[0], hash[1], hash[2], W[9] + 0x6ed9eba1, 9);
	STEP(H, hash[2], hash[3], hash[0], hash[1], W[5] + 0x6ed9eba1, 11);
	STEP(H2, hash[1], hash[2], hash[3], hash[0], W[13] + 0x6ed9eba1, 15);
	STEP(H, hash[0], hash[1], hash[2], hash[3], W[3] + 0x6ed9eba1, 3);
	STEP(H2, hash[3], hash[0], hash[1], hash[2], W[11] + 0x6ed9eba1, 9);
	STEP(H, hash[2], hash[3], hash[0], hash[1], W[7] + 0x6ed9eba1, 11);
	STEP(H2, hash[1], hash[2], hash[3], hash[0], W[15] + 0x6ed9eba1, 15);

	hash[0] = hash[0] + 0x67452301;
	hash[1] = hash[1] + 0xefcdab89;
	hash[2] = hash[2] + 0x98badcfe;
	hash[3] = hash[3] + 0x10325476;
}

inline void ntlm_hash(unsigned char *plaintext, unsigned int plaintext_len, unsigned char *hash /*, __global unsigned char *g_debug*/) {
  unsigned int key[16] = {0};
  unsigned int output[4];


#ifdef AMD_ROCM
  /* Below is a workaround for an EXTREMELY confusing bug only observed while using the
   * AMD ROCm driver (I'm going to go ahead and blame the driver...).  plaintext_len
   * is set correctly, yet at the same time, it isn't.  The code below basically
   * sets plaintext_len to the same value that it was; without this, the wrong
   * hash is somehow calculated. */
  /*plaintext[plaintext_len] = 0;

  int my_len = 0;
  for (; my_len < MAX_PLAINTEXT_LEN; my_len++)
    if (plaintext[my_len] == 0)
      break;

  plaintext_len = my_len;*/
  /***********************************************************************************/
#endif

  if (plaintext_len > 27) {
    plaintext[27] = 0;
    plaintext_len = 27;
  }

  int i = 0;
  for (; i < (plaintext_len / 2); i++)
    key[i] = plaintext[i * 2] | (plaintext[(i * 2) + 1] << 16);

  if ((plaintext_len % 2) == 1)
    key[i] = plaintext[plaintext_len - 1] | 0x800000;
  else
    key[i] = 0x80;

  key[14] = plaintext_len << 4;

  md4_encrypt(output, key);

  i = 0;
  hash[i++] = ((output[0] >> 0) & 0xff);
  hash[i++] = ((output[0] >> 8) & 0xff);
  hash[i++] = ((output[0] >> 16) & 0xff);
  hash[i++] = ((output[0] >> 24) & 0xff);
  hash[i++] = ((output[1] >> 0) & 0xff);
  hash[i++] = ((output[1] >> 8) & 0xff);
  hash[i++] = ((output[1] >> 16) & 0xff);
  hash[i++] = ((output[1] >> 24) & 0xff);
  hash[i++] = ((output[2] >> 0) & 0xff);
  hash[i++] = ((output[2] >> 8) & 0xff);
  hash[i++] = ((output[2] >> 16) & 0xff);
  hash[i++] = ((output[2] >> 24) & 0xff);
  hash[i++] = ((output[3] >> 0) & 0xff);
  hash[i++] = ((output[3] >> 8) & 0xff);
  hash[i++] = ((output[3] >> 16) & 0xff);
  hash[i++] = ((output[3] >> 24) & 0xff);
}
