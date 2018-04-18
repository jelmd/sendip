/** wesp.h - wrapped ESP header */

#ifndef _SENDIP_WESP_H
#define _SENDIP_WESP_H

#include <asm/byteorder.h>

/* wrapped esp header
 *
 * The WESP header is still an ietf draft
 * (draft-ietf-ipsecme-traffic-visibility-12 as I write this),
 * and has not yet been finalized or had any real Linux
 * implementations that I'm aware of. So we just put everything
 * in here.
 */
#define WESP_VERSION_MASK	0xc0
#define	WESP_ENCRYPTED		0x20
#define	WESP_PADDED			0x10
#define WESP_RESERVED_MASK	0x0f

typedef struct ip_wesp_phdr {
	u_int8_t nexthdr;
	u_int8_t hdrlen;	/* either 0 (encrypted ESP) or WESP+ESP len */
	u_int8_t trlrlen;	/* either 0 (encrypted ESP) or ICV len */
	/* flags: version (2 bits), encrypted, padded */
	/* I did have this as a union, for debugging purposes, but
	 * the Linux standard now seems to be going straight to
	 * the bitfields.
	 */
#if defined(__LITTLE_ENDIAN_BITFIELD) || defined(_LITTLE_ENDIAN)
	u_int8_t	reserved:4,
				padded:1,
				encrypted:1,
				version:2;
#elif defined(__BIG_ENDIAN_BITFIELD) || defined(_BIG_ENDIAN)
	u_int8_t 	version:2,
				encrypted:1,
				padded:1,
				reserved:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
	u_int32_t padding[];
} wesp_header;

#define WESP_MOD_VERSION	(1)
#define WESP_MOD_ENCRYPTED	(1<<1)
#define WESP_MOD_PADDED		(1<<2)
#define WESP_MOD_RESERVED	(1<<3)
#define WESP_MOD_HDRLEN		(1<<4)
#define WESP_MOD_TRLRLEN	(1<<5)
#define WESP_MOD_NEXTHDR	(1<<6)

/* Options */
sendip_option wesp_opts[] = {
	{ "v", 1, "Version", "0" },
	{ "e", 1, "Flag encrypted payload", "0" },
	{ "p", 1, "Flag padded", "0" },
	{ "r", 1, "Reserved field", "0" },
	{ "h", 1, "Header Length", "Correct" },
	{ "t", 1, "Trailer Length", "Correct" },
	{ "n", 1, "Next header protocol", "Correct" },
};

#endif  /* _SENDIP_WESP_H */

/* vim: ts=4 sw=4 filetype=c
 */
