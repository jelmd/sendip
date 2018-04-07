#ifndef _COMPACT_H
#define _COMPACT_H

#include <netinet/in.h>
#include "types.h"

#include <stdio.h>
#define usage_error(x)	fprintf(stderr, (x))

#define MAXRAND 8192    /* maximum length of random data */

u_int32_t compact_string(char *data_out);
u_int8_t * randombytes(int length);
u_int32_t stringargument(char *input, char **output);
u_int32_t integerargument(const char *input, int length);
u_int32_t hostintegerargument(const char *input, int length);
in_addr_t ipv4argument(const char *input, int length);
in_addr_t cidrargument(const char *input, char *slashpoint);

#endif  /* _COMPACT_H */
