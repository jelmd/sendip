/** headers.h */

#ifndef _HEADERS_H
#define _HEADERS_H

#include "types.h"

/* IPV6 extension headers
 *	In the Linux source, some of these (those also in v4)
 *	are part of an enum, but let's just pull them all together.
 */
#ifndef IPPROTO_HOPOPTS
#define IPPROTO_HOPOPTS		0	/* IPv6 hop-by-hop options	*/
#endif
#ifndef IPPROTO_ROUTING
#define IPPROTO_ROUTING		43	/* IPv6 routing header		*/
#endif
#ifndef IPPROTO_FRAGMENT
#define IPPROTO_FRAGMENT	44	/* IPv6 fragmentation header	*/
#endif
#ifndef IPPROTO_GRE
#define IPPROTO_GRE			47	/* Cisco GRE tunnels		*/
#endif
#ifndef IPPROTO_ESP
#define IPPROTO_ESP			50	/* Encapsulation Security Payload */
#endif
#ifndef IPPROTO_AH
#define IPPROTO_AH			51	/* Authentication Header	*/
#endif
#ifndef IPPROTO_ICMPV6
#define IPPROTO_ICMPV6		58	/* ICMPv6			*/
#endif
#ifndef IPPROTO_NONE
#define IPPROTO_NONE		59	/* IPv6 no next header		*/
#endif
#ifndef IPPROTO_DSTOPTS
#define IPPROTO_DSTOPTS		60	/* IPv6 destination options	*/
#endif
#ifndef IPPROTO_BEETPH
#define IPPROTO_BEETPH		94	/* IP option pseudo header for BEET */
#endif
#ifndef IPPROTO_COMP
#define IPPROTO_COMP		108	/* Compression Header		*/
#endif
#ifndef IPPROTO_MH
#define IPPROTO_MH			135	/* IPv6 mobility header		*/
#endif

/* others ... */
#ifndef IPPROTO_IP
#define IPPROTO_IP			0	/* Dummy protocol for TCP	*/
#endif
#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP		1	/* Internet Control Message 	*/
#endif
#ifndef IPPROTO_IGMP
#define IPPROTO_IGMP		2	/* Internet Group Management 	*/
#endif
#ifndef IPPROTO_IPIP
#define IPPROTO_IPIP		4	/* IPIP tunnels 		*/
#endif
#ifndef IPPROTO_TCP
#define IPPROTO_TCP			6	/* Transmission Control Protocol*/
#endif
#ifndef IPPROTO_EGP
#define IPPROTO_EGP			8	/* Exterior Gateway Protocol	*/
#endif
#ifndef IPPROTO_PUP
#define IPPROTO_PUP			12	/* PUP 				*/
#endif
#ifndef IPPROTO_UDP
#define IPPROTO_UDP			17	/* User Datagram Protocol	*/
#endif
#ifndef IPPROTO_IDP
#define IPPROTO_IDP			22	/* XNS IDP protocol		*/
#endif
#ifndef IPPROTO_DCCP
#define IPPROTO_DCCP		33	/* Datagram Congestion Control 	*/
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6		41	/* IPv6-in-IPv4 tunnelling	*/
#endif
#ifndef IPPROTO_RSVP
#define IPPROTO_RSVP		46	/* RSVP 			*/
#endif
#ifndef IPPROTO_PIM
#define IPPROTO_PIM			103	/* Protocol Independent Multicast*/
#endif
#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP		132	/* Stream Control Transport 	*/
#endif
#ifndef IPPROTO_UDPLITE
#define IPPROTO_UDPLITE		136	/* UDP-Lite (RFC 3828)		*/
#endif
#ifndef IPPROTO_RAW
#define IPPROTO_RAW			255	/* Raw IP packets		*/
#endif
#ifndef IPPROTO_WESP
#define IPPROTO_WESP		141	/* Wrapped ESP */
#endif

/* Header types and associated sendip opt_chars. Things should be set
 * up so this can be determined dynamically, but for now, we'll just
 * make a fixed table.
 */
struct sendip_headers {
	const char opt_char;
	const u_int8_t ipproto;
};

extern struct sendip_headers sendip_headers[];

u_int8_t header_type(const char hdr_char);
int outer_header(const char *hdrs, int index, const char *choices);
int inner_header(const char *hdrs, int index, const char *choices);

#endif  /* _HEADERS_H */

/* vim: ts=4 sw=4 filetype=c
 */
