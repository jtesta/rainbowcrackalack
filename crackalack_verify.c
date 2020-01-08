/*
 * Rainbow Crackalack: crackalack_verify.c
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal_color.h"
#include "verify.h"
#include "version.h"


static int raw_table = 0, quick_table = 0, sorted_table = 0, truncate = VERIFY_DONT_TRUNCATE;
static struct option long_options[] = {
  {"raw", no_argument, &raw_table, 1},
  {"quick", no_argument, &quick_table, 1},
  {"sorted", no_argument, &sorted_table, 1},
  {"truncate", no_argument, &truncate, VERIFY_TRUNCATE_ON_ERROR},
  {"num_chains", required_argument, 0, 'n'},
  {0, 0, 0, 0}
};


void print_usage(char *prog_name) {
  fprintf(stderr, "This program verifies rainbow tables.\n\n\n  %s --raw [--truncate] [--num_chains X] table.rt\n\nThe above command will verify a newly-generated rainbow table.  This ensures that the table 1.) has sequential start points, and 2.) has non-zero ending points.  Optionally, it can truncate the file to just before the first error found, if any.\n\n\n  %s --quick table.rt\n\nThe above command will quickly verify a newly-generated rainbow table.  It is similar to using '--raw', but does not examine the start & end points, and only verifies 5 random chains.  As a result, it can do basic verification without needing to read the entire table into memory first (which incurs a huge I/O cost).  The use case for this option is for quickly checking terabytes of tables for sanity.\n\n\n  %s --sorted [--num_chains X] table.rtc\n\nThe above command will verify a sorted rainbow table (i.e.: that it is suitable for lookups).  It ensures that the end indices are sorted in ascending order.  The table may be compressed or uncompressed.\n\n\nIn any case, --num_chains sets the number of random chains to verify using CPU code (hence, providing a large number here will have a dramatic effect on the speed of verification).  Unless overridden, this defaults to 100.\n\n\n", prog_name, prog_name, prog_name);
}


int main(int ac, char **av) {
  char *filename = NULL;
  unsigned int table_type = 0;
  int num_chains_to_verify = -1, c = 0, option_index = 0;


  ENABLE_CONSOLE_COLOR();
  PRINT_PROJECT_HEADER();
  while ((c = getopt_long(ac, av, "", long_options, &option_index)) != -1) {
    switch(c) {
    case 0:
      break;
    case 'n':
      num_chains_to_verify = atoi(optarg);
      break;
    default:
      print_usage(av[0]);
      exit(-1);
    }
  }

  /* Only one of --raw, --quick, or --sorted must be specified. */
  if ((raw_table + quick_table + sorted_table) != 1) {
    fprintf(stderr, "\nError: either --raw, --quick, or --sorted must be specified!\n\n");
    print_usage(av[0]);
    exit(-1);
  }

  /* Sorted tables cannot be truncated. */
  if (sorted_table && truncate) {
    fprintf(stderr, "\nError: sorted tables cannot be truncated.\n\n");
    exit(-1);
  }

  /* Ensure that one argument remains (i.e.: the filename). */
  if (optind != ac - 1) {
    fprintf(stderr, "\nError: RT/RTC file must be specified!\n\n");
    print_usage(av[0]);
    exit(-1);
  }
  filename = av[optind];

  if (raw_table)
    table_type = VERIFY_TABLE_TYPE_GENERATED;
  else if (quick_table)
    table_type = VERIFY_TABLE_TYPE_QUICK;
  else if (sorted_table)
    table_type = VERIFY_TABLE_TYPE_LOOKUP;

  if (!verify_rainbowtable_file(filename, table_type, VERIFY_TABLE_IS_COMPLETE, truncate, num_chains_to_verify)) {
    fprintf(stderr, "\n%sRainbow table verification FAILED.%s", REDB, CLR);
    if (truncate == VERIFY_TRUNCATE_ON_ERROR)
      fprintf(stderr, "  File truncated.");
    fprintf(stderr, "\n\n");
    return -1;
  }

  printf("%sRainbow table successfully verified!%s\n", GREENB, CLR);
  return 0;
}
