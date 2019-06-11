#ifndef TEST_SHARED_H
#define TEST_SHARED_H

#include "charset.h"

unsigned int bytes_to_hex(unsigned char *bytes, unsigned int num_bytes, char *hex, unsigned int hex_size);
unsigned int hex_to_bytes(char *hex_str, unsigned int bytes_len, unsigned char *bytes);

#endif
