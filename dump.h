#ifndef _DUMP_H
#define _DUMP_H

#include <sys/types.h>

size_t bdump(const uint8_t *data, size_t dlen, char *buf, size_t buflen, int hex);

#endif /* _DUMP_H */
