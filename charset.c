/*
 * Rainbow Crackalack: charset.c
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

#include "charset.h"
#include <stdio.h>
#include <string.h>

struct charsets {
  char name[32];
  char content[96];
};
struct charsets valid_charsets[] = {
  {"numeric", CHARSET_NUMERIC},
  {"alpha", CHARSET_ALPHA},
  {"alpha-numeric", CHARSET_ALPHA_NUMERIC},
  {"loweralpha", CHARSET_LOWERALPHA},
  {"loweralpha-numeric", CHARSET_LOWERALPHA_NUMERIC},
  {"mixalpha", CHARSET_MIXALPHA},
  {"mixalpha-numeric", CHARSET_MIXALPHA_NUMERIC},
  {"ascii-32-95", CHARSET_ASCII_32_95},
  {"ascii-32-65-123-4", CHARSET_ASCII_32_65_123_4},
  {"alpha-numeric-symbol32-space", CHARSET_ALPHA_NUMERIC_SYMBOL32_SPACE}
};


/* Given the name of a character set (such as "ascii-32-95"), return the
 * characters in that set (" !\"#$%&'()*..."). */
char *validate_charset(char *charset_name) {
  char *ret = NULL;
  unsigned int i = 0;


  /* Loop through all the valid charset names and see if any match what the
   * user chose. */
  for (i = 0; i < (sizeof(valid_charsets) / sizeof(struct charsets)); i++) {
    if (strcmp(charset_name, valid_charsets[i].name) == 0)
      ret = valid_charsets[i].content;
  }

  return ret;
}


/* Get a comma-separated list of valid character set names. */
void get_valid_charsets(char *buf, unsigned int buf_size) {
  unsigned int i = 0;


  if (buf_size == 0)
    return;

  buf[0] = '\0';
  for (i = 0; i < (sizeof(valid_charsets) / sizeof(struct charsets)); i++) {
    strncat(buf, valid_charsets[i].name, buf_size - 1);
    strncat(buf, ", ", buf_size - 1);
  }
  if (strlen(buf) >= 2)
    buf[strlen(buf) - 2] = '\0';

  return;
}
