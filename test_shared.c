/*
 * Rainbow Crackalack: test_shared.c
 * Copyright (C) 2018-2019  Joe Testa <jtesta@positronsecurity.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include "test_shared.h"


/* Converts a byte array to hex.  Returns 1 on success, or 0 if output buffer is too
 * small. */
unsigned int bytes_to_hex(unsigned char *bytes, unsigned int num_bytes, char *hex, unsigned int hex_size) {
  unsigned int i = 0;


  /* Ensure the output buffer is big enough for all the hex characters. */
  if (hex_size < (num_bytes * 2) + 1)
    return 0;

  for (i = 0; i < num_bytes; i++)
    snprintf(hex + (i * 2), 3, "%02x", bytes[i]);

  return 1;
}


/* Converts a hex string to bytes and returns the number of bytes converted. */
unsigned int hex_to_bytes(char *hex_str, unsigned int bytes_len, unsigned char *bytes) {
  unsigned int i = 0, u = 0;
  char hex[3];

  /* Convert the string hex into bytes. */
  for (i = 0; (i < strlen(hex_str) / 2) && (i < bytes_len); i++) {
    memcpy(hex, hex_str + (i * 2), 2);
    sscanf(hex, "%2x", &u);
    bytes[i] = u;
  }

  return i;
}
