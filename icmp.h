/** icmp.h
 */
#ifndef _SENDIP_ICMP_H
#define _SENDIP_ICMP_H

#define ICMP6_ECHO_REQUEST 128
#define ICMP_ECHO          8

/* ICMP HEADER */
typedef struct {
	u_int8_t type;
	u_int8_t code;
	u_int16_t check;
} icmp_header;


/* Defines for which parts have been modified */
#define ICMP_MOD_TYPE  1
#define ICMP_MOD_CODE  1<<1
#define ICMP_MOD_CHECK 1<<2

/* Options */
sendip_option icmp_opts[] = {
	{ "t", 1, "Message type", "8 for IPv4, 128 for IPv6" },
	{ "d", 1, "Code", "0" },
	{ "c", 1, "Checksum", "Correct" }
};

#endif  /* _SENDIP_ICMP_H */

/* vim: ts=4 sw=4 filetype=c
 */
