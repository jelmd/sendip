/** ntp.h
 */
#ifndef _SENDIP_NTP_H
#define _SENDIP_NTP_H

typedef struct {
	u_int32_t intpart;
	u_int32_t fracpart;
} ntp_ts;

/* NTP HEADER */
typedef struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t mode:3;
	u_int8_t version:3;
	u_int8_t leap:2;     
#elif __BYTE_ORDER == __BIG_ENDIAN
	u_int8_t leap:2;
	u_int8_t version:3;
	u_int8_t mode:3;
#else
#  error "Adjust your <bits/endian.h> defines"
#endif
	u_int8_t stratum;
	u_int8_t poll;
	u_int8_t precision;
	u_int32_t error;
	u_int32_t drift;
	union {
		u_int32_t ipaddr;
		char id[4];
	} reference;
	ntp_ts reference_ts;
	ntp_ts originate_ts;
	ntp_ts receive_ts;
	ntp_ts transmit_ts;
} ntp_header;

/* Defines for which parts have been modified */
#define NTP_MOD_LEAP      (1)
#define NTP_MOD_STATUS    (1<<1)
#define NTP_MOD_TYPE      (1<<2)
#define NTP_MOD_PRECISION (1<<3)
#define NTP_MOD_ERROR     (1<<4)
#define NTP_MOD_DRIFT     (1<<5)
#define NTP_MOD_REF       (1<<6)
#define NTP_MOD_REFERENCE (1<<7)
#define NTP_MOD_ORIGINATE (1<<8)
#define NTP_MOD_RECEIVE   (1<<9)
#define NTP_MOD_TRANSMIT  (1<<10)

/* Options */
sendip_option ntp_opts[] = {
	{ "l", 1, "Leap Indicator", "0" },
	{ "v", 1, "Version", "4" },
	{ "m", 1, "Mode", "0" },
	{ "s", 1, "Stratum", "0" },
	{ "P", 1, "Poll intervall", "6" },
	{ "p", 1, "Precision of system clock", "0" },
	{ "e", 1, "Root delay", "0.0" },
	{ "d", 1, "Estimated drift rate", "0.0" },
	{ "r", 1, "Reference clock ID", "0" },
	{ "f", 1, "Reference timestamp", "0.0" },
	{ "o", 1, "Originate timestamp", "0.0" },
	{ "a", 1, "Receive timestamp", "0.0" },
	{ "x", 1, "Transmit timestamp", "0.0" }
};

#endif  /* _SENDIP_NTP_H */

/* vim: ts=4 sw=4 filetype=c
 */
