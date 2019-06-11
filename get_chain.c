/*
 * Rainbow Crackalack: get_chain.c
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

/* This tool extracts a specific chain from an uncompressed rainbow table. */

#ifdef _WIN32
#include <windows.h>
#endif
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal_color.h"

/* The size of one chain entry (16 bytes). */
#define CHAIN_SIZE (unsigned int)(sizeof(uint64_t) * 2)


int main(int ac, char **av) {
  uint64_t start = 0, end = 0;
  char *filename = NULL;
  unsigned int chain_num = 0, file_pos = 0, file_size = 0;
  FILE *f = NULL;


  ENABLE_CONSOLE_COLOR();
  if (ac != 3) {
    printf("\nThis tool outputs a specific chain in an uncompressed rainbow table.\n\nUsage: %s uncompressed_table.rt chain_num\n\n", av[0]);
    exit(0);
  }

  filename = av[1];
  chain_num = (unsigned int)atoi(av[2]);
  file_pos = chain_num * CHAIN_SIZE;

  f = fopen(filename, "rb");

  /* Get the file size. */
  fseek(f, 0, SEEK_END);
  file_size = ftell(f);

  /* Ensure that the file size is aligned to 16 bytes.  Otherwise, this
   * rainbow table is invalid or compressed. */
  if ((file_size % CHAIN_SIZE) != 0) {
    fprintf(stderr, "Error: file size is not aligned to %u bytes: %u\n", CHAIN_SIZE, file_size);
    exit(-1);
  }

  /* Ensure that the requested chain number is in the file. */
  if (((file_size == CHAIN_SIZE) && (chain_num > 0)) ||
      (file_pos > (file_size - CHAIN_SIZE))) {
    fprintf(stderr, "Error: requested chain number would extend past end of file.  Max chain number is %u.\n", (file_size / CHAIN_SIZE) - 1);
    exit(-1);
  }

  fseek(f, file_pos, SEEK_SET);
  if ((fread(&start, sizeof(start), 1, f) != 1) || \
      (fread(&end, sizeof(end), 1, f) != 1)) {
    fprintf(stderr, "Error while reading start and end indices: %s (%d)\n", strerror(errno), errno);
    fclose(f);
    return -1;
  }

  fclose(f);

  printf("Start index: %" PRIu64 "\nEnd index:   %" PRIu64 "\n", start, end);
  return 0;
}
