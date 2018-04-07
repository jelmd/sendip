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

#include "fargs.h"
#include "compact.h"

u_int32_t
compact_string(char *data_out) {
	char *data_in = data_out;
	u_int32_t i = 0;
	if (*data_in == '0') {
		data_in++;
		if (*data_in == 'x' || *data_in == 'X') {
			/* Hex */
			char c = '\0';
			data_in++;
			while (*data_in) {
				if (*data_in >= '0' && *data_in <= '9') {
					c += *data_in - '0';
				} else if (*data_in >= 'A' && *data_in <= 'F') {
					c += *data_in - 'A' + 10;
				} else if(*data_in >= 'a' && *data_in <= 'f') {
					c += *data_in - 'a' + 10;
				} else {
					fprintf(stderr, "Character %c invalid in hex data stream\n",
							  *data_in);
					return 0;
				}
				if (i & 1) {
					*(data_out++) = c;  // odd nibble - output it
					c = '\0';
				} else {
					c <<= 4;   // even nibble - shift to top of byte
				}
				data_in++;
				i++;
			}
			*data_out = c; // make sure last nibble is added
			i++;
			i >>= 1;  // i was a nibble count...
			return i;
		} else {
         /* Octal */
			char c = '\0';
			while (*data_in) {
				if(*data_in >= '0' && *data_in <= '7') {
					c += *data_in - '0';
				} else {
					fprintf(stderr,
						"Character %c invalid in octal data stream\n",*data_in);
					return 0;
				}
				if( (i & 3) == 3 ) {
					*(data_out++) = c;  // output every 4th char
					c = '\0';
				} else {        // otherwise just shift it up
					c <<= 2;
				}
				data_in++;
				i++;
			}
			*data_out = c;     // add partial last byte
			i += 3;
			i >>= 2;
			return i;
		}
	} else {
		/* String */
		return strlen(data_in);
	}
}


/* Functions for filling out several header data areas using the
 * "string, rand or zero" business.
 *
 * Note the handling of space is slightly screwy - compact_string
 * above overwrites its argument in place, since it knows that
 * no matter what, the string it produces can be no longer than
 * its argument. randombytes and zerobytes, however, uses a static area, since
 * the calling argument there (something like r32) will generally
 * be much shorter than the string produced.
 *
 * In practice, in both cases the string returned will be immediately
 * copied into an allocated area, so the differences in string handling
 * don't matter. But this should be kept in mind if these routines
 * are used elsewhere.
 */

/* @return a pointer to a string of random bytes. Note this is a
 * static area which is periodically overwritten.
 */
u_int8_t *
randombytes(int length)
{
	static unsigned short xsubi[3];
	static union {
		u_int32_t random32[MAXRAND >> 2];
		u_int8_t random8[MAXRAND];
	} store;

	int i;

	/* Sanity check */
	if (length > MAXRAND) {
		usage_error("Random data too long to be sane\n");
		return NULL;
	} else if (length == 0) {
		return NULL;
	}
	/* random() returns 31 random bits, only. So we use jrand48(), which gives
	 * us the 32 bit random range we need. */
	for (i = ((length + 3) >> 2) - 1; i >= 0; i--)
		store.random32[i] = (u_int32_t) jrand48(xsubi);

	return &store.random8[0];
}

/* @return a pointer to a string of zero bytes. Note this is a
 * static area which should be used in read-only mode!
 */
u_int8_t *
zerobytes(int length)
{
	static u_int8_t *s = NULL;

	if (s == NULL)
		s = calloc(MAXRAND, sizeof(u_int8_t));

	/* Sanity check */
	if (length > MAXRAND) {
		usage_error("Zero data too long to be sane\n");
		return NULL;
	}
	return s;
}

/* Write the time in host byte order onto the beginning of a zero-padded
 * static buffer of BUFSIZ bytes. Therefore it should be used in read-only mode!
 * @return The pointer to the beginning of the timestamp buffer.
 */
u_int8_t *
timestamp(int length)
{
	static u_int8_t *s = NULL;

	if (s == NULL)
		s = calloc(BUFSIZ, sizeof(u_int8_t));

	/* Sanity check */
	if (length > BUFSIZ) {
		usage_error("Time data too long to be sane\n");
		return NULL;
	}
	gettimeofday((struct timeval *) s, NULL);
	return s;
}

/* Yes, well, not the world's most brilliant name, but this
 * does the standard string argument handling. The output
 * may either be the transformed input or a static area.
 * @return The length of the output.
 */
u_int32_t
stringargument(char *input, char **output)
{
	u_int32_t len = 0;

	if (input == NULL || output == NULL)
		return 0;

	/* Special case for f*, rN, zN, tN strings */
	if (*input == 'f') {
		char *data =  fileargument(input + 1);
		if (data != NULL)
			return stringargument(data , output);
	}

	if ((*input == 'r' || *input == 'z') && isdigit(*(input + 1))) {
		len = atoi(input + 1);
		*output = (char *)
			((*input == 'r') ? randombytes(len) : zerobytes(len));
		return (*output) ? len : 0;
	}

	if (*input == 't' && isdigit(*(input + 1))) {
		len = atoi(input+1);
		*output = (char *) timestamp(len);
		return (*output == NULL) ? 0 : len;
	}

	/* read hex/octal/decimal/raw string */
	len = compact_string(input);
	*output = input;
	return len;
}

/* This is the integer (1, 2 or 4 byte) version of the above. It takes
 * the input, which may be decimal, octal, hex, or the special strings
 * rX (random bytes) or zX (zero bytes - kind of pointless) and converts
 * it to an integer with the specified number of bytes in *network* byte
 * order. The idea is you can just do:
 *
 * 	field = integerargument(input, sizeof(field));
 */
u_int32_t
integerargument(const char *input, int length)
{
	int inputlength;
	u_int8_t *string;

	if (input == NULL || length < 1)
		return 0;

	/* Special case for f*, rN, zN strings */
	if (*input == 'f')
		return integerargument(fileargument(input + 1), length);

	if (*input == 'r' && isdigit(*(input + 1))) {
		inputlength = atoi(input + 1);
		if (inputlength > length)
			inputlength = length;
		string = randombytes(inputlength);
		if (!string)
			return 0;

		/* There's no point in byte-swapping random bytes */
		switch (length) {
			case 1:
				return (u_int8_t) *string;
			case 2:
				return *(u_int16_t *) string;
			case 3:
				return (0x00ffffff & *(u_int32_t *) string);
			default:
				return *(u_int32_t *) string;
		}
	}
	if (*input == 'z')
		return 0;	/* like I said, pointless ... */

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
	int inputlength;
	u_int8_t *string;

	if (input == NULL || length < 1)
		return 0;

	/* Special case for f*, rN, zN strings */
	if (*input == 'f')
		return hostintegerargument(fileargument(input + 1), length);

	if (*input == 'r' && isdigit(*(input + 1))) {
		inputlength = atoi(input + 1);
		if (inputlength > length)
			inputlength = length;
		string = randombytes(inputlength);
		if (string == NULL)
			return 0;

		switch (length) {
			case 1:
				return (u_int8_t) *string;
			case 2:
				return *(u_int16_t *) string;
			case 3:
				return (0x00ffffff & *(u_int32_t *) string);
			default:
				return *(u_int32_t *) string;
		}
	}

	if (*input == 'z') {
		/* like I said, pointless ... */
		return 0;
	}

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
	int slash;

	strncpy(ipv4space, input, slashpoint - input);
	ipv4space[slashpoint - input] = '\0';
	inet_pton(AF_INET, ipv4space, &cidrarg);
	slash = atoi(++slashpoint);
	/* Interpret weird /xx values as fixed addresses */
	if (slash <= 0 || slash >= 32)
		return cidrarg.s_addr;
	/* The host and subnet parts, in host order for now */
	hmask = (1 << (32 - slash)) - 1;
	smask = ~hmask;
	/* Determine how much randomness we need, and get it. */
	/* Don't allow 0 or ffff.. host parts if we can help it.
	* We can help it so long as slash < 31.
	*/
	do {
		if (slash < 8) {
			host = integerargument("r4", 4);
		} else if (slash < 16) {
			host = integerargument("r3", 3);
		} else if (slash < 24) {
			host = integerargument("r2", 2);
		} else {
			host = integerargument("r1", 1);
		}
		host &= hmask;
	} while (slash < 31 && (host == 0 || host == hmask));
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
