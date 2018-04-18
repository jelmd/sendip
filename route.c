/** route.c - (IPv6) routing extension header
 *
 * TBD - create a version that works for IPv4 as well.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "sendip_module.h"
#include "common.h"

#include "ipv6ext.h"
#include "route.h"
#include "parseargs.h"

/* Character that identifies our options */
const char opt_char = 'o';	/* s'o'urce routing - r and R already used */

sendip_data *
initialize(void)
{
	sendip_data *ret = malloc(sizeof(sendip_data));
	/* Note the Linux generic routing header structure doesn't
	 * include the 4-byte reserved field. To get that, let's
	 * just use the type 0 routing header as the allocation unit.
	 */
	route_header *route = malloc(sizeof(struct rt0_hdr));

	memset(route,0,sizeof(struct rt0_hdr));
	ret->alloc_len = sizeof(struct rt0_hdr);
	ret->data = route;
	ret->modified = 0;
	return ret;
}

bool
readaddrs(const char *arg, sendip_data *pack)
{
	/* We'll use the type 0 routing header to get at the
	 * other fields.
	 */
	struct rt0_hdr *rt;
	int count, i;
	char *addrs[ADDRMAX];
	char *args = strdup(arg);

	if (args == NULL) {
		PERROR("route - unable to parse addresses")
		return FALSE;
	}
	count = parsenargs(args, addrs, ADDRMAX, ", ");
	pack->data = realloc(pack->data,
		sizeof(struct rt0_hdr) + count * sizeof(struct in6_addr));
	rt = (struct rt0_hdr *) pack->data;
	pack->alloc_len = sizeof(struct rt0_hdr) + count * sizeof(struct in6_addr);
	for (i = 0; i < count; ++i) {
		if (!inet_pton(AF_INET6, addrs[i], &rt->addr[i])) {
			DERROR("Can't parse address '%s'", addrs[i])
			free(args);
			return FALSE;
		}
	}
	rt->rt_hdr.hdrlen = count << 2;
	free(args);
	return TRUE;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack)
{
	route_header *route = (route_header *) pack->data;
	/* We'll use the type 0 routing header to get at the other fields. */
	struct rt0_hdr *rt = (struct rt0_hdr *) route;
	u_int16_t svalue;

	switch (opt[1]) {
	case 'n':	/* Route next header */
		route->nexthdr = name_to_proto(arg);
		pack->modified |= ROUTE_MOD_NEXTHDR;
		break;
	case 't':	/* Type */
		svalue = opt2inth(arg, NULL, 1);
		if (svalue > OCTET_MAX) {
			DERROR("route - type value too big (%d > %d)", svalue, OCTET_MAX)
			return FALSE;
		}
		route->type = svalue;
		pack->modified |= ROUTE_MOD_TYPE;
		break;
	case 's':	/* Segments left */
		svalue = opt2inth(arg, NULL, 1);
		if (svalue > OCTET_MAX) {
			DERROR("route - segments left value too big (%d > %d)",
				svalue, OCTET_MAX)
			return FALSE;
		}
		route->segments_left = svalue;
		pack->modified |= ROUTE_MOD_SEGMENTS;
		break;
	case 'r':	/* Reserved field (4 bytes) */
		rt->reserved = opt2intn(arg, NULL, 4);
		pack->modified |= ROUTE_MOD_RESV;
		break;
	case 'a':	/* address list */
		if (!readaddrs(arg, pack))
			return FALSE;
		pack->modified |= ROUTE_MOD_ADDRLIST;
		break;
	}
	return TRUE;

}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[], int index,
	__attribute__((unused)) sendip_data *data, sendip_data *pack)
{
	route_header *route = (route_header *) pack->data;

	if (!(pack->modified & ROUTE_MOD_NEXTHDR))
		route->nexthdr = header_type(hdrs[index + 1]);
	return TRUE;
}

int
num_opts() {
	return sizeof(route_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return route_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
