/** csum.c - Computes the standard internet checksum of a set of data
 * (see RFC1071)
 */

#define __USE_BSD    /* GLIBC */
#define _DEFAULT_SOURCE  /* LIBC5 */
#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "csum.h"

/* Checksum a block of data */
u_int16_t
csum (u_int16_t *packet, int packlen) {
	register unsigned long sum = 0;

	while (packlen > 1) {
		sum += *(packet++);
		packlen -= 2;
	}
	if (packlen > 0)
		sum += *(unsigned char *) packet;

	/* TODO: this depends on byte order */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return (u_int16_t) ~sum;
}

/* Checksum a vector of blocks of data */
u_int16_t
csumv (u_int16_t *packet[], int packlen[]) {
	register unsigned long sum = 0;
	int i;

	for (i = 0; packlen[i]; ++i) {
		while (packlen[i] > 1) {
			sum += *(packet[i]++);
			packlen[i] -= 2;
		}
		if (packlen[i] > 0)
			sum += *(unsigned char *)packet[i];
	}

	/* TODO: this depends on byte order */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return (u_int16_t) ~sum;
}

/* vim: ts=4 sw=4 filetype=c
 */
