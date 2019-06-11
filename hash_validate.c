/*
 * Rainbow Crackalack: hash_validate.c
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

#include <string.h>
#include "hash_validate.h"
#include "shared.h"


cl_uint hash_str_to_type(char *hash_str) {
  unsigned int ret = HASH_UNDEFINED;


  if (strcmp(hash_str, "lm") == 0)
    ret = HASH_LM;
  else if (strcmp(hash_str, "ntlm") == 0)
    ret = HASH_NTLM;

  return ret;
}
