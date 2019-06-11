/*
 * Rainbow Crackalack: rtc_decompress.c
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "rtc_decompress.h"


/* Uncompresses an RTC file and returns a pointer to the rainbow table, along with the
 * number of chains in it.  Returns 0 on success, or an error code. */
int rtc_decompress(char *filename, uint64_t **ret_uncompressed_table, unsigned int *ret_num_chains) {
  char *fn_ptr = NULL;
  FILE *f = NULL;
  unsigned int i = 0, chain_size = 0, unused = 0, table_ptr = 0, num_chains = 0;
  int ret = 0;
  uint64_t *uncompressed_table = NULL;

  unsigned int uVersion = 0;
  unsigned short uIndexSBits = 0;
  unsigned short uIndexEBits = 0;
  uint64_t uIndexSMin = 0, uIndexEMin = 0, uIndexEInterval = 0;

  uint64_t s = 0, e = 0, s_mask = 0, /*e_mask = 0,*/ buf[2] = {0};


  *ret_uncompressed_table = NULL;
  *ret_num_chains = 0;

  /* sscanf(), below, is greedy when parsing "%s".  So we will skip past all the
   * strings in the filename. */
  for (i = strlen(filename) - 1; i > 0; i--) {
    if (filename[i] == 'x') {
      fn_ptr = &(filename[i + 1]);
      break;
    }
  }

  if (sscanf(fn_ptr, "%u_%u.rtc", &num_chains, &unused) != 2) {
    fprintf(stderr, "Error: failed to parse number of chains from filename: %s\n", fn_ptr);
    ret = -1;
    goto done;
  }

  /*printf("Total chains in table: %u\n", total_chains_in_table);*/
  uncompressed_table = calloc(num_chains, sizeof(uint64_t) * 2);
  if (uncompressed_table == NULL) {
    fprintf(stderr, "Error: could not allocate %"PRIu64" bytes in memory for uncompressed table.\n", num_chains * sizeof(uint64_t) * 2);
    ret = -2;
    goto done;
  }

  f = fopen(filename, "rb");
  if (f == NULL) {
    fprintf(stderr, "Error: failed to open RTC file %s: %s\n", filename, strerror(errno));
    ret = -3;
    goto done;
  }

  if ((fread(&uVersion, sizeof(unsigned int), 1, f) != 1) || \
      (fread(&uIndexSBits, sizeof(unsigned short), 1, f) != 1) || \
      (fread(&uIndexEBits, sizeof(unsigned short), 1, f) != 1) || \
      (fread(&uIndexSMin, sizeof(uint64_t), 1, f) != 1) ||	  \
      (fread(&uIndexEMin, sizeof(uint64_t), 1, f) != 1) ||	  \
      (fread(&uIndexEInterval, sizeof(uint64_t), 1, f) != 1)) {
    fprintf(stderr, "Error while reading RTC header: %s (%d)\n", strerror(errno), errno);
    ret = -4;
  }

  if (uVersion != 0x30435452) {
    fprintf(stderr, "Error: RTC header invalid.\n");
    ret = -5;
    goto done;
  }

  /*
  printf("uIndexSBits: %u\n", uIndexSBits);
  printf("uIndexEBits: %u\n", uIndexEBits);
  printf("uIndexSMin: %"PRIu64"\n", uIndexSMin);
  printf("uIndexEMin: %"PRIu64"\n", uIndexEMin);
  printf("uIndexEInterval: %"PRIu64"\n", uIndexEInterval);
  */

  if ((uIndexSBits > 64) || (uIndexEBits > 64)) {
    fprintf(stderr, "Error: uIndexSBits and/or uIndexEBits is greater than 64: %u %u\n", uIndexSBits, uIndexEBits);
    ret = -5;
    goto done;
  }

  chain_size = (uIndexSBits + uIndexEBits + 7) / 8;
  if (chain_size > 16) {
    fprintf(stderr, "Error: chain size is somehow greater than 16: %u\n", chain_size);
    ret = -6;
    goto done;
  }
  /*printf("Chain size: %u\n", chain_size);*/

  for (i = 0; i < uIndexSBits; i++) {
    s_mask <<= 1;
    s_mask |= 1;
  }
  /*printf("s_mask: %"PRIu64"\n", s_mask);*/

  for (i = 0; i < num_chains; i++) {
    buf[0] = 0;
    buf[1] = 0;
    if (fread(buf, chain_size, 1, f) != 1) {
      fprintf(stderr, "Error while reading chain: %s (%d)\n", strerror(errno), errno);
      ret = -7;
      goto done;
    }

    s = (buf[0] & s_mask) + uIndexSMin;
    e = uIndexEMin + (uIndexEInterval * i) + ((buf[0] >> uIndexSBits) | (buf[1] << (64 - uIndexSBits)));

    /*printf("#%u: %"PRIu64" %"PRIu64"\n", i, buf[0], buf[1]);
      printf("\t%"PRIu64" %"PRIu64"\n", s, e);*/

    uncompressed_table[table_ptr] = s;
    table_ptr++;
    uncompressed_table[table_ptr] = e;
    table_ptr++;
  }

 done:
  if (f != NULL) {
    fclose(f);
    f = NULL;
  }

  /* On error, free the table.  Set the table pointer to NULL along with num_chains to
   * zero so that the caller gets correct output. */
  if ((ret != 0) && (uncompressed_table != NULL)) {
    free(uncompressed_table);
    uncompressed_table = NULL;
    num_chains = 0;
  }

  *ret_uncompressed_table = uncompressed_table;
  *ret_num_chains = num_chains;
  return ret;
}
