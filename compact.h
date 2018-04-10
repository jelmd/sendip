#ifndef _COMPACT_H
#define _COMPACT_H

#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include "types.h"

#define usage_error(x)	fprintf(stderr, (x))

#define DERROR(...) \
	fprintf(stderr, "ERROR: "); \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n");

#define DWARN(...) \
	fprintf(stderr, "WARNING: "); \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n");

#define PERROR(...) { \
	int err = errno; \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, ": %s\n", err == 0 ? "Error 0" : strerror(err)); \
	errno = 0; \
}

#define ERROR(x) fprintf(stderr, "ERROR: %s\n", x);
#define WARN(x) fprintf(stderr, "WARNING: %s\n", x);

#define MAXRAND 8192    /* maximum length of random data */

size_t compact_string(const char *input, char *output, size_t outlen);
void randombytes(size_t length, char *output);
size_t timestamp(size_t length, char *output);
size_t stringargument(const char *input, char *output, size_t outlen);
u_int32_t integerargument(const char *input, int length);
u_int32_t hostintegerargument(const char *input, int length);
in_addr_t ipv4argument(const char *input, int length);
in_addr_t cidrargument(const char *input, char *slashpoint);

#endif  /* _COMPACT_H */
