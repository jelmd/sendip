/** gre.h
 */
#ifndef _SENDIP_GRE_H
#define _SENDIP_GRE_H

#include "types.h"

/* Linux has a tunnel structure, but it is a generic version from
 * which the actual GRE (or whatever) header is constructed during
 * transmit time. So we'll just hack one out.
 */
typedef struct {
	u_int16_t	gre_flag;	/* Flags and version number */
	u_int16_t	gre_protocol;	/* Ethernet protocol value */
	union {
		u_int16_t	sixteen[1];	/* 16-bit add-on fields */
		u_int32_t	thirtytwo[1];	/* 32-bit add-on fields */
	} gre_info;
} gre_header;

/* These two have a fixed location (in 16-bit words) */
#define	GRE_CHECKSUM_FIELD	0	/* First add-on, 16-bit checksum */
#define GRE_OFFSET_FIELD	1	/* Second add-on, 16-bit offset */

/* Other fields are variable, depending on which options are already
 * in place, so we have a subroutine (gre_where) to determine their
 * location.
 */

/* Taken from Linux headers, GRE flag/field defines */
#define GRE_CSUM        (0x8000)
#define GRE_ROUTING     (0x4000)
#define GRE_KEY         (0x2000)
#define GRE_SEQ         (0x1000)
#define GRE_STRICT      (0x0800)
#define GRE_REC         (0x0700)
#define GRE_FLAGS       (0x00F8)
#define GRE_VERSION     (0x0007)

#define GRE_MAX_REC		7	/* three-bit value */
#define GRE_MAX_VERSION	7	/* three-bit value */
#define GRE_REC_SHIFT	3

/* Taken from Linux headers, (Ethernet) protocol defines */
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_P_IPV6	0x86DD		/* IPv6 over bluebook		*/
/* The following (plus any others) are TBD ... */
#define ETH_P_ARP	0x0806		/* Address Resolution packet	*/
#define ETH_P_IPX	0x8137		/* IPX over DIX			*/


/* Defines for which parts have been modified */
#define GRE_MOD_CHECKSUM  	(1)
#define GRE_MOD_ROUTING		(1<<1)
#define GRE_MOD_KEY			(1<<2)
#define GRE_MOD_SEQUENCE	(1<<3)
#define GRE_MOD_STRICT		(1<<4)
#define GRE_MOD_RECURSION	(1<<5)
#define GRE_MOD_VERSION		(1<<6)
#define GRE_MOD_PROTOCOL	(1<<7)
#define GRE_MOD_OFFSET		(1<<8)

/* Options. Let's use
 * 	lower case - needs value
 * 	upper case - doesn't need value
 */
sendip_option gre_opts[] = {
	{ "c", 1, "Checksum", NULL },
	{ "C", 0, "Calulate and add checksum when finalizing", "0" },
	{ "r", 1, "Routing field", NULL },
	{ "k", 1, "Key", NULL },
	{ "s", 1, "Sequence number", NULL },
	{ "S", 0, "Strict source routing flag on", "0" },
	{ "e", 1, "Recursion encapsulation limit", "0" },
	{ "v", 1, "Version number", "0" },
	{ "p", 1, "Encapsulated protocol", "Correct (if known)" },
	{ "o", 1, "Offset", NULL },
};

#endif  /* _SENDIP_GRE_H */

/* vim: ts=4 sw=4 filetype=c
 */
