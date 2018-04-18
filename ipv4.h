/** ip.h */
#ifndef _SENDIP_IP_H
#define _SENDIP_IP_H

#include "types.h"
#include "sendip_module.h"

/* IP HEADER */
typedef struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int header_len:4;
	unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;
	unsigned int header_len:4;
#else
#  error "Please fix <bits/endian.h>"
#endif
	u_int8_t tos;
	u_int16_t tot_len;
	u_int16_t id;
	/* In FreeBSD, for historical reasons, fragment offsets (and also tot_len)
	 * are specified in *host* byte order at user level; the kernel then does
	 * byte swapping before sending. So for FreeBSD only, we use the same
	 * bitfield ordering and IP_SET_FRAGOFF macro for both little and big
	 * endian.  */
#if defined(__FreeBSD) || (__BYTE_ORDER == __BIG_ENDIAN)
	u_int16_t res:1;
	u_int16_t df:1;
	u_int16_t mf:1;
	u_int16_t frag_off:13;
#	define IP_SET_FRAGOFF(ip, v) (ip)->frag_off = (v);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	u_int16_t frag_off1:5;
	u_int16_t mf:1;
	u_int16_t df:1;
	u_int16_t res:1;
	u_int16_t frag_off2:8;
#	define IP_SET_FRAGOFF(ip, v) \
		(ip)->frag_off1 = ((v) >> 8) & 0x1F; (ip)->frag_off2 = (v) & 0xFF;
#else
#  error "Please fix <bits/endian.h>"
#endif
	u_int8_t ttl;
	u_int8_t protocol;
	u_int16_t check;
	u_int32_t saddr;
	u_int32_t daddr;
} ip_header;

/* Defines for which parts have been modified */
#define IP_MOD_HEADERLEN  (1)
#define IP_MOD_VERSION    (1<<1)
#define IP_MOD_TOS        (1<<2)
#define IP_MOD_TOTLEN     (1<<3)
#define IP_MOD_ID         (1<<4)
#define IP_MOD_RES        (1<<5)
#define IP_MOD_DF         (1<<6)
#define IP_MOD_MF         (1<<7)
#define IP_MOD_FRAGOFF    (1<<8)
#define IP_MOD_TTL        (1<<9)
#define IP_MOD_PROTOCOL   (1<<10)
#define IP_MOD_CHECK      (1<<11)
#define IP_MOD_SADDR      (1<<12)
#define IP_MOD_DADDR      (1<<13)

/* Options */
sendip_option ip_opts[] = {
	{ "s", 1, "Source address", "127.0.0.1" },
	{ "d", 1, "Destination address", "Correct" },
	{ "h", 1, "Header length", "Correct" },
	{ "v", 1, "Version", "4" },
	{ "y", 1, "Type of service", "0" },
	{ "l", 1, "Total packet length", "Correct" },
	{ "i", 1, "Packet ID", "random" },
	{ "fd", 1, "Don't fragment flag", "0" },
	{ "fm", 1, "More fragments flag", "0" },
	{ "fr", 1, "Reserved flag", "0" },
	{ "f", 1, "Fragment offset", "0" },
	{ "t", 1, "Time to live", "255" },
	{ "p", 1, "Protcol", "Correct" },
	{ "c", 1, "Checksum", "Correct" },

	{ "onum", 1, "Add option value", NULL },
	{ "oeol", 0, "Add end of option list", NULL },
	{ "onop", 0, "Add no-op option", NULL },
	{ "orr", 1, "Add record route option (hex:v4addr...)", NULL },
	{ "ossr", 1, "Add strict source route option (hex:v4addr...)", NULL },
	{ "olsr", 1, "Add loose source route option (hex:v4addr...)", NULL },
	{ "ots", 1, "Add timestamp option (hex:oflow:flag:[ip:]ts...", NULL },
	{ "osid", 1, "Add stream identifier option", NULL },
};

#endif  /* _SENDIP_IP_H */

/* vim: ts=4 sw=4 filetype=c
 */
