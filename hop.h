/** hop.h
 *
 * Hop-by-hop and destination options header
 *
 * see hop.c for more information
 */
#ifndef _SENDIP_HOP_H
#define _SENDIP_HOP_H

/* hop header */
typedef struct ipv6_opt_hdr hop_header;	/* defined in ipv6ext.h */

/* Generic ipv6 hop-by-hop option structure */
struct ipv6_hopopt {
	u_int8_t	hopt_type;	/* top three bits are "special" */
	u_int8_t	hopt_len;	/* length-2 */
	u_int8_t	hopt_data[];
};

/* Header allocation unit ... */
#define HDR_ALLOC	8


/* Hop option types. I don't know offhand a "standard" place for these. */
#define IPV6_TLV_PAD0           0	/* Actually, pads with 1 byte */
#define IPV6_TLV_PADN           1	/* Pad with N bytes (len=N-2) */
#define IPV6_TLV_ROUTERALERT    5	/* Router alert (len=2) */
#define IPV6_TLV_JUMBO          194	/* Jumbo (len=4, data is jumbo len) */
/* 194 is 110 00010 - option 2 with flags:
 * 	discard if don't understand
 * 	send ICMP parameter problem if not multicast
 * 	cannot change enroute
 */
#define IPV6_TLV_HAO            201     /* home address option (for destination
										 * options header) */
/* 201 is 110 01001 - option 9 with flags:
 * 	discard if don't understand
 * 	send ICMP parameter problem if not multicast
 * 	cannot change enroute
 */


/* Defined router alerts */
#define IPV6_RA_MLD		0	/* multicast listener discovery */
#define IPV6_RA_RSVP	1	/* RSVP */
#define IPV6_RA_AN		2	/* active networks */


/* Defines for which parts have been modified */
#define HOP_LEN_FRAG	(HDR_ALLOC-1)	/* First 3 bits are part of length */

/* Hop-by-hop options we have defined. The flags don't serve
 * much purpose here, but we'll include them for the sake of reference.
 */
#define HOP_MOD_NEXTHDR (1<<3)
#define HOP_MOD_PAD0  	(1<<4)
#define HOP_MOD_PADN  	(1<<5)
#define HOP_MOD_RA    	(1<<6)
#define HOP_MOD_JUMBO 	(1<<7)
#define HOP_MOD_HAO   	(1<<8)
#define HOP_MOD_TLV   	(1<<9)

/* Options */
sendip_option hop_opts[] = {
	{ "n", 1, "Next header protocol", "Correct" },
	{ "0", 0, "Use 1 byte padding", NULL },
	{ "p", 1, "Use N byte padding", "2" },
	{ "r", 1, "Router alert value", "0" },
	{ "j", 1, "Jumbo frame length", "0" },
	{ "h", 1, "Destination", "::1" },
	{ "a", 1, "Arbitrary t.l.v option", "0.0.0" },
};

#endif  /* _SENDIP_HOP_H */

/* vim: ts=4 sw=4 filetype=c
 */
