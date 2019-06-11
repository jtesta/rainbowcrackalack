/*
 * Rainbow Crackalack: crackalack_rtc2rt.c
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

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>

#include "rtc_decompress.h"
#include "version.h"


int main(int ac, char **av) {
  uint64_t *uncompressed_table = NULL;
  unsigned int i = 0, num_chains = 0;
  char *rtc_filename_input = NULL, *rt_filename_output = NULL;
  int ret = 0;
  FILE *f = NULL;


  ENABLE_CONSOLE_COLOR();
  PRINT_PROJECT_HEADER();
  if (ac != 3) {
    fprintf(stderr, "Usage: %s [rtc file input] [rt file output]\n", av[0]);
    return -1;
  }

  rtc_filename_input = av[1];
  rt_filename_output = av[2];
  
  ret = rtc_decompress(rtc_filename_input, &uncompressed_table, &num_chains);
  if (ret != 0) {
    fprintf(stderr, "Error while uncompressing RTC file: %s; error code: %d\n", rtc_filename_input, ret);
    return -1;
  }

  f = fopen(rt_filename_output, "wb");
  if (!f) {
    fprintf(stderr, "Error: could not open %s for writing.\n", rt_filename_output);
    return -1;
  }
  
  for (i = 0; i < num_chains; i++) {
    fwrite(&(uncompressed_table[i * 2]), sizeof(uint64_t), 1, f);
    fwrite(&(uncompressed_table[(i * 2) + 1]), sizeof(uint64_t), 1, f);
  }

  fclose(f);

  printf("Successfully uncompressed %u chains in RTC file \"%s\" to RT file \"%s\".\n", num_chains, rtc_filename_input, rt_filename_output);
  return 0;
}
