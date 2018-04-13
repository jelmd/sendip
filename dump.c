/** dump.c
 * License: MIT
 * (C)opyright 2018 Jens Elkner, Otto-von-Guericke University Magdeburg, Germany
 */

#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include "parseargs.h"		/* macros only */

#include "dump.h"

#define BPL 77		/* we consume 77 byte/line  => 77 byte /16 byte chunk */


/* Hex dump the @data in chunks of 16 bytes.
 * @data	data to dump
 * @dlen	number of bytes to dump
 * @buf		where to store the dump
 * @buflen	size of the @buf. If the size is < @dlen, @dlen get adjusted
 * 			automagically.
 * @hex		if 0, print offset in decimal, otherwise in hex.
 * @return the number of characters written excuding the terminating '\0'.
 */
size_t
bdump(const uint8_t *data, size_t dlen, char *buf, size_t buflen, int hex) {
	const uint8_t *q, *dend;
	const char *fmt, *fmt_head;
   	char *p, *e;
	size_t d, max, i;
	uint8_t c;

	const char H2C[] = "0123456789abcdef";

	if (hex) {
		fmt = "%8X: ";
		fmt_head = "          00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F";
	} else {
		fmt = "%8ld: ";
		fmt_head = "          00 01 02 03 04 05 06 07  08 09 19 11 12 13 14 15";
	}

	max = (buflen - strlen(fmt_head) - 2 - 1) / BPL * 16;	/* "\n\n\0" */
	if (max == 0) {
		DWARN("Dump buffer too small - expecting at least %ld",
			strlen(fmt_head) + 3 + BPL)
		return 0;
	}

	if (dlen > max) {
		DWARN("Dump buffer too small - dumping %ld of %ld bytes", max, dlen)
		dlen = max;
	}
	/* No one wants to dump 16 MiB+ */
	if (dlen > 0xFFFFFF) {
		WARN("Reducing dump size to 16 MiB");
		dlen = 0xFFFFFF;
	}
	dend = data + dlen;

	snprintf(buf, buflen, "%s\n\n", fmt_head);
	max = strlen(buf);
	p = buf + max;
	e = p + (BPL - 16 - 1);				/* offset for char decoding */
	q = data;
	for (d = 0; d < dlen; d += 16) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
		p += sprintf(p, fmt, d);
#pragma GCC diagnostic pop
		for (i = 0; i < 16 && q < dend; i++, q++) {
			c = *q;
			if (c < 15) {
				*p++ = '0';
				*p++ = H2C[c];
				*e++ = '.';
			} else {
				*p++ = H2C[(c >> 4) & 0x0F];
				*p++ = H2C[c & 0x0F];
				// UTF-8 terminals print a wc for 127, so we skip it
				 *e++ =  (c < 32 || c > 126) ? '.' : c;
			}
			*p++ = ' ';
			if (i == 7)
				*p++ = ' ';
		}
		if (i == 16) {
			*p++ = ' ';
			*e++ = '\n';
			p += 16 + 1;
			e += (BPL - 16 - 1);
		}
	}
	/* finish the line (may contain garbage) */
	max = dlen & 0xF;
	if (max != 0) {
		if (max < 8 )
			*p++ = ' ';
		for (i = max; i < 16; i++) {
			*p++ = ' '; *p++ = ' '; *p++ = ' '; *e++ = ' ';
		}
		*p++ = ' ';
		*e++ = '\n';
	}
	*e = '\0';
	return e - buf;
}

#ifdef TEST
int
main(int argc, char **argv) {
	char bla[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	char buf[BUFSIZ];
	uint32_t n[] = { 0x00006000, 0x3b200000, 0, 0, 0, 1, 0, 0, 0, 0, 0 };

	size_t w = bdump(bla, sizeof(bla), buf, BUFSIZ, 1);
	printf("%s\n%ld bytes written\n\n\n", buf, w);

	w = bdump(bla, sizeof(bla), buf, BUFSIZ, 0);
	printf("%s\n%ld bytes written\n", buf, w);

	w = bdump((char *) n, sizeof(n), buf, BUFSIZ, 0);
	printf("%s\n%ld bytes written\n", buf, w);
}
#endif
