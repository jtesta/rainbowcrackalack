/*
 * Rainbow Crackalack: perfectify.c
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

/* This program will compare a sorted rainbow table with a directory of sorted
 * tables, and strip out chains with identical end points. */

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clock.h"

#define CHAIN_SIZE (sizeof(uint64_t) * 2)
#define FREE(_ptr) { free(_ptr); _ptr = NULL; }
#define FCLOSE(_f) { if (_f != NULL) { fclose(_f); _f = NULL; } }
#define CLOSEDIR(_d) { if (_d != NULL) { closedir(_d); _d = NULL; } }


struct source_rt {
  char *filename;
  unsigned long num_chains;
  uint64_t *table;
  unsigned long num_pruned_chains;
  unsigned long pruned_since_last_compression;
  unsigned long in_table_duplicates;
};


/* TODO: this is in misc.c too.  Merge! */
int str_ends_with(const char *str, const char *suffix) {
  size_t str_len;
  size_t suffix_len;


  if ((str == NULL) || (suffix == NULL))
    return 0;

  str_len = strlen(str);
  suffix_len = strlen(suffix);
  if (suffix_len > str_len)
    return 0;

  return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}


/* Compacts a rainbow table (this involves stripping out chains that have endpoints of zero).  Returns the new
 * number of chains in the table. */
unsigned int compact_table(uint64_t *rainbow_table, unsigned int num_chains) {
  unsigned int current_chain = 0, compacted_chain = 0;
  uint64_t end = 0;


  for (current_chain = 0; current_chain < num_chains; current_chain++) {
    end = rainbow_table[(current_chain * 2) + 1];
    if (end != 0) {
      rainbow_table[(compacted_chain * 2)] = rainbow_table[(current_chain * 2)];
      rainbow_table[(compacted_chain * 2) + 1] = end;
      compacted_chain++;
    }
  }
  return compacted_chain;
}


/* Loads a rainbow table from disk, and returns it as an array and sets num_chains
 * accordingly.  Returns NULL on error. */
uint64_t *load_rainbow_table(char *filename, unsigned long *num_chains) {
  FILE *f = fopen(filename, "rb");
  uint64_t *rainbow_table = NULL;
  long file_size = 0;


  if (f == NULL) {
    fprintf(stderr, "Failed to open rainbow table file: %s\n", filename);
    *num_chains = 0;
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  /* Load the entire rainbow table at once. */
  *num_chains = file_size / CHAIN_SIZE;
  rainbow_table = calloc(*num_chains * 2, sizeof(uint64_t));
  if (fread(rainbow_table, sizeof(uint64_t), *num_chains * 2, f) != *num_chains * 2) {
    fprintf(stderr, "Error while reading file: %s (%d)\n", strerror(errno), errno);
    *num_chains = 0;
    FCLOSE(f);
    return NULL;
  }
  FCLOSE(f);

  return rainbow_table;
}


/* Prunes a set of source tables to a comparison table. */
void prune_tables(struct source_rt *source_rts, unsigned int num_source_files, uint64_t *compare_table, unsigned long compare_num_chains) {
  unsigned int i = 0;
  long compare_current_chain = 0;
  unsigned int pruned_chains = 0;
  uint64_t compare_end = 0, process_end = 0;
  unsigned long process_current_chain = 0, process_num_chains = 0;
  uint64_t *process_table = NULL;


  /* If compare table is NULL, look for in-table duplicates only. */
  if ((compare_table == NULL) && (compare_num_chains == 0)) {
    for (i = 0; i < num_source_files; i++) {
      uint64_t previous_process_end = 0;
      uint64_t *process_table = source_rts[i].table;


      for (process_current_chain = 0; process_current_chain < source_rts[i].num_chains; process_current_chain++) {
	process_end = process_table[(process_current_chain * 2) + 1];

	if ((process_end != 0) && (process_end == previous_process_end)) {
	  process_table[(process_current_chain * 2) + 1] = 0;
	  source_rts[i].in_table_duplicates++;
	  source_rts[i].num_pruned_chains++;
	} else
	  previous_process_end = process_end;
      }
    }

    return;
  }


  for (i = 0; i < num_source_files; i++) {
    process_table = source_rts[i].table;
    process_num_chains = source_rts[i].num_chains;
    process_current_chain = 0;
    compare_current_chain = 0;
    pruned_chains = 0;

    while ((process_current_chain < process_num_chains) && (compare_current_chain < compare_num_chains)) {
      process_end = process_table[(process_current_chain * 2) + 1];
      compare_end = compare_table[(compare_current_chain * 2) + 1];

      if (process_end == compare_end) {
	/*printf("Dup: %lu %lu (%"PRIu64" == %"PRIu64")\n", process_current_chain, compare_current_chain, process_end, compare_end);*/

	/* Set the endpoint of this chain to zero, so we know to delete it later. */
	process_table[(process_current_chain * 2) + 1] = 0;

	pruned_chains++;  /* Number pruned with respect to current comparison table. */
	source_rts[i].num_pruned_chains++;  /* Total number pruned w.r.t. all tables compared so far. */
	source_rts[i].pruned_since_last_compression++;  /* Number of chains pruned since the process table was last compressed. */

	process_current_chain++;
	/*compare_current_chain++;*/
      } else if (process_end < compare_end)
	process_current_chain++;
      else if (process_end > compare_end)
	compare_current_chain++;
      else {
	fprintf(stderr, "\n  !!!! Error: this should never happen!\n\n"); fflush(stderr);
	exit(-1);
      }
    }

    if (pruned_chains > 0) {
      printf("Pruned %u chains from %s\n", pruned_chains, source_rts[i].filename); fflush(stdout);
    } else {
      printf("\n  !! WARNING: no chains pruned while processing %s!\n\n", source_rts[i].filename); fflush(stdout);
    }

    /* Every 8M chains pruned, compress the table to remove empty chains.  This
     * slightly speeds up comparisons. */
    if (source_rts[i].pruned_since_last_compression > (8 * 1024 * 1024)) {
      printf("  -> Table compression reduced number of chains from %lu to ", process_num_chains);
      source_rts[i].num_chains = process_num_chains = compact_table(process_table, process_num_chains);
      printf("%lu.\n", process_num_chains); fflush(stdout);
      source_rts[i].pruned_since_last_compression = 0;
    }
  }
}


int main(int ac, char **av) {
  unsigned int i = 0, num_source_files = 0;
  FILE *f = NULL;
  char *rt_dir = NULL;
  DIR *dir = NULL;
  struct dirent *de = NULL;
  uint64_t *compare_table = NULL;
  unsigned long compare_num_chains = 0;
  unsigned int num_total_rt_files = 0, num_compared_rt_files = 0;

  struct source_rt *source_rts = NULL;
  struct timespec start_time = {0};
  struct stat st = {0};


  if (ac < 3) {
    printf("Usage: %s rt_to_process.rt [rt_to_process2.rt ...] rt_dir/\n", av[0]);
    return 0;
  }

  /* Ensure all arguments except the last one refer to a path to a regular file. */
  for (i = 1; i < ac - 1; i++) {

    if (stat(av[i], &st) < 0) {
      fprintf(stderr, "Error: stat(%s): %s\n", av[i], strerror(errno)); fflush(stderr);
      goto err;
    }

    if ((st.st_mode & S_IFMT) == S_IFREG)
      num_source_files++;
    else {
      fprintf(stderr, "Error: %s is not a file!\n", av[i]); fflush(stderr);
      goto err;
    }
  }

  /* Ensure the last argument is a directory. */
  if (stat(av[ac - 1], &st) < 0) {
    fprintf(stderr, "Error: stat(%s): %s\n", av[ac - 1], strerror(errno)); fflush(stderr);
    goto err;
  }

  if ((st.st_mode & S_IFMT) != S_IFDIR) {
    fprintf(stderr, "Error: %s is not a directory!\n", av[ac - 1]); fflush(stderr);
    goto err;
  }

  /* Load the source files into an array of source_rt structs. */
  source_rts = calloc(num_source_files, sizeof(struct source_rt));
  if (source_rts == NULL) {
    fprintf(stderr, "Failed to allocate buffer.\n"); fflush(stderr);
    goto err;
  }

  start_timer(&start_time);
  for (i = 0; i < num_source_files; i++) {
    source_rts[i].filename = av[i + 1];
    source_rts[i].table = load_rainbow_table(source_rts[i].filename, &source_rts[i].num_chains);
    if (source_rts[i].table == NULL) {
      fprintf(stderr, "Error: failed to load table: %s\n", source_rts[i].filename); fflush(stderr);
      goto err;
    }
  }
  printf("Loaded %u sources files in %.2f seconds.\n\n", num_source_files, get_elapsed(&start_time)); fflush(stdout);

  /* Set reference to compare directory. */
  rt_dir = av[ac - 1];

  /* Prune sources with respect to each other. */
  if (num_source_files > 1) {
    start_timer(&start_time);
    for (i = 0; i < num_source_files; i++) {
      compare_table = source_rts[i].table;
      compare_num_chains = source_rts[i].num_chains;
      prune_tables(&source_rts[i + 1], num_source_files - i - 1, compare_table, compare_num_chains);
    }
    printf("Self-pruned %u sources files in %.2f seconds.\n\n", num_source_files, get_elapsed(&start_time)); fflush(stdout);
  }

  /* Open a handle to the directory. */
  dir = opendir(rt_dir);
  if (dir == NULL) {
    fprintf(stderr, "Error while opening directory: %s: %s (%d)\n", rt_dir, strerror(errno), errno); fflush(stderr);
    goto err;
  }

  /* Count the number of *.rt files. */
  while ((de = readdir(dir)) != NULL) {
    if (str_ends_with(de->d_name, ".rt"))
      num_total_rt_files++;
  }
  rewinddir(dir);
  printf("%u rainbow tables found in %s\n", num_total_rt_files, rt_dir); fflush(stdout);

  while ((de = readdir(dir)) != NULL) {
    char filename[384] = {0};


    if (!str_ends_with(de->d_name, ".rt"))
      continue;

    snprintf(filename, sizeof(filename) - 1, "%s/%s", rt_dir, de->d_name);
    printf("[%u of %u] Comparing %u source files to table: %s...\n", num_compared_rt_files, num_total_rt_files, num_source_files, filename); fflush(stdout);

    start_timer(&start_time);
    compare_table = load_rainbow_table(filename, &compare_num_chains);
    if (compare_table == NULL) {
      fprintf(stderr, "Error: failed to load table: %s\n", filename); fflush(stderr);
      continue;
    }

    prune_tables(source_rts, num_source_files, compare_table, compare_num_chains);
    FREE(compare_table);

    printf("Finished processing in %.2f seconds.\n\n", get_elapsed(&start_time)); fflush(stdout);
    num_compared_rt_files++;
  }
  CLOSEDIR(dir);


  /* Prune duplicates within each table. */
  prune_tables(source_rts, num_source_files, NULL, 0);

  for (i = 0; i < num_source_files; i++) {
    printf("Total pruned chains for %s: %lu; in-table duplicates: %lu\n", source_rts[i].filename, source_rts[i].num_pruned_chains, source_rts[i].in_table_duplicates); fflush(stdout);

    /* If any chains were pruned since we last compressed the table, its time to
     * compress it again. */
    if ((source_rts[i].pruned_since_last_compression > 0) || (source_rts[i].in_table_duplicates > 0))
      source_rts[i].num_chains = compact_table(source_rts[i].table, source_rts[i].num_chains);

    /* If any chains were pruned at all, its time to update the source table. */
    if ((source_rts[i].num_pruned_chains > 0) || (source_rts[i].in_table_duplicates > 0)) {
      if (source_rts[i].num_chains == 0) {
	printf("\n!!! NOTE: resulting file is empty!  Deleting original...\n\n"); fflush(stdout);
	if (unlink(source_rts[i].filename) < 0) {
	  fprintf(stderr, "Failed to delete file: %s: %s (%d)\n", source_rts[i].filename, strerror(errno), errno);
	  goto err;
	}
      } else {
	f = fopen(source_rts[i].filename, "w");
	if (f == NULL) {
	  fprintf(stderr, "Failed to open %s for writing!\n", source_rts[i].filename); fflush(stderr);
	  goto err;
	}

	if (fwrite(source_rts[i].table, sizeof(uint64_t), source_rts[i].num_chains * 2, f) != source_rts[i].num_chains * 2) {
	  fprintf(stderr, "Failed to write to %s!\n", source_rts[i].filename); fflush(stderr);
	  goto err;
	}

	FCLOSE(f);

	/*
	 * Make a copy of the original filename we are processing, since we are
	 * going to modify it. *
	 temp_str = strdup(rt_to_process);
	 if (temp_str == NULL) {
	 fprintf(stderr, "Failed to copy rt_to_process string!\n");  fflush(stderr);
	 goto err;
	 }

	 xpos = strchr(temp_str, 'x');
	 uspos = strrchr(temp_str, '_');
	 if ((xpos != NULL) && (uspos != NULL)) {
	 * Cut off everything after the x character.  Example:
	 * "/path/ntlm_ascii-32-95#8-8_0_422000x67108864_0.rt" -> 
	 * "/path/ntlm_ascii-32-95#8-8_0_422000x" *
	 xpos++;
	 *xpos = '\0';

	 * Make a new filename with the correct number of chains. *
	 snprintf(rt_to_process_new, sizeof(rt_to_process_new) - 1, "%s%lu%s", temp_str, process_num_chains, uspos);

	 * Rename the file using the updated number of chains. *
	 if (rename(rt_to_process, rt_to_process_new) < 0) {
	 fprintf(stderr, "Failed to rename %s to %s!\n", rt_to_process, rt_to_process_new); fflush(stderr);
	 goto err;
	}

	printf("\nSuccessfully wrote pruned table to %s.\n\n", rt_to_process_new); fflush(stdout);
	} else {

	fprintf(stderr, "Failed to parse filename: %s\n", temp_str); fflush(stderr);
	goto err;
	}
	FREE(temp_str);
	*/
	printf("\nSuccessfully wrote pruned table to %s.\n\n", source_rts[i].filename); fflush(stdout);
      }
    } else {
      printf("\n !! WARNING: no chains pruned from %s.\n\n", source_rts[i].filename); fflush(stdout);
    }
  }


  for (i = 0; i < num_source_files; i++) {
    FREE(source_rts[i].table);
  }
  FREE(source_rts);
  return 0;
 err:
  FCLOSE(f);
  CLOSEDIR(dir);

  if (source_rts != NULL) {
    for (i = 0; i < num_source_files; i++) {
      FREE(source_rts[i].table);
    }
    FREE(source_rts);
  }

  FREE(compare_table);
  /*FREE(temp_str);*/
  return -1;
}
