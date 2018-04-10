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

void addrand(char *output, size_t length);
size_t timestamp(size_t length, char *output);

size_t str2val(char *output, const char *input, size_t outlen);
size_t opt2val(char *output, const char *input, size_t outlen);

u_int32_t opt2intn(const char *input, int length);
u_int32_t opt2inth(const char *input, int length);

in_addr_t cidr2addr(const char *input, char *slashpoint);
in_addr_t opt2v4(const char *input, int length);

int parseargs(char *string, char *args[], const char *seps);
int parsenargs(char *string, char *args[], int limit, const char *seps);

#endif  /* _COMPACT_H */
