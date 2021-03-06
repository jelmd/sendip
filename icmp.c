/** icmp.c - ICMP support for sendip
 * Created by Mike Ricketts <mike@earth.li>
 */

#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#include "sendip_module.h"
#include "common.h"

#include "icmp.h"
#include "ipv4.h"
#include "ipv6.h"

/* Character that identifies our options */
const char opt_char = 'c';

static void
icmpcsum(sendip_data *icmp_hdr, sendip_data *data) {
	icmp_header *icp = (icmp_header *) icmp_hdr->data;
	u_int16_t *buf = malloc(icmp_hdr->alloc_len + data->alloc_len);
	u_int8_t *temp = (u_int8_t *) buf;
	icp->check = 0;
	if (temp == NULL) {
		PERROR("ICMP checksum not computed")
		return;
	}
	memcpy(temp, icmp_hdr->data, icmp_hdr->alloc_len);
	memcpy(temp + icmp_hdr->alloc_len, data->data, data->alloc_len);
	icp->check = csum(buf, icmp_hdr->alloc_len + data->alloc_len);
	free(buf);
}

static void
icmp6csum(struct in6_addr *src, struct in6_addr *dst, sendip_data *hdr,
	sendip_data *data)
{
	icmp_header *icp = (icmp_header *) hdr->data;
	struct ipv6_pseudo_hdr phdr;

	/* Make sure tempbuf is word aligned */
	u_int16_t *buf = malloc(sizeof(phdr) + hdr->alloc_len + data->alloc_len);
	u_int8_t *temp = (u_int8_t *) buf;
	icp->check = 0;
	if (temp == NULL) {
		PERROR("ICMP checksum not computed")
		return;
	}
	memcpy(temp + sizeof(phdr), hdr->data, hdr->alloc_len);
	memcpy(temp + sizeof(phdr) + hdr->alloc_len, data->data, data->alloc_len);

	/* do an ipv6 checksum */
	memset(&phdr, 0, sizeof(phdr));
	memcpy(&phdr.source, src, sizeof(struct in6_addr));
	memcpy(&phdr.destination, dst, sizeof(struct in6_addr));
	phdr.ulp_length = htonl(hdr->alloc_len + data->alloc_len);
	phdr.nexthdr = IPPROTO_ICMPV6;
	
	memcpy(temp, &phdr, sizeof(phdr));
	
	icp->check = csum(buf, sizeof(phdr) + hdr->alloc_len + data->alloc_len);
	free(buf);
}

sendip_data *
initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	icmp_header *icmp = malloc(sizeof(icmp_header));
	memset(icmp, 0, sizeof(icmp_header));
	ret->alloc_len = sizeof(icmp_header);
	ret->data = icmp;
	ret->modified = 0;
	return ret;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	icmp_header *icp = (icmp_header *) pack->data;
	switch (opt[1]) {
	case 't':
		icp->type = opt2intn(arg, NULL, 1);
		pack->modified |= ICMP_MOD_TYPE;
		break;
	case 'd':
		icp->code = opt2intn(arg, NULL, 1);
		pack->modified |= ICMP_MOD_CODE;
		break;
	case 'c':
		icp->check = opt2intn(arg, NULL, 2);
		pack->modified |= ICMP_MOD_CHECK;
		break;
	}
	return TRUE;
}

bool
finalize(char *hdrs, sendip_data *headers[], int index, sendip_data *data,
	sendip_data *pack)
{
	icmp_header *icp = (icmp_header *) pack->data;

	/* Search backward for the first v4 or v6 header and use that. */
	int i = outer_header(hdrs, index, "i6");
	if (i < 0) {
		ERROR("icmp - can't find ip header")
		return FALSE;
	}

	/* Check the enclosing IPv4 header for type. For IPv6 the next header type
	   gets determined in the ipv6 module. */
	if (hdrs[i] == 'i' && !(headers[i]->modified & IP_MOD_PROTOCOL)) {
		((ip_header *) (headers[i]->data))->protocol = IPPROTO_ICMP;
		headers[i]->modified |= IP_MOD_PROTOCOL;
	}
		
	if (!(pack->modified & ICMP_MOD_TYPE)) {
		if (hdrs[i] == '6') {
			icp->type = ICMP6_ECHO_REQUEST;
		} else {
			icp->type = ICMP_ECHO;
		}
	}

	/* Do the checksum */
	if (!(pack->modified & ICMP_MOD_CHECK)) {
		if (hdrs[i] == '6') {
			// ipv6
			struct in6_addr *src, *dst;
			src = (struct in6_addr *)
				&(((ipv6_header *) (headers[i]->data))->ip6_src);
			dst = (struct in6_addr *)
				&(((ipv6_header *) (headers[i]->data))->ip6_dst);
			icmp6csum(src, dst, pack, data);
		} else {
			/* ipv4 or anything else */
			icmpcsum(pack, data);
		}
	}
	return TRUE;
}

int
num_opts() {
	return sizeof(icmp_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return icmp_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
