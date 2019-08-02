/*
 * Rainbow Crackalack: enumerate_chain.c
 * Copyright (C) 2019  Joe Testa <jtesta@positronsecurity.com>
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

/* Enumerates the hashes & plaintexts stored in a rainbow chain. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu_rt_functions.h"
#include "test_shared.h"


char charset[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
#define CHARSET_LEN 95



int main(int ac, char **av) {
  uint64_t plaintext_space_total = 0;
  uint64_t plaintext_space_up_to_index[16] = {0};
  uint64_t index = 0, end_index = 0;
  char plaintext[16] = {0}, hash_hex[48] = {0};
  unsigned char hash[16] = {0};
  unsigned int plaintext_len = 0, chain_len = 0, hash_len = 16, pos = 0;


  if (ac != 5) {
    fprintf(stderr, "Usage: %s num_plaintext_chars chain_len start_index end_index\n\nExample: %s 9 803000 101781 139954541451149691\n\n", av[0], av[0]);
    return -1;
  }

  plaintext_len = strtoul(av[1], NULL, 10);
  chain_len = strtoul(av[2], NULL, 10);
  index = strtoimax(av[3], NULL, 10);
  end_index = strtoimax(av[4], NULL, 10);

  if ((plaintext_len != 8) && (plaintext_len != 9)) {
    fprintf(stderr, "Error: plaintext length must be either 8 or 9!\n");
    return -1;
  }


  plaintext_space_total = fill_plaintext_space_table(CHARSET_LEN, plaintext_len, plaintext_len, plaintext_space_up_to_index);

  printf("Position   Plaintext   Hash   Hash Index\n");
  for (pos = 0; pos < chain_len - 1; pos++) {
    index_to_plaintext(index, charset, CHARSET_LEN, plaintext_len, plaintext_len, plaintext_space_up_to_index, plaintext, &plaintext_len);
    ntlm_hash(plaintext, plaintext_len, hash);

    if (!bytes_to_hex(hash, hash_len, hash_hex, sizeof(hash_hex))) {
      fprintf(stderr, "Error while converting bytes to hex.\n");
      return -1;
    }

    index = hash_to_index(hash, hash_len, 0, plaintext_space_total, pos);
    printf("%u  %s  %s  %"PRIu64"\n", pos, plaintext, hash_hex, index);
  }

  if (index != end_index) {
    fprintf(stderr, "\nError: calculated index (%"PRIu64") != expected index (%"PRIu64")\n\n", index, end_index);
    return -1;
  }

  printf("\nSuccess.\n\n");
  return 0;
}
