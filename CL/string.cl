#ifndef _STRING_CL
#define _STRING_CL

/* Performs standard strncpy() on __global source array to a local destination
 * array.  Unlike the traditional strncpy(), however, it returns the number of
 * bytes copied, not a pointer to the destination. */
inline unsigned int g_strncpy(char *dest, __global char *g_src, unsigned int n) {
  int i = 0;
  for (; i < n; i++) {
    dest[i] = g_src[i];
    if (dest[i] == 0)
      break;
  }
  return i;
}

inline unsigned int strlen(char *s) {
  unsigned int i = 0;
  for (; *s; i++, s++)
    ;
  return i;
}

inline void g_memcpy(unsigned char *dest, __global unsigned char *g_src, unsigned int n) {
  unsigned int i = 0;
  for (; i < n; i++)
    dest[i] = g_src[i];
}

inline void bzero(char *s, unsigned int n) {
  unsigned int i;
  for (i = 0; i < n; i++)
    s[i] = 0;
}

#endif
