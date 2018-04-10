/* ripng.c - RIPng (version 1) code for sendip
 * Created by hacking rip code
 * ChangeLog since 2.2 release:
 * 15/10/2002 Read the spec
 * 24/11/2002 Made it compile on archs needing alignment
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "sendip_module.h"
#include "common.h"

#include "ripng.h"

/* Character that identifies our options */
const char opt_char = 'R';

static struct in6_addr inet6_addr(char *hostname) {
	struct hostent *host;
	static struct in6_addr ret;

	if (hostname == NULL)
		return in6addr_any;
	host = gethostbyname2(hostname, AF_INET6);
	if (host == NULL) {
		DERROR("RIPng: Couldn't get address for %s defaulting to loopback",
			hostname)
		return in6addr_loopback;
	}
	if (host->h_length != sizeof(struct in6_addr)) {
		DERROR("RIPng: IPV6 address is the wrong size: defaulting to loopback")
		return in6addr_loopback;
	}
	memcpy(&ret, host->h_addr, sizeof(ret));
	return ret;
}

sendip_data *initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	ripng_header *rip = malloc(sizeof(ripng_header));
	memset(rip, 0, sizeof(ripng_header));
	ret->alloc_len = sizeof(ripng_header);
	ret->data = (void *) rip;
	ret->modified = 0;
	return ret;
}

#define NEXT_FIELD \
	p = (c == '|') ? ++q : q; \
	while (*q != '|' && *q != '\0') { q++; } \
	len = q - p; \
	c = *q; \
	*q = '\0';

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	ripng_header *rippack = (ripng_header *) pack->data;
	ripng_entry *ripopt;
	char *p, *q, c = '\0';
	int len;

	switch(opt[1]) {
	case 'v': /* version */
		rippack->version = opt2inth(arg, 1);
		pack->modified |= RIPNG_MOD_VERSION;
		break;
	case 'c': /* command */
		rippack->command = opt2inth(arg, 1);
		pack->modified |= RIPNG_MOD_COMMAND;
		break;
	case 'r': /* reserved */
		rippack->res = opt2intn(arg, 2);
		pack->modified |= RIPNG_MOD_RESERVED;
		break;
	case 'e': /* rip entry */
		RIPNG_ADD_ENTRY(pack);
		ripopt = RIPNG_ENTRY(pack);
		q = strdup(arg == NULL ? "" : arg);
		NEXT_FIELD
		ripopt->prefix = inet6_addr(len == 0 ? NULL : p);
		NEXT_FIELD
		ripopt->tag = len == 0 ? 0 : opt2intn(p, 2);
		NEXT_FIELD
		ripopt->len = len == 0 ? 128 : opt2inth(p, 1);
		NEXT_FIELD
		ripopt->metric = len == 0 ? 16 : opt2inth(p, 1);
		break;
	case 'd': /* default request */
		if (RIPNG_NUM_ENTRIES(pack) != 0) {
			WARN("Warning: a real RIPng packet does not have any other "
				"entries in a default request.")
		}
		rippack->command = (u_int8_t) 1;
		rippack->version = (u_int8_t) 1;
		rippack->res = (u_int16_t) 0;
		pack->modified |=
			RIPNG_MOD_COMMAND | RIPNG_MOD_VERSION | RIPNG_MOD_RESERVED;
		RIPNG_ADD_ENTRY(pack);
		ripopt = RIPNG_ENTRY(pack);
		ripopt->prefix = in6addr_any;
		ripopt->tag = (u_int16_t) 0;
		ripopt->len = (u_int8_t) 0;
		ripopt->metric = htons((u_int16_t) 16);
		break;
	}
	return TRUE;

}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[],
	int index, __attribute__((unused)) sendip_data *data,
	__attribute__((unused)) sendip_data *pack)
{
	if (hdrs[index - 1] != 'u') {
		WARN("RIPng should be contained in an UDP packet");
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
