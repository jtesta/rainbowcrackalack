#include "shared.h"
#include "ntlm.cl"

#ifdef USE_DES_BITSLICE
#include "des_bs.cl"
#else
#include "des.cl"
#endif

inline void index_to_plaintext(unsigned long index, char *charset, unsigned int charset_len, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned long *plaintext_space_up_to_index, unsigned char *plaintext, unsigned int *plaintext_len) {
  for (int i = plaintext_len_max - 1; i >= plaintext_len_min - 1; i--) {
    if (index >= plaintext_space_up_to_index[i]) {
      *plaintext_len = i + 1;
      break;
    }
  }

  unsigned long index_x = index - plaintext_space_up_to_index[*plaintext_len - 1];
  for (int i = *plaintext_len - 1; i >= 0; i--) {
    plaintext[i] = charset[index_x % charset_len];
    index_x = index_x / charset_len;
  }

  return;
}


inline void do_hash(unsigned int hash_type, unsigned char *plaintext, unsigned int plaintext_len, unsigned char *hash_value, unsigned int *hash_len /*, __global unsigned char *g_debug*/) {
#if HASH_TYPE == HASH_LM
  for (int i = plaintext_len; i < 8; i++)
    plaintext[i] = 0;

/* For some reason, a very strange compiler error happens when the SK array is moved
 * into des.cl:des_encrypt().  Shelved for now... */
#ifdef USE_DES_BITSLICE
  des_encrypt(plaintext, hash_value);
#else
  uint32_t SK[32];
  des_encrypt(SK, plaintext, hash_value /*, g_debug*/);
#endif

  *hash_len = 8;

#elif HASH_TYPE == HASH_NTLM
  ntlm_hash(plaintext, plaintext_len, hash_value /*, g_debug*/);
  *hash_len = 16;
#endif

  return;
}


inline unsigned long hash_to_index(unsigned char *hash_value, unsigned int hash_len, unsigned int reduction_offset, unsigned long plaintext_space_total, unsigned int pos) {
  unsigned long ret = hash_value[7];
  ret <<= 8;
  ret |= hash_value[6];
  ret <<= 8;
  ret |= hash_value[5];
  ret <<= 8;
  ret |= hash_value[4];
  ret <<= 8;
  ret |= hash_value[3];
  ret <<= 8;
  ret |= hash_value[2];
  ret <<= 8;
  ret |= hash_value[1];
  ret <<= 8;
  ret |= hash_value[0];

  return (ret + reduction_offset + pos) % plaintext_space_total;
}


inline unsigned long fill_plaintext_space_table(unsigned int charset_len, unsigned int plaintext_len_min, unsigned int plaintext_len_max, unsigned long *plaintext_space_up_to_index) {
  unsigned long n = 1;

  plaintext_space_up_to_index[0] = 0;
  for (int i = 1; i <= plaintext_len_max; i++) {
    n = n * charset_len;
    if (i < plaintext_len_min)
      plaintext_space_up_to_index[i] = 0;
    else
      plaintext_space_up_to_index[i] = plaintext_space_up_to_index[i - 1] + n;
  }
  return plaintext_space_up_to_index[plaintext_len_max];
}


// Copies the plaintext_space_up_to_index array from global memory to local memory.
inline void copy_plaintext_space_up_to_index(unsigned long *dest, __global unsigned long *src) {
  for (int i = 0; i < MAX_PLAINTEXT_LEN; i++)
    dest[i] = src[i];
}


inline unsigned long generate_rainbow_chain(
    unsigned int hash_type,
    char *charset,
    unsigned int charset_len,
    unsigned int plaintext_len_min,
    unsigned int plaintext_len_max,
    unsigned int reduction_offset,
    unsigned int chain_len,
    unsigned long start,
    unsigned int pos,
    unsigned long *plaintext_space_up_to_index,
    unsigned long plaintext_space_total,
    unsigned char *plaintext,
    unsigned int *plaintext_len,
    unsigned char *hash,
    unsigned int *hash_len) {

  unsigned long index = start;
  for (; pos < chain_len - 1; pos++) {
    index_to_plaintext(index, charset, charset_len, plaintext_len_min, plaintext_len_max, plaintext_space_up_to_index, plaintext, plaintext_len);
    do_hash(hash_type, plaintext, *plaintext_len, hash, hash_len);
    index = hash_to_index(hash, *hash_len, reduction_offset, plaintext_space_total, pos);
  }
  return index;
}
