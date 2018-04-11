/** ah.h */
#ifndef _SENDIP_AH_H
#define _SENDIP_AH_H

/* AH header is defined in ipv6ext.h */
typedef struct ip_auth_hdr ah_header;

/* Defines for which parts have been modified */
#define AH_MOD_SPI		(1)
#define AH_MOD_SEQUENCE	(1<<1)
#define AH_MOD_AUTHDATA	(1<<2)
#define AH_MOD_NEXTHDR	(1<<3)
#define AH_MOD_KEY		(1<<4)

/* We include provisions for a stored key for future use, when we allow
 * "real" AH implementations.
 */
typedef struct ip_ah_private {	/* keep track of things privately */
	u_int32_t type;		/* type = IPPROTO_AH */
	u_int32_t keylen;	/* length of "key" (not transmitted data) */
	u_int32_t key[];	/* key itself */
} ah_private;
/* The key is passed on to the authentication module, if any. */


#ifdef _AH_MAIN
/* Options */
sendip_option ah_opts[] = {
	{ "s", 1, "AH Security Parameters Index", "1" },
	{ "q", 1, "AH Sequence Number", "1" },
	{ "d", 1, "AH Authentication Data "
		"- variable length , can be either a user-provided string (in hex,"
		" octal, decimal, or raw), rN for N random or zN for N zero bytes.",
		"0" },
	{ "n", 1,"AH Next Header","Correct"},
	{ "k", 1, "AH Key (string, rN for N random bytes, or zN for N zero bytes) "
		"Not transmitted in the packet, but passed to the authentication "
		"module, if any.",
		"none" },
	{ "m", 1, "AH Authentication Module", "none" }
};
#endif

#endif  /* _SENDIP_AH_H */

/* vim: ts=4 sw=4 filetype=c
 */
