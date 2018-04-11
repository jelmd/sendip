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
	{ "m", TRUE, "BGP Marker field (format is <hex byte>:<hex byte>:...)",
		"FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF" },
	{ "l", TRUE, "Packet length", "Correct" },
	{ "t", TRUE, "Message Type (1 OPEN, 2 UPDATE, 3 NOTIFICATION, 4 KEEPALIVE)",
		"4 (KEEPALIVE)" },
	{ "o", TRUE, "Open message.  Format is <version>:<AS number>:"
		"<Hold time>:<BGP Identifier>:<Options length>",
		"4:1:90:127.0.0.1:Correct  (Any parameter can be omitted to get "
		"the default)" },
	{ "oo", TRUE, "Optional OPEN parameter.  Format is <Type>:<Length>:"
	"<Value>   - value is in hex bytes separated by :s",
	"None, though length may be omitted to get correct value" },
	{ "ul", TRUE, "Withdrawn routes length", "Correct" },
	{ "uw", TRUE, "Withdrawn route.  Format is x.x.x.x/n:<bytes for prefix>",
	"Bytes field may be omitted to use the correct number" },
	{ "us", TRUE, "Attributes length", "Correct" },
	{ "ua", TRUE, "Attribute.  Format is <flags>:<type>:"
		"<length length (1 or 2):<length>:<data>",
		"The length fields may be omitted to use the correct value" },
	{ "un", TRUE, "NLRI Prefix.  Format is as for -buw", "As for -buw" },
	{ "n", TRUE, "Notification.  Format is <code>:<subcode>:<data>",
		"Data may be omitted for no data" },
};
#endif

/* vim: ts=4 sw=4 filetype=c
 */
