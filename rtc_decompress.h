#ifndef _RTC_DECOMPRESS_H
#define _RTC_DECOMPRESS_H

#include <stdint.h>

int rtc_decompress(char *filename, uint64_t **uncompressed_table, unsigned int *num_chains);

#endif
