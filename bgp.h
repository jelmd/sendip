/**---------------------------------------------------------------------------
 * bgp.h   - bgp module for SendIP
 *
 * Copyright (C) David Ball for Project Purple, August 2001
 *----------------------------------------------------------------------------
 */
#ifndef BGP_H
#define BGP_H

#include "sendip_module.h"

/* Roughly stolen from arpa/nameser.h */
#define GETSHORT(ptr) (ntohs(					\
	((u_int16_t)((u_int8_t *)(ptr))[0] << 8) |	\
	((u_int16_t)((u_int8_t *)(ptr))[1])			\
))

#define GETLONG(ptr) (ntohl(					\
	((u_int32_t)((u_int8_t *)(ptr))[0] << 24) |	\
	((u_int32_t)((u_int8_t *)(ptr))[1] << 16) |	\
	((u_int32_t)((u_int8_t *)(ptr))[2] << 8)  |	\
	((u_int32_t)((u_int8_t *)(ptr))[3])			\
))

#define PUTSHORT(ptr, s) {					\
	u_int16_t v = htons((u_int16_t)(s));	\
	*((u_int8_t *)(ptr)) = v >> 8;			\
	*(((u_int8_t *)(ptr)) + 1) = v;			\
}

#define PUTLONG(ptr, l) {					\
	u_int32_t v = htonl((u_int32_t)(l));	\
	*((u_int8_t *)(ptr)) = v >> 24;			\
	*(((u_int8_t *)(ptr)) + 1) = v >> 16;	\
	*(((u_int8_t *)(ptr)) + 2) = v >> 8;	\
	*(((u_int8_t *)(ptr)) + 3) = v;			\
}

/* Defines for which parts have been modified */
const u_int32_t BGP_MOD_LENGTH =   0x00000001;
const u_int32_t BGP_MOD_OPT_LEN =  0x00000002;
const u_int32_t BGP_MOD_WDR_LEN =  0x00000004;
const u_int32_t BGP_MOD_ATTR_LEN = 0x00000008;


/* Parts of BGP messages */
typedef enum {
	BGP_HEADER,
	BGP_OPEN,
	BGP_OPEN_OPT,
	BGP_UPDATE_WDR_LEN,
	BGP_UPDATE_WDR,
	BGP_UPDATE_ATTR_LEN,
	BGP_UPDATE_ATTR,
	BGP_UPDATE_NLRI,
	BGP_NOTFN
} bgp_msg_part;

/* Gaping buffer overrun - make sure this is long enough :) */
const u_int32_t  BGP_BUFLEN = 1400;

/* Options */
sendip_option bgp_opts[] = {
	{ "m", 1, "Marker field (16 byte)", "FF:FF:FF:FF..." },
	{ "l", 1, "Packet length", "Correct" },
	{ "t", 1, "Message type", "4 (KEEPALIVE)" },
	{ "o", 1, "Open message (vers:AS:time:ID:olen)",
				"\n4:1:90:127.0.0.1:Correct" },
	{ "oo", 1, "Optional OPEN parameter. (type:length:value)", "0::0" },
	{ "ul", 1, "Withdrawn routes length", "Correct" },
	{ "uw", 1, "Withdrawn route (x.x.x.x/n:length)", "0:0:" },
	{ "us", 1, "Attributes length", "Correct" },
	{ "ua", 1, "Attribute (flags:type:length:data)", NULL },
	{ "un", 1, "NLRI (x.x.x.x/n:length)", "0:0:" },
	{ "n", 1, "Notification  (code:subcode:data)", "0:0:" },
};
#endif

/* vim: ts=4 sw=4 filetype=c
 */
