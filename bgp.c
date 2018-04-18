/*----------------------------------------------------------------------------
 * bgp.c   - bgp module for SendIP
 * Copyright (C) David Ball for Project Purple, August 2001
 *----------------------------------------------------------------------------
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "sendip_module.h"
#include "common.h"

#include "bgp.h"

/* Character that identifies our options
 */
const char bgp_opt_char = 'b';

bgp_msg_part bgp_prev_part;
u_int8_t *bgp_len_ptr = NULL;
u_int8_t *bgp_opt_len_ptr = NULL;
u_int8_t *bgp_wdr_len_ptr = NULL;
u_int8_t *bgp_attr_len_ptr = NULL;


sendip_data *
initialize (void)
{
	sendip_data *data = NULL;
	u_int8_t *dp;
	
	data = malloc(sizeof(sendip_data));
	if (data == NULL)
		return NULL;

	memset(data, 0, sizeof(sendip_data));
	data->data = malloc(BGP_BUFLEN);
	if (data->data == NULL) {
		free(data);
		return NULL;
	}
	memset(data->data, 0, BGP_BUFLEN);
	/* Initialize the start of the 1st message with the 16 x 0xff marker */
	dp = data->data;
	memset(data->data, 0xFF, 16);
	dp += 16;

	/* set message length: marker(16) + length(2) + type(1) */
	bgp_len_ptr = dp;
	PUTSHORT(dp, 19)

	/* set the message type to 4 (KEEPALIVE) */
	dp += 2;
	*dp++ = 4;
		
	/* update meta data */
	data->alloc_len = dp - (u_int8_t *) data->data;
	bgp_prev_part = BGP_HEADER;

	return (data);
}

static int
bgp_parse_bytes (u_int8_t *current_pe, const char *arg, char **endptr,
	u_int32_t limit, int base)
{
	u_int8_t val, *pe = current_pe;
	char *p, *q;

	errno = 0;
	/* get the first number and non-const ptr we can work with */
	val = strtoul(arg, &q, base);
	if (errno) {
		if (endptr != NULL)
			*endptr = q;
		return 0;
	}
	*pe++ = val;
	p = (*q == '\0') ? q : q + 1;
	/* now parse remaining numbers */
	while (--limit > 0 && *p != '\0') {
		val = strtoul(p, &q, base);
		if (errno) {
			if (endptr != NULL)
				*endptr = p;
			return arg - p;
		}
		*pe++ = val;
		p = (*q == '\0') ? q : q + 1;
	}
	if (endptr != NULL)
		*endptr = p;
	return (pe - current_pe);
}


static int
bgp_parse_nlri (u_int8_t *current_pe, const char *arg)
{
	u_int8_t *pe = current_pe;
	char *p, *q;
	u_int8_t bytes;
	int n;

	pe++;
	n = bgp_parse_bytes(pe, arg, &q, 4, 10);
	if (n < 1)
		return n;
	p = q;
	*current_pe = (u_int8_t) strtoul(p, &q, 10);
	p = (*q == '\0') ? q : q + 1;
	bytes = (u_int8_t) strtoul(p, &q, 10);
	if (p != q) {
		pe += bytes;
	} else if (*current_pe > 0) {
		pe += ((*current_pe - 1) >> 3) + 1;
	}

	return (pe - current_pe);
}


bool
do_opt (const char *optstring, const char *optarg, sendip_data *pack)
{
	/* start of the bgp data */
	u_int8_t *pd = NULL;
	/* end of the bgp data, i.e. start of the remaining free block */
	u_int8_t *pe = (u_int8_t *) pack->data + pack->alloc_len;

	char *q, *p = NULL;
	bool rc = TRUE;
	bool len_mod = FALSE;
	u_int8_t bytes;
	int n;

	switch (optstring[1]) {
	case 'm':
		pd = (u_int8_t *) pack->data;
		(void) bgp_parse_bytes(pd, optarg, NULL, 16, 16);
		break;
	case 'l':
		pd = (u_int8_t *) pack->data + 16;
		PUTSHORT(pd, opt2inth(optarg, NULL, 2))
		pack->modified |= BGP_MOD_LENGTH;
		break;
	case 't':
		pd = (u_int8_t *) pack->data + 18;
		*pd = (u_int8_t) opt2inth(optarg, NULL, 1);
		break;
	case 'o':
		switch (optstring[2]) {
		case '\0':
			if (bgp_prev_part != BGP_HEADER) {
				ERROR("bgp OPEN message (-o) must come right after the header")
				rc = FALSE;
			} else {
				/* version */
				*pe = (u_int8_t) strtoul(optarg, &q, 0);
				if (optarg == q) {
					*pe = 4;
				}
				pe++;
				/* AS number */
				p = (*q == '\0' ) ? q : q + 1;
				PUTSHORT(pe, (u_int16_t) strtoul(p, &q, 0))
				if (p == q) {
					PUTSHORT(pe, 1)
				}
				pe += 2;
				/* hold time */
				p = (*q == '\0' ) ? q : q + 1;
				PUTSHORT(pe, (u_int16_t) strtoul(p, &q, 0))
				if (p == q) {
					PUTSHORT(pe, 90)
				}
				pe += 2;
				/* ID */
				p = (*q == '\0' ) ? q : q + 1;
				n = bgp_parse_bytes(pe, p, &q, 4, 10);
				if (n < 1) {
					DERROR("Invalid number in '%s' at position %d", optarg, n)
					rc = FALSE;
					break;
				}
				if (p == q) {
					PUTLONG(pe, 0x7F000001)
				}
				pe += 4;
				/* olength */
				p = (*q == '\0' ) ? q : q + 1;
				*pe = (u_int8_t) strtoul(p, &q, 0);
				if (p == q) {
					*pe = 0;
				} else {
					pack->modified |= BGP_MOD_OPT_LEN;
				}
				bgp_opt_len_ptr = pe;
				pe++;
				
				bgp_prev_part = BGP_OPEN;
			}
			break;
		case 'o':
			if (bgp_prev_part != BGP_OPEN && bgp_prev_part != BGP_OPEN_OPT) {
				DERROR("bgp OPTIONS message (-oo) must come after open message")
				rc = FALSE;
			} else {
				pd = pe;
				*pe++ = (u_int8_t) strtoul(optarg, &q, 0);
				p = (*q == '\0') ? q : q + 1;

				*pe = (u_int8_t) strtoul(p, &q, 0);
				if (p == q) {
					*pe = 0;
				} else {
					len_mod = TRUE;
				}
				pe++;
				p = (*q == '\0') ? q : q + 1;
				pe += bgp_parse_bytes(pe, p, NULL, 0xFF, 16);
				
				if (!len_mod) {
					*(pd + 1) = pe - pd;
				}
				if (!(pack->modified & BGP_MOD_OPT_LEN)) {
					*bgp_opt_len_ptr += pe - pd;
				}
				bgp_prev_part = BGP_OPEN_OPT;
			}
			break;
		default:
			DERROR("Unrecognised bgp OPEN option '%s'", optstring)
			rc = FALSE;
		}
		break;
	case 'u':
		switch (optstring[2]) {
		case 'l':
			if (bgp_prev_part != BGP_HEADER) {
				DERROR("bgp UPDATE message (-ul) must come right after header")
				rc = FALSE;
			} else {
				bgp_wdr_len_ptr = pe;
				PUTSHORT(pe, (u_int16_t) strtoul(optarg, NULL, 0))
				pe += 2;
				pack->modified |= BGP_MOD_WDR_LEN;
				bgp_prev_part = BGP_UPDATE_WDR_LEN;
			}
			break;
		case 'w':
			if (bgp_prev_part == BGP_HEADER) {
				bgp_wdr_len_ptr = pe;
				PUTSHORT(pe, 0)
				pe += 2;
				bgp_prev_part = BGP_UPDATE_WDR_LEN;
			}
			if (bgp_prev_part != BGP_UPDATE_WDR
				&& bgp_prev_part != BGP_UPDATE_WDR_LEN)
			{
				ERROR("bgp unfeasible routes (-uw) must come right after "
					"header or -ul")
				rc = FALSE;
			} else {
				pd = pe;
				pe += bgp_parse_nlri(pe, optarg);
				
				if (!(pack->modified & BGP_MOD_WDR_LEN)) {
					PUTSHORT(bgp_wdr_len_ptr,
						GETSHORT(bgp_wdr_len_ptr) + pe - pd)
				}
				bgp_prev_part = BGP_UPDATE_WDR;
			}
			break;
		case 's':
			if (bgp_prev_part == BGP_HEADER) {
				bgp_wdr_len_ptr = pe;
				PUTSHORT(pe, 0)
				pe += 2;
				bgp_prev_part = BGP_UPDATE_WDR_LEN;
			}
			if (bgp_prev_part != BGP_UPDATE_WDR_LEN
					&& bgp_prev_part != BGP_UPDATE_WDR)
			{
				ERROR("bgp Total Path Attribute Length (-us) must come after "
					"unfeasible routes (if any) (-uw), or after header")
				rc = FALSE;
			} else {
				bgp_attr_len_ptr = pe;
				PUTSHORT(pe, opt2inth(optarg, NULL, 2))
				pe += 2;
				pack->modified |= BGP_MOD_ATTR_LEN;
				bgp_prev_part = BGP_UPDATE_ATTR_LEN;
			}
			break;
		case 'a':
			if (bgp_prev_part == BGP_HEADER) {
				bgp_wdr_len_ptr = pe;
				PUTSHORT(pe, 0)
				pe += 2;
				bgp_prev_part = BGP_UPDATE_WDR_LEN;
			}
			if (bgp_prev_part == BGP_UPDATE_WDR_LEN
				|| bgp_prev_part == BGP_UPDATE_WDR)
			{
				bgp_attr_len_ptr = pe;
				PUTSHORT(pe, 0)
				pe += 2;
				bgp_prev_part = BGP_UPDATE_ATTR_LEN;
			}
			if (bgp_prev_part != BGP_UPDATE_ATTR_LEN
				&& bgp_prev_part != BGP_UPDATE_ATTR)
			{
				DERROR("bgp Path Attributes (-ua) must come after Total Path "
					"Attribute Length (-us), or after header")
				rc = FALSE;
			} else {
				*pe++ = (u_int8_t) strtoul(optarg, &q, 0);
				p = (*q == '\0') ? q : q + 1;

				*pe++ = (u_int8_t) strtoul(p, &q, 0);
				p = (*q == '\0') ? q : q + 1;

				bytes = (u_int8_t) strtoul(p, &q, 0);
				if (p != q) {
					bytes = (bytes <= 1) ? 1 : 2;
				} else {
					bytes = (*pd & 0x10) ? 2 : 1;
				}
				p = (*q == '\0') ? q : q + 1;

				if (bytes == 1) {
					*pe++ = (u_int8_t) strtoul(p, &q, 0);
				} else {
					PUTSHORT(pe, (u_int16_t) strtoul(p, &q, 0))
					pe += 2;
				}
				if (p != q) {
					len_mod = TRUE;
				}
				p = (*q == '\0') ? q : q + 1;

				if (bytes == 1) {
					pe += bgp_parse_bytes(pe, p, NULL, 0xFF, 16);
				} else {
					pe += bgp_parse_bytes(pe, p, NULL, 0xFFFF, 16);
				}
				if (!len_mod) {
					if (bytes == 1) {
						*(pd + 2) = pe - pd - 3;
					} else {
						PUTSHORT(pd + 2, pe - pd - 4)
					}
				}
				if (!(pack->modified & BGP_MOD_ATTR_LEN)) {
					PUTSHORT(bgp_attr_len_ptr, 
						GETSHORT(bgp_attr_len_ptr) + pe - pd)
				}
				bgp_prev_part = BGP_UPDATE_ATTR;
			}
			break;
		case 'n':
			if (bgp_prev_part == BGP_HEADER) {
				bgp_wdr_len_ptr = pe;
				PUTSHORT(pe, 0)
				pe += 2;
				bgp_prev_part = BGP_UPDATE_WDR_LEN;
			}
			
			if (bgp_prev_part == BGP_UPDATE_WDR_LEN
				|| bgp_prev_part == BGP_UPDATE_WDR)
			{
				bgp_attr_len_ptr = pe;
				PUTSHORT(pe, 0)
				pe += 2;
				bgp_prev_part = BGP_UPDATE_ATTR_LEN;
			}

			if (bgp_prev_part != BGP_UPDATE_ATTR_LEN
				&& bgp_prev_part != BGP_UPDATE_ATTR
				&& bgp_prev_part != BGP_UPDATE_NLRI)
			{
				ERROR("bgp NLRI (-un) must come after unfeasible routes (-uw) "
					"and attributes (-ua) (if any), or after header")
				rc = FALSE;
			} else {
				pd = pe;
				pe += bgp_parse_nlri(pe, optarg);
				
				bgp_prev_part = BGP_UPDATE_NLRI;
			}
			break;
		default:
			DERROR("Unrecognised bgp UPDATE option '%s'", optstring)
			rc = FALSE;
		}
		break;
	case 'n':
		if (bgp_prev_part != BGP_HEADER) {
			ERROR("bgp Notification (-n) must come immediately after header")
			rc = FALSE;
		} else {
			*pe++ = (u_int8_t) strtoul(optarg, &q, 0);
			p = (*q == '\0') ? q : q + 1;

			*pe++ = (u_int8_t) strtoul(p, &q, 0);
			p = (*q == '\0') ? q : q + 1;

			pe += bgp_parse_bytes(pe, p, NULL, 0xFFFF, 16);

			bgp_prev_part = BGP_NOTFN;
		}
		break;
	default:
		DERROR("Unrecognised bgp option '%s'", optstring)
		rc = FALSE;
	}
	
	if (rc) {
		pack->alloc_len = pe - (u_int8_t *) pack->data;
		if (!(pack->modified & BGP_MOD_LENGTH)) {
			PUTSHORT(bgp_len_ptr, pack->alloc_len)
		}
	}
	
	return (rc);
}


bool
finalize (char *hdrs, __attribute__((unused)) sendip_data *headers[],
	int index, __attribute__((unused)) sendip_data *data,
	__attribute__((unused)) sendip_data *pack)
{
	if (hdrs[index - 1] != 't') {
		WARN("BGP should be carried over TCP")
	}
	return TRUE;
}

int
num_opts (void) {
	return (sizeof(bgp_opts) / sizeof(sendip_option));
}

sendip_option *
get_opts (void) {
	return (bgp_opts);
}

char
get_optchar (void) {
	return (bgp_opt_char);
}

/* vim: ts=4 sw=4 filetype=c
 */
