/** ipv6.c - sendip IPv6 code
 * Taken from code by Antti Tuominen <ajtuomin@tml.hut.fi>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "sendip_module.h"
#include "ipv6.h"

/* Character that identifies our options */
const char opt_char = '6';

sendip_data *
initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	ipv6_header *ipv6 = malloc(sizeof(ipv6_header));
	memset(ipv6, 0, sizeof(ipv6_header));
	ret->alloc_len = sizeof(ipv6_header);
	ret->data = ipv6;
	ret->modified = 0;
	return ret;
}

bool
set_addr(char *hostname, sendip_data *pack) {
	ipv6_header *ipv6 = (ipv6_header *) pack->data;
	struct hostent *host = gethostbyname2(hostname, AF_INET6);
	if (!(pack->modified & IPV6_MOD_SRC)) {
		ipv6->ip6_src = in6addr_loopback;
	}
	if (!(pack->modified & IPV6_MOD_DST)) {
		if (host == NULL)
			return FALSE;

		if (host->h_length != sizeof(ipv6->ip6_dst)) {
			ERROR("IPV6 destination address is the wrong size!!!")
			return FALSE;
		}
		memcpy(&(ipv6->ip6_dst), host->h_addr, host->h_length);
	}
	return TRUE;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	ipv6_header *hdr = (ipv6_header *) pack->data;
	struct in6_addr addr;

	switch (opt[1]) {
	case 'f':
		/* TODO : This looks byte-order dependent */
		hdr->ip6_flow |= opt2intn(arg, 4) & 0xFFF00000;
		pack->modified |= IPV6_MOD_FLOW;
		break;
	case 't':
		/* TODO : This looks byte-order dependent */
		hdr->ip6_flow |= htonl((opt2inth(arg, 4) << 20) & 0x0F000000);
		pack->modified |= IPV6_MOD_FLOW;
		break;
	case 'v':
		hdr->ip6_vfc &= 0x0F;
		hdr->ip6_vfc |= (u_int8_t) ((opt2intn(arg, 1) & 0x0F) << 4);
		pack->modified |= IPV6_MOD_VERSION;
		break;
	case 'p':
		hdr->ip6_vfc &= 0xF0;
		hdr->ip6_vfc |= (u_int8_t) (opt2intn(arg, 1) & 0x0F);
		pack->modified |= IPV6_MOD_PRIORITY;
		break;
	case 'l':
		hdr->ip6_plen = opt2intn(arg, 2);
		pack->modified |= IPV6_MOD_PLEN;
		break;
	case 'h':
		hdr->ip6_hlim = opt2intn(arg, 1);
		pack->modified |= IPV6_MOD_HLIM;
		break;
	case 'n':
		/* allow use of protocol names */
		hdr->ip6_nxt = name_to_proto(arg);
		pack->modified |= IPV6_MOD_NXT;
		break;
	case 's':
		/* TODO: flexible address specification */
		if (inet_pton(AF_INET6, arg, &addr)) {
			memcpy(&hdr->ip6_src, &addr, sizeof(struct in6_addr));
		}
		pack->modified |= IPV6_MOD_SRC;
		break;
	case 'd':
		/* TODO: flexible address specification */
		if (inet_pton(AF_INET6, arg, &addr)) {
			memcpy(&hdr->ip6_dst, &addr, sizeof(struct in6_addr));
		}
		pack->modified |= IPV6_MOD_DST;
		break;
	}
	return TRUE;
}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[], int index,
	sendip_data *data, sendip_data *pack)
{
	ipv6_header *ipv6 = (ipv6_header *) pack->data;

	if (!(pack->modified & IPV6_MOD_VERSION)) {
		ipv6->ip6_vfc &= 0x0F;
		ipv6->ip6_vfc |= (6 << 4);
	}
	if (!(pack->modified & IPV6_MOD_PLEN)) {
		ipv6->ip6_plen = htons(data->alloc_len);
	}
	if (!(pack->modified & IPV6_MOD_NXT)) {
		/* the actual type of following header */
		ipv6->ip6_nxt = header_type(hdrs[index + 1]);
	}
	if (!(pack->modified & IPV6_MOD_HLIM)) {
		ipv6->ip6_hlim = 32;
	}

	return TRUE;
}

int
num_opts() {
	return sizeof(ipv6_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return ipv6_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
