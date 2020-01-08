/*
 * Rainbow Crackalack: verify.c
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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "charset.h"
#include "cpu_rt_functions.h"
#include "file_lock.h"
#include "misc.h"
#include "rtc_decompress.h"
#include "shared.h"
#include "verify.h"


void _print_chain_error(uint64_t random_chain, uint64_t start, uint64_t actual_end, uint64_t computed_end) {
  fprintf(stderr, "Error: chain #%"PRIu64" is invalid!\n  Start index:        %"PRIu64"\n  Actual chain end:   %"PRIu64"\n  Computed chain end: %"PRIu64"\n\n", random_chain, start, actual_end, computed_end);
}

/* Verifies a rainbow table already loaded from disk. */
int verify_rainbowtable(uint64_t *rainbowtable, unsigned int num_chains, unsigned int table_type, uint64_t expected_start, uint64_t plaintext_space_total, unsigned int *error_chain_num) {
  unsigned int i = 0;
  uint64_t start = 0, end = 0;


  if (table_type == VERIFY_TABLE_TYPE_GENERATED) {
    /* Newly-generated tables must have sequential start indices, and end indices that are not zero. */
    for (i = 0; i < num_chains; i++) {
      start = rainbowtable[i * 2];
      end = rainbowtable[(i * 2) + 1];

      if (start != expected_start) {
	fprintf(stderr, "Start index at chain #%u is not the expected value!  Expected %"PRIu64", but found %"PRIu64".\n", i, expected_start, start);
	*error_chain_num = i;
	return 0;
      }

      if (end == 0) {
	fprintf(stderr, "Chain #%u has an end value of zero!\n", i);
	*error_chain_num = i;
	return 0;
      }

      /* The indices must not be equal or greater than the plaintext space total. */
      if ((plaintext_space_total > 0) && \
	  ((start >= plaintext_space_total) || (end >= plaintext_space_total))) {
	fprintf(stderr, "Start and/or end indices are greater or equal to the plaintext space total!\n\n\tStart index: %"PRIu64"\n\tEnd index:   %"PRIu64"\nPlaintext space total: %"PRIu64"\n\n", start, end, plaintext_space_total);
	*error_chain_num = i;
	return 0;
      }
      expected_start++;
    }
  } else if (table_type == VERIFY_TABLE_TYPE_LOOKUP) {
    uint64_t last_end = 0;


    /* For tables ready to be used for lookups (i.e.: sorted tables), the end indices must be sorted in ascending order. */
    for (i = 0; i < num_chains; i++) {
      start = rainbowtable[i * 2];
      end = rainbowtable[(i * 2) + 1];

      if (end == 0) {
	fprintf(stderr, "Error: end index for chain #%u is zero.\n", i);
	return 0;
      }

      if (end < last_end) {
	fprintf(stderr, "Error: table end indices are not sorted.  Current end index (at chain #%u) is not greater or equal to last end index.\n\n\tCurrent end index: %"PRIu64"\n\tLast end index:    %"PRIu64"\n\n", i, end, last_end);
	return 0;
      }

      /* The indices must not be equal or greater than the plaintext space total. */
      if ((plaintext_space_total > 0) && \
	  ((start >= plaintext_space_total) || (end >= plaintext_space_total))) {
	fprintf(stderr, "Start and/or end indices are greater or equal to the plaintext space total!\n\n\tStart index: %"PRIu64"\n\tEnd index:   %"PRIu64"\nPlaintext space total: %"PRIu64"\n\n", start, end, plaintext_space_total);
	return 0;
      }

      last_end = end;
    }
  } else {
    fprintf(stderr, "Invalid value for table type: %u\n", table_type);
    return 0;
  }

  
  return 1;
}


/* Verifies a rainbow table file.
 *
 * When 'table_type' is VERIFY_TABLE_TYPE_GENERATED, the table is understood to have
 * just been generated.  Hence, the start point must begin at (total_chains_in_table *
 * part_index), then must increment sequentially.  The end points must not be zero.
 * When 'table_type' is VERIFY_TABLE_TYPE_LOOKUP, the table is checked for increasing
 * end indices (as they must be sorted), and must not be zero.
 *
 * If 'table_should_be_complete' is VERIFY_TABLE_IS_COMPLETE, then the file size should
 * be (total_chains_in_table * CHAIN_SIZE).  Otherwise, the file size is not checked,
 * other than ensuring it is some multiple of CHAIN_SIZE.  This is ignored when
 * 'table_type' is VERIFY_TABLE_TYPE_LOOKUP.
 *
 * If 'truncate_at_error' is set, then the file is truncated to just before the first
 * error found, if any. This is ignored when 'table_type' is VERIFY_TABLE_TYPE_LOOKUP.
 *
 * The num_chains_to_verify parameter specifies how many random chains should be
 * verified with CPU code.  When set to -1, the default of 100 is checked.
 * 
 * Returns 1 on success, or 0 on failure. */
int verify_rainbowtable_file(char *filename, unsigned int table_type, unsigned int table_should_be_complete, unsigned int truncate_at_error, int num_chains_to_verify) {
  rt_parameters rt_params = {0};
  uint64_t plaintext_space_up_to_index[MAX_PLAINTEXT_LEN] = {0};

  rc_file f = NULL;
  uint64_t *rainbow_table = NULL;
  char *charset = NULL;

  unsigned int file_size = 0, actual_num_chains = 0, error_chain_num = 0, is_compressed = 0;
  uint64_t expected_start = 0, plaintext_space_total = 0;


  is_compressed = str_ends_with(filename, ".rtc");

  /* Parse the RT parameters from the filename. */
  parse_rt_params(&rt_params, filename);
  if (rt_params.parsed == 0) {
    fprintf(stderr, "Error: failed to parse filename: %s\n", filename);
    return 0;
  }

  charset = validate_charset(rt_params.charset_name);
  if (charset == NULL) {
    fprintf(stderr, "Character set is invalid: %s\n", rt_params.charset_name);
    return 0;
  }

  plaintext_space_total = fill_plaintext_space_table(strlen(charset), rt_params.plaintext_len_min, rt_params.plaintext_len_max, plaintext_space_up_to_index);

  expected_start = (uint64_t)rt_params.num_chains * (uint64_t)rt_params.table_part;

  /* Open the file and obtain a lock on it. */
  f = rc_fopen(filename, 0);
  if (f == NULL) {
    fprintf(stderr, "Failed to open rainbow table file: %s\n", filename);
    return 0;
  }

  if (rc_flock(f) != 0) {
    rc_fclose(f);
    fprintf(stderr, "Failed to lock rainbow table file: %s\n", filename);
    return 0;
  }

  /* Get the file size. */
  rc_fseek(f, 0, RCSEEK_END);
  file_size = rc_ftell(f);
  rc_fseek(f, 0, RCSEEK_SET);

  /* An empty file is always an error. */
  if (file_size == 0) {
    rc_fclose(f);
    fprintf(stderr, "Error: file is empty!\n");
    return 0;
  /* If the table should be complete, then ensure its file size is what we'd expect.  Skip compressed files. */
  } else if ((table_should_be_complete == VERIFY_TABLE_IS_COMPLETE) && (file_size != (rt_params.num_chains * CHAIN_SIZE)) && !is_compressed) {
    rc_fclose(f);
    fprintf(stderr, "Error: table is expected to be complete, but file size does not match expected value.  Expected: %u; actual: %u\n", rt_params.num_chains * CHAIN_SIZE, file_size);
    return 0;
  /* If the table is incomplete, ensure that the file size is a multiple of CHAIN_SIZE. */
  } else if (((file_size % CHAIN_SIZE) != 0) && !is_compressed) {
    rc_fclose(f);
    fprintf(stderr, "Error: file size is not aligned to %u bytes: %u\n", CHAIN_SIZE, file_size);
    return 0;
  }

  /* Compressed tables cannot be quickly checked, as they currently require the entire table to be loaded into memory. */
  if (is_compressed && VERIFY_TABLE_TYPE_QUICK) {
    rc_fclose(f);
    fprintf(stderr, "Error: quick verification of compressed tables is not supported.\n");
    return 0;
  }

  /* Handle the case of a quick table verification up-front. */
  if (table_type == VERIFY_TABLE_TYPE_QUICK) {
    uint64_t random_chain = 0, start = 0, actual_end = 0, computed_end = 0;
    char plaintext[MAX_PLAINTEXT_LEN] = {0};
    unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};
    unsigned int i = 0, plaintext_len = sizeof(plaintext), hash_len = sizeof(hash);


    /* The actual number of chains in the file. */
    actual_num_chains = file_size / CHAIN_SIZE;

    /* Only verify 5 chains. */
    for (i = 0; i < 5; i++) {
      random_chain = get_random(actual_num_chains);
      rc_fseek(f, random_chain * (sizeof(uint64_t) * 2), RCSEEK_SET); /* Jump to random chain. */

      /* Read start & end point from random chain in file. */
      rc_fread(&start, sizeof(uint64_t), 1, f);
      rc_fread(&actual_end, sizeof(uint64_t), 1, f);

      /* Compute the expected end point. */
      computed_end = generate_rainbow_chain(rt_params.hash_type, charset, strlen(charset), rt_params.plaintext_len_min, rt_params.plaintext_len_max, rt_params.reduction_offset, rt_params.chain_len, start, plaintext_space_up_to_index, plaintext_space_total, plaintext, &plaintext_len, hash, &hash_len);

      /* Ensure that the end point in the file matches what we just computed. */
      if (actual_end != computed_end) {
        _print_chain_error(random_chain, start, actual_end, computed_end);
        rc_fclose(f);
        return 0;
      }
    }

    rc_fclose(f);
    return 1;
  }

  /* Load & decompress RTC files. */
  if (is_compressed) {
    int ret = -1;


    rc_fclose(f);
    ret = rtc_decompress(filename, &rainbow_table, &actual_num_chains);
    if (ret < 0) {
      fprintf(stderr, "Error while decompressing RTC file: %d\n", ret);
      return 0;
    }

    if (table_type == VERIFY_TABLE_TYPE_GENERATED)
      printf("\n!! WARNING: table is compressed, yet is supposedly unsorted!  Only sorted tables should be compressed...\n\n");

  } else { /* Simply load uncompressed RT files. */
    actual_num_chains = file_size / CHAIN_SIZE;
    rainbow_table = calloc(actual_num_chains * 2, sizeof(uint64_t));
    if (rainbow_table == NULL) {
      fprintf(stderr, "Error while creating buffer to read file.\n");
      return 0;
    }

    if (rc_fread(rainbow_table, sizeof(uint64_t), actual_num_chains * 2, f) != (actual_num_chains * 2)) {
      fprintf(stderr, "Error while reading file: %s (%d)\n", strerror(errno), errno);
      return 0;
    }
    rc_fclose(f);
  }

  if (!verify_rainbowtable(rainbow_table, actual_num_chains, table_type, expected_start, plaintext_space_total, &error_chain_num)) {
    if ((table_type == VERIFY_TABLE_TYPE_GENERATED) && (truncate_at_error == VERIFY_TRUNCATE_ON_ERROR)) {
      f = rc_fopen(filename, 0);
      if (f == NULL)
	fprintf(stderr, "Error while opening file for truncation: %s (%d)\n", strerror(errno), errno);
      else {
	rc_ftruncate(f, (error_chain_num * CHAIN_SIZE));
	rc_fclose(f);
      }
    }
    goto err;
  }

  /* If the number of chains to verify wasn't set by the user... */
  if (num_chains_to_verify < 0) {
    charset = validate_charset(rt_params.charset_name);

    /* Set it to 50 for NTLM9 tables. */
    if ((charset != NULL) && is_ntlm9(rt_params.hash_type, charset, rt_params.plaintext_len_min, rt_params.plaintext_len_max, rt_params.reduction_offset, rt_params.chain_len))
      num_chains_to_verify = 50;
    else /* Set it to 100 for all other tables. */
      num_chains_to_verify = 100;
  }

  if (num_chains_to_verify > 0) {
    uint64_t start = 0, computed_end = 0, actual_end = 0, random_chain = 0;
    char plaintext[MAX_PLAINTEXT_LEN] = {0};
    unsigned char hash[MAX_HASH_OUTPUT_LEN] = {0};
    unsigned int i = 0, plaintext_len = sizeof(plaintext), hash_len = sizeof(hash);


    if (rt_params.hash_type == HASH_NTLM) {
      for (i = 0; i < num_chains_to_verify; i++) {
	random_chain = get_random(actual_num_chains);
	/*printf("  Verifying chain #%"PRIu64"...\n", random_chain);*/

	start = rainbow_table[random_chain * 2];
	actual_end = rainbow_table[(random_chain * 2) + 1];

	computed_end = generate_rainbow_chain(rt_params.hash_type, charset, strlen(charset), rt_params.plaintext_len_min, rt_params.plaintext_len_max, rt_params.reduction_offset, rt_params.chain_len, start, plaintext_space_up_to_index, plaintext_space_total, plaintext, &plaintext_len, hash, &hash_len);

	if (actual_end != computed_end) {
          _print_chain_error(random_chain, start, actual_end, computed_end);
	  goto err;
	}
      }
    } else {
      printf("Note: skipping CPU chain verification since hash type is not NTLM.\n"); fflush(stdout);
    }
  }

  FREE(rainbow_table);
  return 1;

 err:
  FREE(rainbow_table);
  return 0;
}
