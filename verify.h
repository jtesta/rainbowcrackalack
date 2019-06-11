#ifndef _VERIFY_H
#define _VERIFY_H

#include <inttypes.h>


#define VERIFY_TABLE_TYPE_LOOKUP 1
#define VERIFY_TABLE_TYPE_GENERATED 0

#define VERIFY_TABLE_IS_COMPLETE 1
#define VERIFY_TABLE_MAY_BE_INCOMPLETE 0

#define VERIFY_TRUNCATE_ON_ERROR 1
#define VERIFY_DONT_TRUNCATE 0


int verify_rainbowtable(uint64_t *rainbowtable, unsigned int num_chains, unsigned int table_type, uint64_t expected_start, uint64_t plaintext_space_total, unsigned int *error_chain_num);

int verify_rainbowtable_file(char *filename, unsigned int table_type, unsigned int table_should_be_complete, unsigned int truncate_at_error, int num_chains_to_verify);


#endif
