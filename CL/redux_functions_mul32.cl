inline void reduce_9chars(unsigned long index, unsigned int *hi4, unsigned long *lo5) {
  // Divide by multiply

  // 9 chars -> 4 chars, 5 chars
  // floor(2 * 2**ceil(log2(95**9)) / 95**5) = 297,996,874
  // 148,998,437 (297,996,874/2) or 297,996,874

  unsigned int  tmp;
  unsigned long tmp2;

  tmp    = (unsigned int) (((unsigned long) ((unsigned int) (index >> 32)) * 148998437) >> 28); // just right
  // tmp = (unsigned int) (((unsigned long) ((unsigned int) (index >> 31)) * 148998437) >> 29); // overkill
  // tmp = (unsigned int) (((unsigned long) ((unsigned int) (index >> 31)) * 297996874) >> 30); // overkill

  //tmp2 = index - 7737809375 * tmp;
  // 7737809375 == 3442842079 + 2**32
  tmp2 = index - (unsigned long) 3442842079 * tmp - ((unsigned long) tmp << 32);
  if (tmp2 >= 7737809375UL) {
    tmp2 -= 7737809375UL;
    tmp++;
  }
  *hi4 = tmp;
  *lo5 = tmp2;
}

inline void reduce_8chars(unsigned long index, unsigned int *hi4, unsigned int *lo4) {
  // Divide by multiply

  // 8 chars -> 4 chars, 4 chars
  // floor(2 * 2**ceil(log2(95**8)) / 95**4) = 221,169,555
  // 110,584,777 (221,169,555/2) or 221,169,555

  unsigned int tmp;
  unsigned int tmp2;

  // tmp = (unsigned int) (((unsigned long) ((unsigned int) (index >> 26)) * 110584777) >> 27); // not enough
  tmp    = (unsigned int) (((unsigned long) ((unsigned int) (index >> 25)) * 110584777) >> 28); // just right
  // tmp = (unsigned int) (((unsigned long) ((unsigned int) (index >> 25)) * 221169555) >> 29); // overkill

  tmp2 = (unsigned int) index - 81450625 * tmp;
  if (tmp2 >= 81450625) {
    tmp2 -= 81450625;
    tmp++;
  }
  *hi4 = tmp;
  *lo4 = tmp2;
}

inline void reduce_5chars(unsigned long index, unsigned int *hi2, unsigned int *lo3) {
  // Divide by multiply

  // 5 chars -> 2 chars, 3 chars
  // floor(2 * 2**ceil(log2(95**5)) / 95**3) = 20,037
  // 10,018 (20,037/2) or 20,037

  unsigned int tmp;
  unsigned int tmp2;

  // tmp = ((index >> 19) * 10018) >> 14; // not enough
  // tmp = ((index >> 18) * 10018) >> 15; // not enough
  tmp    = ((index >> 18) * 20037) >> 16; // just right

  tmp2 = (unsigned int) index - 857375 * tmp;
  if (tmp2 >= 857375) {
    tmp2 -= 857375;
    tmp++;
  }
  *hi2 = tmp;
  *lo3 = tmp2;
}

inline void reduce_4chars(unsigned int index, unsigned int *hi2, unsigned int *lo2) {
  // Divide by multiply

  // 4 chars -> 2 chars, 2 chars
  // floor(2 * 2**ceil(log2(95**4)) / 95**2) = 29,743
  // 14,871 (29,743/2) or 29,743

  unsigned int tmp;
  unsigned int tmp2;

  // tmp = ((index >> 13) * 14871) >> 14; // not enough
  tmp    = ((index >> 12) * 14871) >> 15; // just right
  // tmp = ((index >> 12) * 29743) >> 16; // overkill

  tmp2 = index - 9025 * tmp;
  if (tmp2 >= 9025) {
    tmp2 -= 9025;
    tmp++;
  }
  *hi2 = tmp;
  *lo2 = tmp2;
}

inline void reduce_3chars(unsigned int index, unsigned char *hi1, unsigned int *lo2) {
  // Divide by multiply

  // 3 chars -> 1 char, 2 chars
  // floor(2 * 2**ceil(log2(95**3)) / 95**2) = 232
  // 116 (232/2) or 232

  unsigned int tmp;
  unsigned int tmp2;

  tmp    = ((index >> 13) * 116) >> 7; // just right
  // tmp = ((index >> 12) * 116) >> 8; // overkill
  // tmp = ((index >> 12) * 232) >> 9; // overkill

  tmp2 = index - 9025 * tmp;
  if (tmp2 >= 9025) {
    tmp2 -= 9025;
    tmp++;
  }
  *hi1 = tmp + 32;
  *lo2 = tmp2;
}

inline void reduce_2chars(unsigned int index, unsigned char *hi1, unsigned char *lo2) {
  // Divide by multiply

  // 2 chars -> 1 char, 1 char
  // floor(2 * 2**ceil(log2(95**2)) / 95**1) = 344
  // 172 (344/2) or 344

  unsigned int tmp;
  unsigned int tmp2;

  tmp    = ((index >> 6) * 172) >>  8; // just right
  // tmp = ((index >> 5) * 172) >>  9; // overkill
  // tmp = ((index >> 5) * 344) >> 10; // overkill

  tmp2 = index - 95 * tmp;
  if (tmp2 >= 95) {
    tmp2 -= 95;
    tmp++;
  }
  *hi1 = tmp + 32;
  *lo2 = tmp2 + 32;
}
