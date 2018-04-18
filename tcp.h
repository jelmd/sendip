/** tcp.h */

#ifndef _SENDIP_TCP_H
#define _SENDIP_TCP_H

#ifndef NULL
#define NULL (0L)
#endif

/* TCP HEADER
 * Ripped straight out of glibc-2.2.2, modified for RFC22481
 * Reproduced here to prevent nasty #defines on non-linux boxes
 */
typedef struct {
	u_int16_t source;
	u_int16_t dest;
	u_int32_t seq;
	u_int32_t ack_seq;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int16_t res:4;
	u_int16_t off:4;
	u_int16_t fin:1;
	u_int16_t syn:1;
	u_int16_t rst:1;
	u_int16_t psh:1;
	u_int16_t ack:1;
	u_int16_t urg:1;
	u_int16_t cwr:1;
	u_int16_t ecn:1;
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int16_t off:4;
	u_int16_t res:4;
	u_int16_t ecn:1;
	u_int16_t cwr:1;
	u_int16_t urg:1;
	u_int16_t ack:1;
	u_int16_t psh:1;
	u_int16_t rst:1;
	u_int16_t syn:1;
	u_int16_t fin:1;
#else
#  error "Adjust your <bits/endian.h> defines"
#endif
	u_int16_t window;
	u_int16_t check;
	u_int16_t urg_ptr;

} tcp_header;

/* Defines for which parts have been modified */
#define TCP_MOD_SOURCE 1
#define TCP_MOD_DEST   1<<1
#define TCP_MOD_SEQ    1<<2
#define TCP_MOD_ACKSEQ 1<<3
#define TCP_MOD_RES    1<<4
#define TCP_MOD_OFF    1<<5
#define TCP_MOD_FIN    1<<6
#define TCP_MOD_SYN    1<<7
#define TCP_MOD_RST    1<<8
#define TCP_MOD_PSH    1<<9
#define TCP_MOD_ACK    1<<10
#define TCP_MOD_URG    1<<11
#define TCP_MOD_ECN    1<<12
#define TCP_MOD_CWR    1<<13
#define TCP_MOD_WINDOW 1<<14
#define TCP_MOD_CHECK  1<<15
#define TCP_MOD_URGPTR 1<<16

/* Options */
sendip_option tcp_opts[] = {
	{ "s", 1, "Source port", "0" },
	{ "d", 1, "Destination port", "0" },
	{ "n", 1, "Sequence number", "Random" },
	{ "a", 1, "Next expected Ack number", "0" },
	{ "t", 1, "Data offset", "Correct" },
	{ "r", 1, "Reserved field", "0" },
	{ "fe", 1, "ECN flag", "0" },
	{ "fc", 1, "CWR flag", "0" },
	{ "fu", 1, "URG flag", "0" },
	{ "fa", 1, "ACK flag", "0" },
	{ "fp", 1, "PSH flag", "0" },
	{ "fr", 1, "RST flag", "0" },
	{ "fs", 1, "SYN flag", "1" },
	{ "ff", 1, "FIN flag", "0" },
	{ "w", 1, "Window size", "65535" },
	{ "c", 1, "Checksum", "Correct" },
	{ "u", 1, "Urgent pointer", "0" },
	{ "onum", 1, "Add options (hex string)", NULL },
	{ "oeol", 0, "Add end of option list", NULL },
	{ "onop", 0, "Add no op option", NULL },
	{ "omss", 1, "Add maximum segment size option", NULL },
	{ "owscale", 1, "Add window scale option", NULL },
	{ "osackok", 0, "Add allow selective ack option", NULL },
	{ "osack", 1, "Add selective ack option (left:right,...)", NULL },
	{ "ots", 1,"Add timestamp option (tsval:tsecr)", NULL }
};

#endif  /* _SENDIP_TCP_H */

/* vim: ts=4 sw=4 filetype=c
 */
