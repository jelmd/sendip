/* compact.c - function to convert hex/octal/decimal/raw string to raw
 * ChangeLog since initial release in sendip 2.1.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#ifdef __linux
#include <bsd/string.h>
#endif

#include "fargs.h"
#include "compact.h"

size_t
compact_string(const char *input, char *output, size_t buflen) {
	const char *p = input + 1;
	char *s = output;
	size_t max = buflen;
	size_t i;

	if (*input == '0' && (*p == 'x' || *p == 'X')) {
		p++;
		/* Hex */
		i = (strlen(p) + 1) >> 1;
		if (i > buflen) {
			DWARN("output buffer is too small to comsume the whole input "
				"of %ld hex bytes. Skipping last %ld hex bytes!", i, i - max)
		}
		i = 0;
		char c = '\0';
		while (*p && max != 0) {
			if (*p >= '0' && *p <= '9') {
				c += *p - '0';
			} else if (*p >= 'A' && *p <= 'F') {
				c += *p - 'A' + 10;
			} else if(*p >= 'a' && *p <= 'f') {
				c += *p - 'a' + 10;
			} else {
				DERROR("Character %c invalid in hex data stream", *p)
				return 0;
			}
			if (i & 1) {
				*(s++) = c;  // odd nibble - output it
				c = '\0';
				max--;
			} else {
				c <<= 4;   // even nibble - shift to top of byte
			}
			p++;
			i++;
		}
		if (i & 1 && max != 0) {
			*s = c; // make sure last nibble is added
			max--;
		}
		return buflen - max;
	}

	if (*input == '0') {
		/* Octal */
		char c = '\0';
		i = (strlen(p) + 3) >> 2;
		if (i > buflen) {
			DWARN("output buffer is too small to comsume the whole input "
				"of %ld octo bytes. Skipping last %ld octo bytes!", i, i - max)
		}
		i = 0;
		while (*p && max != 0) {
			if (*p >= '0' && *p <= '7') {
				c += *p - '0';
			} else {
				DERROR("Character %c invalid in octo data stream", *p)
				return 0;
			}
			if( (i & 3) == 3 ) {
				*(s++) = c;  // output every 4th char
				c = '\0';
				max--;
			} else {        // otherwise just shift it up
				c <<= 2;
			}
			p++;
			i++;
		}
		if (i & 3 && max != 0) {
			*s = c;     // add partial last byte
			max--;
		}
		return buflen - max;
	}

	/* String */
	if ((i = strlcpy(output, input, buflen)) >= buflen) {
		DWARN("output buffer is too small, skipping '%s' of '%s'",
			(input + buflen), input)
	}
	return i;
}

/* Generate @length random bytes and copy them to @output, which is expected
 * to be large enough (no check for overflow of the array).
 */
void
randombytes(size_t length, char *output)
{
	static unsigned short xsubi[3];
	u_int32_t random32;

	int n = length >> 2, i;

	/* random() returns 31 random bits, only. So we use jrand48(), which gives
	 * us the 32 bit random range we need. */
	for (i = 0; i < n; i++) {
		random32 = jrand48(xsubi);
		memcpy(output, &random32, 4);
		output += 4;
	}
	length -= n << 2;
	if (length != 0) {
		random32 = jrand48(xsubi);
		memcpy(output, &random32, length);
	}
}

/* Write the time in host byte order to @output and pad remaining bytes with 0.
 * @output is expected to have a least a size of @length  bytes (check for
 * overflow of the array). If @length is smaller than the size of a timestamp,
 * only the first @length bytes of the timestamp get copied over to the @output.
 * @return number of bytes copied to @output .
 */
size_t
timestamp(size_t length, char *output)
{
	struct timeval ts;

	if (gettimeofday(&ts, NULL) == 0)
		return 0;
	if (sizeof(ts) > length) {
		memcpy(output, &ts, length);
	} else {
		memcpy(output, &ts, sizeof(ts));
		memset(output + sizeof(ts), 0, length - sizeof(ts));
	}
	return length;
}

/* Yes, well, not the world's most brilliant name, but this
 * does the standard string argument handling. The @output is expected to
 * has at least a size @outlen bytes.
 * @return The number of bytes copied to @output.
 */
size_t
stringargument(const char *input, char *output, size_t outlen)
{
	u_int32_t len = 0;

	if (input == NULL || output == NULL)
		return 0;

	/* Special case for f*, rN, zN, tN strings */
	if (*input == 'f') {
		char *data =  fileargument(input + 1);
		if (data != NULL)
			return stringargument(data , output, outlen);
	}

	if ((*input == 'r' || *input == 'z') && isdigit(*(input + 1))) {
		len = atoi(input + 1);
		if (len == 0)
			return 0;
		if (len > outlen) {
			DWARN("output buffer to small. Generating only %ld %s bytes",
				outlen, (*input == 'r' ? "random" : "zero"))
			len = outlen;
		}
		if (*input == 'r') {
			randombytes(len, output);
		} else {
			memset(output, 0, len);
		}
		return len;
	}

	if (*input == 't' && isdigit(*(input + 1))) {
		len = atoi(input + 1);
		if (len > outlen) {
			DWARN("output buffer to small. "
				"Reducing timestamp from %d to %ld bytes", len, outlen)
			len = outlen;
		}
		return timestamp(len, output);
	}

	/* read hex/octal/decimal/raw string */
	return compact_string(input, output, outlen);
}

/* Extracts a decimal, octal, hex integer of from the given @input string or
 * generate one using the special strings rX (random bytes) or zX (zero bytes),
 * converts it to an integer with the specified number of bytes in *network*
 * byte order and returns it. Since an integer has not more than 4 bytes,
 * 4 will be used instead of @length, if it is bigger than that, and @length
 * overrules X if it is smaller than X. The idea is you can just do:
 *
 * 	field = integerargument(input, sizeof(field));
 */
u_int32_t
integerargument(const char *input, int length)
{
	if (input == NULL || length < 1)
		return 0;

	/* Special case for f*, rN, zN strings */
	if (*input == 'f')
		return integerargument(fileargument(input + 1), length);

	if (*input == 'r' && isdigit(*(input + 1))) {
		u_int32_t r;
		int len = atoi(input + 1);
		if (len > length) {
			DWARN("Reducing number of random bytes for an int from %d to %d",
				len, length);
			len = length;
		}
		randombytes(len, (char *) &r);

		/* There's no point in byte-swapping random bytes */
		switch (length) {
			case 1:
				return (u_int8_t) r;
			case 2:
				return (u_int16_t) r;
			case 3:
				return (0x00ffffff & r);
			default:
				return r;
		}
	}
	if (*input == 'z')
		return 0;

	/* Everything else, just use strtoul, then cast and swap */
	if (length == 1)
		return (u_int8_t) strtoul(input, (char **) NULL, 0);

	return (length == 2)
		? htons((u_int16_t) strtoul(input, (char **) NULL, 0))
		: htonl(strtoul(input, (char **) NULL, 0));
}


/* same as above, except the result is in host byte order */
u_int32_t
hostintegerargument(const char *input, int length)
{
	if (input == NULL || length < 1)
		return 0;

	/* Special case for f*, rN, zN strings */
	if (*input == 'f')
		return hostintegerargument(fileargument(input + 1), length);

	if (*input == 'r' && isdigit(*(input + 1))) {
		u_int32_t r;
		int len = atoi(input + 1);
		if (len > length) {
			DWARN("Reducing number of random bytes for an int from %d to %d",
				len, length)
			len = length;
		}
		randombytes(len, (char *) &r);

		switch (length) {
			case 1:
				return (u_int8_t) r;
			case 2:
				return (u_int16_t) r;
			case 3:
				return (0x00ffffff & r);
			default:
				return r;
		}
	}

	if (*input == 'z')
		return 0;

	/* Everything else, just use strtoul, then cast */
	if (length == 1)
		return (u_int8_t) strtoul(input, (char **) NULL, 0);

	return (length == 2)
		? (u_int16_t) strtoul(input, (char **) NULL, 0)
		: strtoul(input, (char **) NULL, 0);
}

/* IPv4 dotted decimal arguments can be specified in several ways.
 * First off, you can use the rN random arguments as above:
 * 	10.1.2.r1 - random address within this /24
 * 	10.1.r2 - random address within this /16
 * 	10.r3 - random 10. address
 * 	r4 - completely random IPv4 address.
 *
 * Secondly, and probably more usefully, you can specify addresses with
 * CIDR notation, implicitly requesting a random address within the given
 * subnet. So, for example:
 * 	10.1.2.0/24 - same result as 10.1.2.r1 (except leaves out 0 and 255)
 * 	10.1.2.0/26 - address in the range 10.1.2.1 to 10.1.2.62
 * Note the CIDR specification won't generate either a 0 or all 1s (broadcast)
 * in the host portion of the address, while the rN method above can.
 *
 * Finally, you can use file arguments - most useful when you are using
 * looping, and working through a list of addresses. The addresses can
 * include the random or CIDR-type specifications above.
 *
 * If you're wondering why the different types, the answer is that I
 * only did the first type at first, because it was easiest to implement,
 * but then needed the others, with the noted restrictions, for a
 * particular project.
 *
 * @return the parsed address, in network byte order.
 */
in_addr_t
cidrargument(const char *input, char *slashpoint)
{
	static char ipv4space[BUFSIZ]; /* actual max around 40 */
	in_addr_t host;
	in_addr_t hmask, smask;
	struct in_addr cidrarg;
	int bits;

	strncpy(ipv4space, input, slashpoint - input);
	ipv4space[slashpoint - input] = '\0';
	inet_pton(AF_INET, ipv4space, &cidrarg);
	bits = strtol(++slashpoint, NULL, 0);
	/* Interpret weird /xx values as fixed addresses */
	if (bits <= 0 || bits >= 32)
		return cidrarg.s_addr;

	/* The host and subnet mask, in host order for now */
	hmask = (1 << (32 - bits)) - 1;
	smask = ~hmask;

	/* Get the random host part */
	do {
		if (bits < 8) {
			host = integerargument("r4", 4);
		} else if (bits < 16) {
			host = integerargument("r3", 3);
		} else if (bits < 24) {
			host = integerargument("r2", 2);
		} else {
			host = integerargument("r1", 1);
		}
		host &= hmask;
	} while (host == 0 || host == hmask); /* Don't allow all 0 or all 0xff */

	/* Now fold the random host into the output */
	return ((cidrarg.s_addr & htonl(smask)) | htonl(host));
}

/* This is the IPv4 dotted decimal version of the above. Here,
 * each period-separated field is interpreted via integerargument.
 * Note that "dotted decimal" includes not just those of the form
 * aa.bb.cc.dd, but can also be aa.bb.cccc (i.e., last two bytes
 * are specified as one 16-bit integer), aa.bbbbbb (last three bytes
 * specified as one 24-bit integer), and even aaaaaaaa (entirely
 * specified as one 32-bit integer). This allows specifying addresses
 * such as:
 * 	10.1.2.r1 - random address within this /24
 * 	10.1.r2 - random address within this /16
 * 	10.r3 - random 10. address
 *
 * @return the address, in network byte order.
 */
in_addr_t
ipv4argument(const char *input, int length)
{
	char ipv4space[BUFSIZ]; /* actual max around 40 */
	u_int32_t a, b, c, d;
	char *dotpoint, *slashpoint;

	/* Special case for f* strings */
	if (*input == 'f')
		return ipv4argument(fileargument(input + 1), length);

	/* Special case for CIDR notation */
	if ((slashpoint = strchr(input, '/')) != NULL)
		return cidrargument(input, slashpoint);

	/* This covers the rN and fixed methods of address specification */
	if ((dotpoint = strchr(input, '.')) == NULL)	/* aaaaaaaa */
		return integerargument(input, 4);	/* in network order */

	a = hostintegerargument(input, 1);
	input = dotpoint;
	++input;
	length -= dotpoint - input;
	if ((dotpoint = strchr(input, '.')) == NULL) {	/* aa.bbbbbb */
		b = hostintegerargument(input, 3);
		sprintf(ipv4space, "%d.%d", a, b);
		return inet_addr(ipv4space);
	}

	b = hostintegerargument(input, 1);
	input = dotpoint;
	++input;
	length -= dotpoint - input;
	if ((dotpoint = strchr(input, '.')) == NULL) {	/* aa.bb.cccc */
		c = hostintegerargument(input, 2);
		sprintf(ipv4space, "%d.%d.%d", a, b, c);
		return inet_addr(ipv4space);
	}

	c = hostintegerargument(input, 1);
	input = dotpoint;
	++input;
	length -= dotpoint - input;
	d = hostintegerargument(input, 1);
	sprintf(ipv4space, "%d.%d.%d.%d", a, b, c, d);
	return inet_addr(ipv4space);
}
