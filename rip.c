/* rip.c - RIP-1 and -2 code for sendip
 * Taken from code by Richard Polton <Richard.Polton@msdw.com>
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "sendip_module.h"
#include "common.h"

#include "rip.h"

/* Character that identifies our options */
const char opt_char = 'r';

sendip_data *initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	rip_header *rip = malloc(sizeof(rip_header));
	memset(rip, 0, sizeof(rip_header));
	ret->alloc_len = sizeof(rip_header);
	ret->data = rip;
	ret->modified = 0;
	return ret;
}

#define NEXT_FIELD \
	p = (c == '|') ? ++q : q; \
	while (*q != '|' && *q != '\0') { q++; } \
	len = q - p; \
	c = *q; \
	*q = '\0';

bool do_opt(const char *opt, const char *arg, sendip_data *pack) {
	rip_header *rippack = (rip_header *)pack->data;
	rip_options *ripopt;
	char *s, *p, *q;
	char c = '\0';
	int len = 0;

	switch(opt[1]) {
	case 'v': /* version */
		rippack->version = opt2intn(arg, 1);
		pack->modified |= RIP_MOD_VERSION;
		break;
	case 'c': /* command */
		rippack->command = opt2intn(arg, 1);
		pack->modified |= RIP_MOD_COMMAND;
		break;
	case 'a': /* authenticate */
		if (RIP_NUM_ENTRIES(pack) != 0) {
			WARN("real RIP-2 packets use authentication as the first entry")
		}
		RIP_ADD_ENTRY(pack);
		ripopt = RIP_OPTION(pack);
		memset(ripopt, 0, sizeof(rip_options));
		ripopt->addressFamily = 0xFFFF;
		s = q = strdup(arg == NULL ? "" : arg);
		NEXT_FIELD
		ripopt->routeTagOrAuthenticationType =
			opt2intn(c == '|' ? p : "2", 2);
		if (c == '|') {
			NEXT_FIELD
		}
		if (len > 16) {
			WARN("Cutting down passord to 16 characters")
			len = 16;
		}
		if (len < 1) {
			WARN("no password supplied")
		} else {
			strncpy((char *) &(ripopt->address), p, len);
		}
		free(s);
		break;
	case 'e': /* rip entry */
		if (RIP_NUM_ENTRIES(pack) == 25) {
			WARN("real RIP packets have not more than 25 entries")
		}
		RIP_ADD_ENTRY(pack);
		ripopt = RIP_OPTION(pack);
		s = q = strdup(arg == NULL ? "" : arg);
		NEXT_FIELD
		ripopt->addressFamily = opt2intn(len == 0 ? "2" : p, 2);
		NEXT_FIELD
		ripopt->routeTagOrAuthenticationType =
			opt2intn(len == 0 ? "2" : p, 2);
		NEXT_FIELD
		ripopt->address = opt2v4(len == 0 ? "0.0.0.0" : p, 4);
		NEXT_FIELD
		ripopt->subnetMask = opt2v4(len == 0 ? "255.255.255.0" : p, 4);
		NEXT_FIELD
		ripopt->nextHop = opt2v4(len == 0 ? "0.0.0.0" : p, 4);
		NEXT_FIELD
		ripopt->metric = opt2intn(len == 0 ? p : "16", 4);
		free(s);
		break;
	case 'd': /* default request */
		if (RIP_NUM_ENTRIES(pack) != 0) {
			WARN("real RIP-1/-2 packet have no entries in a default request")
		}
		RIP_ADD_ENTRY(pack);
		ripopt = RIP_OPTION(pack);
		rippack->command = (u_int8_t) 1;
		ripopt->addressFamily = (u_int16_t) 0;
		ripopt->routeTagOrAuthenticationType = (u_int16_t) 0;
		ripopt->address = inet_addr("0.0.0.0");
		ripopt->subnetMask = inet_addr("0.0.0.0");
		ripopt->nextHop = inet_addr("0.0.0.0");
		ripopt->metric = htons((u_int16_t) 16);
		break;
	case 'r': /* set reserved field */
		rippack->res = opt2intn(arg, 2);
		pack->modified |= RIP_MOD_RESERVED;
		break;
	default:
	  ERROR("Unrecognized option opt");
	  return FALSE;
	}
	return TRUE;
}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[],
	int index, __attribute__((unused)) sendip_data *data,
	__attribute__((unused)) sendip_data *pack)
{
	if (hdrs[index-1] != 'u') {
		WARN("RIP should be contained in a UDP packet");
	}
	return TRUE;
}

int
num_opts() {
	return sizeof(rip_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return rip_opts;
}

char
get_optchar() {
	return opt_char;
}
