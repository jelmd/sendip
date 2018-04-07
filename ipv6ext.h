#ifndef _IPV6EXT_H
#define _IPV6EXT_h

#include "types.h"

/* The following are taken from a variety of Linux kernel header
 * files, rearranged and reworked for our purposes here.
 */

/* hop-by-hop and destination options header */
struct ipv6_opt_hdr {
	u_int8_t		nexthdr;
	u_int8_t		hdrlen;
	/*
	 * TLV encoded option data follows.
	 */
};

#define ipv6_destopt_hdr ipv6_opt_hdr
#define ipv6_hopopt_hdr  ipv6_opt_hdr

/*
 *	home address option in destination options header
 */

struct ipv6_destopt_hao {
	u_int8_t	type;
	u_int8_t	length;
	struct in6_addr	addr;
};

/*
 *	fragmentation header
 */

struct ipv6_frag_hdr {
	u_int8_t	nexthdr;
	u_int8_t	reserved;
	u_int16_t	frag_off;   /* fragment offset - 13 bits + 3 flags */
	u_int32_t	identification;
};


/*
 *	routing header
 */
#define IPV6_SRCRT_STRICT	0x01	/* Deprecated; will be removed */
#define IPV6_SRCRT_TYPE_0	0	/* Deprecated; will be removed */
#define IPV6_SRCRT_TYPE_2	2	/* IPv6 type 2 Routing Header	*/
struct ipv6_rt_hdr {
	u_int8_t		nexthdr;
	u_int8_t		hdrlen;
	u_int8_t		type;
	u_int8_t		segments_left;

	/*
	 *	type specific data
	 *	variable length field
	 */
};

/*
 *	routing header type 0 (used in cmsghdr struct)
 */

struct rt0_hdr {
	struct ipv6_rt_hdr	rt_hdr;
	u_int32_t		reserved;
	struct in6_addr		addr[];

#define rt0_type		rt_hdr.type
};

/*
 *	routing header type 2
 */

struct rt2_hdr {
	struct ipv6_rt_hdr	rt_hdr;
	u_int32_t		reserved;
	struct in6_addr		addr;

#define rt2_type		rt_hdr.type
};

/*
 * authentication header
 */
struct ip_auth_hdr {
	u_int8_t  nexthdr;
	u_int8_t  hdrlen;	/* This one is measured in 32 bit units! */
	u_int16_t reserved;
	u_int32_t spi;
	u_int32_t seq_no;	/* Sequence number */
	u_int8_t  auth_data[];	/* Variable len but >=4. Mind the 64 bit alignment! */
#define	icv	auth_data	/* rfc 4302 name */
	/* TBD - high-order sequence number */
};

/*
 * encapsulated security payload header
 */
struct ip_esp_hdr {
	u_int32_t spi;
	u_int32_t seq_no;	/* Sequence number */
	u_int8_t  enc_data[];	/* Variable len but >=8. Mind the 64 bit alignment! */
};


/*
 * compression header
 */
struct ip_comp_hdr {
	u_int8_t nexthdr;
	u_int8_t flags;
	u_int16_t cpi;
};

/*
 * beet header
 */
struct ip_beet_phdr {
	u_int8_t nexthdr;
	u_int8_t hdrlen;
	u_int8_t padlen;
	u_int8_t reserved;
};

/* miscellanea */
/* for code documentation purposes, largest u_int8_t (8-bit unsigned) value */
#define OCTET_MAX	255
/* Largest 4-bit unsigned value */
#define QUARTET_MAX	15
/* Largest 2-bit unsigned value */
#define DUO_MAX		3
/* Largest 1-bit value is left as an exercise for the reader */

#endif
