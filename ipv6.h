/** ipv6.h */
#ifndef _SENDIP_IPV6_H
#define _SENDIP_IPV6_H

/* Pseudo header used for checksumming ICMP, TCP, UDP etc */
struct ipv6_pseudo_hdr {
	struct in6_addr source;
	struct in6_addr destination;
	u_int32_t ulp_length;
	u_int32_t  zero: 24,
			nexthdr:  8;
};

/* Header taken from glibc 2.2
 */
typedef struct {
	union  {
		struct ip6_hdrctl {
			uint32_t ip6_un1_flow;	/* 24 bits of flow-ID */
			uint16_t ip6_un1_plen;	/* payload length */
			uint8_t  ip6_un1_nxt;	/* next header */
			uint8_t  ip6_un1_hlim;	/* hop limit */
		} ip6_un1;
		uint8_t ip6_un2_vfc;		/* 4 bits version, 4 bits priority */
	} ip6_ctlun;
	struct in6_addr ip6_src;	/* source address */
	struct in6_addr ip6_dst;	/* destination address */
} ipv6_header;

#define ip6_vfc   ip6_ctlun.ip6_un2_vfc
#define ip6_flow  ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen  ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt   ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim  ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops  ip6_ctlun.ip6_un1.ip6_un1_hlim

/* Defines for which parts have been modified */
#define IPV6_MOD_FLOW     (1)
#define IPV6_MOD_VERSION  (1<<1)
#define IPV6_MOD_PRIORITY (1<<2)
#define IPV6_MOD_PLEN     (1<<3)
#define IPV6_MOD_HLIM     (1<<4)
#define IPV6_MOD_NXT      (1<<5)
#define IPV6_MOD_SRC      (1<<6)
#define IPV6_MOD_DST      (1<<7)

/* Options */
sendip_option ipv6_opts[] = {
	{ "v", 1, "Version", NULL },
	{ "p", 1, "Priority", "0" },
	{ "t", 1, "Traffic class", "0" },
	{ "e", 1, "ECN value", "0" },
	{ "f", 1, "Flow ID", "0" },
	{ "l", 1, "Payload length", "Correct" },
	{ "n", 1, "Next header protocol", "Correct" },
	{ "h", 1, "Hop limit", "32" },
	{ "s", 1, "Source address", "::1" },
	{ "d", 1, "Destination address", "Correct" }
};

#endif  /* _SENDIP_IPV6_H */

/* vim: ts=4 sw=4 filetype=c
 */
