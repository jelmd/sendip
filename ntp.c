/** ntp.c - Network Timxe Protocol module for sendip
 * Created by Mike Ricketts <mike@earth.li>
 */

#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#include "sendip_module.h"
#include "common.h"

#include "ntp.h"

/* Character that identifies our options */
const char opt_char = 'n';

#define FRIC 65536.								/* 2^16 as a double */
#define FRAC 4294967296.						/* 2^32 as a double */
#define JAN_1970	2208988800UL				/* 1970 - 1900 in seconds */

#define D2PN(x)	htonl((int) roundl((x) * FRIC))
#define D2LPN(x) htonl((unsigned long) roundl((x) * FRAC))

#ifdef __linux
#define htonll(x)	htobe64(x);
#endif

bool
make_ts(ntp_ts *dest, const char *src) {
	unsigned long val;
	char *fracpart;

	if (src == NULL)
		return FALSE;

	while (*src != '\0' && isspace(*src))
		src++;

	if ((fracpart = strchr(src, '.')) == NULL) {
		unsigned long long int lval = htonll(strtoull(src, &fracpart, 0));
		dest->intpart = (lval >> 32) & 0xFFFFFFFF;
		dest->fracpart = (lval & 0xFFFFFFFF);
		return TRUE;
	}

	val = strtoul(src, &fracpart, 0);
	if (*src == '+')
		val += JAN_1970;
	dest->intpart = htonl(val);
	dest->fracpart = D2LPN(strtold(fracpart, NULL));
	return TRUE;
}

sendip_data *initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	ntp_header *ntp = malloc(sizeof(ntp_header));
	memset(ntp, 0, sizeof(ntp_header));
	ntp->version = 4;
	ntp->poll = 6;
	ret->alloc_len = sizeof(ntp_header);
	ret->data = ntp;
	ret->modified = 0;
	return ret;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	ntp_header *ntp = (ntp_header *) pack->data;

	switch (opt[1]) {
	case 'l':  /* Leap Indicator (2 bits) */
		ntp->leap = opt2inth(arg, NULL, 1) & 3;
		pack->modified |= NTP_MOD_LEAP;
		break;
	case 'v':  /* Version (3 bits) */
		ntp->version = opt2inth(arg, NULL, 1) & 7;
		pack->modified |= NTP_MOD_STATUS;
		break;
	case 'm':  /* Mode (3 bits)  */
		ntp->mode = opt2inth(arg, NULL, 1) & 7;
		pack->modified |= NTP_MOD_TYPE;
		break;
	case 's':  /* Stratum */
		ntp->stratum = opt2inth(arg, NULL, 1);
		pack->modified |= NTP_MOD_STATUS;
		break;
	case 'P':  /* Poll */
		ntp->poll = opt2inth(arg, NULL, 1);
		pack->modified |= NTP_MOD_PRECISION;
		break;
	case 'p':  /* precision (8 bits, range +32 to -32) */
		ntp->precision = opt2inth(arg, NULL, 1);
		pack->modified |= NTP_MOD_PRECISION;
		break;
	case 'e':  /* esitmated error (32 bits, fixed point between bits 15/16) */
		ntp->error = strchr(arg, '.') != NULL
				? D2PN(strtold(arg, NULL))
				: opt2intn(arg, NULL, 4);
		pack->modified |= NTP_MOD_ERROR;
		break;
	case 'd':  /* estimated drift rate (32 bits, signed fixed point left of
				  high order bit) */
		ntp->drift = D2PN(strtold(arg, NULL));
		pack->modified |= NTP_MOD_DRIFT;
		break;
	case 'r':  /* reference clock id (32 bits or a 4 byte string).  Can be:
				  If type == 1: "WWVB", "GOES", "WWV\0"
				  If type == 2: IP address
				  Else: must be zero
				*/
		if ('0' <= *arg && *arg <= '9') {
			/* either a number or an IP */
			if ((ntp->reference.ipaddr = inet_addr(arg)) == (in_addr_t) -1) {
				/* Not a valid IP, or really want 255.255.255.255 */
				if (strcmp(arg, "255.255.255.255") != 0) {
					ntp->reference.ipaddr = htonl(strtol(arg, NULL, 0));
				}
			}
		} else {
			/* Hopefully a 4 byte or less string */
			ntp->reference.ipaddr = 0;
			if (strlen(arg) <= 4) {
				memcpy(ntp->reference.id, arg, strlen(arg));
			} else {
				ERROR("ntp - reference clock ID must be IP addr, "
					"32 bit integer, or 4 byte string")
				return FALSE;
			}
		}
		pack->modified |= NTP_MOD_REF;
		break;
	case 'f':  /* reference timestamp (64 bits) */
		if (make_ts(&(ntp->reference_ts), arg)) {
			pack->modified |= NTP_MOD_REFERENCE;
		} else {
			ERROR("ntp - couldn't parse reference timestamp")
			return FALSE;
		}
		break;
	case 'o':  /* originate timestamp (64 bits) */
		if (make_ts(&(ntp->originate_ts), arg)) {
			pack->modified |= NTP_MOD_ORIGINATE;
		} else {
			ERROR("ntp - couldn't parse originate timestamp")
			return FALSE;
		}
		break;
	case 'a':  /* receive timestamp (64 bits) */
		if (make_ts(&(ntp->receive_ts), arg)) {
			pack->modified |= NTP_MOD_RECEIVE;
		} else {
			ERROR("ntp - couldn't parse receive timestamp")
			return FALSE;
		}
		break;
	case 'x':  /* transmit timestamp (64 bits) */
		if (make_ts(&(ntp->transmit_ts), arg)) {
			pack->modified |= NTP_MOD_TRANSMIT;
		} else {
			ERROR("ntp - couldn't parse transmit timestamp")
			return FALSE;
		}
		break;
	}
	return TRUE;
}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[], int index,
	__attribute__((unused)) sendip_data *data,
	__attribute__((unused)) sendip_data *pack)
{
	if (hdrs[index - 1] != 'u') {
		WARN("NTP should be contained in a UDP packet")
	}
	return TRUE;
}

int
num_opts() {
	return sizeof(ntp_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return ntp_opts;
}

char
get_optchar() {
	return opt_char;
}

/*
   ./sendip -D h -p ipv4 -p udp -us 123 -ud 123 -p ntp -nm 5 -ns 3 \
   -nP 6 -np -22 -ne 0.0004425048828125 -nd 0.0650634765625 -nr 127.0.0.1 \
   -nf 3732887762.94529517274349927902 -nx 3732889545.94502960937097668647 \
   localhost

   Final packet data:
	    00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F

	 0: 45 00 00 4c 6a 56 00 00  ff 11 2d 0f 7f 00 00 01  E..LjV....-.....
	10: 8d 2c 18 0e 00 7b 00 7b  00 38 71 e1 25 03 06 ea  .,...{.{.8b.%...
	20: 00 00 00 1d 00 00 10 a8  7f 00 00 01 de 7f 58 d2  .........,....X.
	30: f1 fe dd 4c 00 00 00 00  00 00 00 00 00 00 00 00  ...L............
	40: 00 00 00 00 de 7f 5f c9  f1 ed 75 e2              ......_...u.

	ts alternative:
	-nf +1523898962.94529517274349927902 -nx +1523900745.94502960937097668647 \
 */

/* vim: ts=4 sw=4 filetype=c
 */
