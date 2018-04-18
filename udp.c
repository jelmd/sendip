/** udp.c - UDP code for sendip
 * Created by Mike Ricketts <mike@earth.li>
 */

#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#include "sendip_module.h"
#include "common.h"

#include "udp.h"
#include "ipv4.h"
#include "ipv6.h"

/* Character that identifies our options */
const char opt_char = 'u';

static void
udpcsum(sendip_data *ip_hdr, sendip_data *udp_hdr, sendip_data *data) {
	udp_header *udp = (udp_header *) udp_hdr->data;
	ip_header *ip  = (ip_header *) ip_hdr->data;
	u_int16_t *buf = malloc(12 + udp_hdr->alloc_len + data->alloc_len);
	u_int8_t *temp = (u_int8_t *) buf;
	udp->check = 0;
	if (temp == NULL) {
		PERROR("UDP checksum not computed")
		return;
	}
	/* Set up the pseudo header */
	memcpy(temp, &(ip->saddr), sizeof(u_int32_t));
	memcpy(&(temp[4]), &(ip->daddr), sizeof(u_int32_t));
	temp[8] = 0;
	temp[9] = ip->protocol;
	temp[10] = ((udp_hdr->alloc_len + data->alloc_len) & 0xFF00) >> 8;
	temp[11] = ((udp_hdr->alloc_len + data->alloc_len) & 0x00FF);
	/* Copy the UDP header and data */
	memcpy(temp + 12, udp_hdr->data, udp_hdr->alloc_len);
	memcpy(temp + 12 + udp_hdr->alloc_len, data->data, data->alloc_len);
	/* CheckSum it */
	udp->check = csum(buf, 12 + udp_hdr->alloc_len + data->alloc_len);
	free(buf);
}

static void
udp6csum(sendip_data *ipv6_hdr, sendip_data *udp_hdr, sendip_data *data) {
	udp_header *udp = (udp_header *) udp_hdr->data;
	ipv6_header *ipv6  = (ipv6_header *) ipv6_hdr->data;
	struct ipv6_pseudo_hdr phdr;

	u_int16_t *buf =
		malloc(sizeof(phdr) + udp_hdr->alloc_len + data->alloc_len);
	u_int8_t *temp = (u_int8_t *) buf;
	udp->check = 0;
	if (temp == NULL) {
		PERROR("UDP checksum not computed")
		return;
	}

	/* Set up the pseudo header */
	memset(&phdr, 0, sizeof(phdr));
	memcpy(&phdr.source, &ipv6->ip6_src, sizeof(struct in6_addr));
	memcpy(&phdr.destination, &ipv6->ip6_dst, sizeof(struct in6_addr));
	phdr.nexthdr = IPPROTO_UDP;
	phdr.ulp_length = udp->len;
	
	memcpy(temp, &phdr, sizeof(phdr));

	/* Copy the UDP header and data */
	memcpy(temp + sizeof(phdr),udp_hdr->data, udp_hdr->alloc_len);
	memcpy(temp + sizeof(phdr) + udp_hdr->alloc_len, data->data, data->alloc_len);

	/* CheckSum it */
	udp->check = csum(buf, sizeof(phdr) + udp_hdr->alloc_len + data->alloc_len);
	free(buf);
}

sendip_data *
initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	udp_header *udp = malloc(sizeof(udp_header));
	memset(udp, 0, sizeof(udp_header));
	ret->alloc_len = sizeof(udp_header);
	ret->data = udp;
	ret->modified = 0;
	return ret;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	udp_header *udp = (udp_header *) pack->data;

	switch (opt[1]) {
	case 's':
		udp->source = opt2intn(arg, NULL, 2);
		pack->modified |= UDP_MOD_SOURCE;
		break;
	case 'd':
		udp->dest = opt2intn(arg, NULL, 2);
		pack->modified |= UDP_MOD_DEST;
		break;
	case 'l':
		udp->len = opt2intn(arg, NULL, 2);
		pack->modified |= UDP_MOD_LEN;
		break;
	case 'c':
		udp->check = opt2intn(arg, NULL, 2);
		pack->modified |= UDP_MOD_CHECK;
		break;
	}
	return TRUE;
}

bool
finalize(char *hdrs, sendip_data *headers[], int index, sendip_data *data,
	sendip_data *pack)
{
	udp_header *udp = (udp_header *) pack->data;
	int i;
	
	/* Set relevant fields */
	if (!(pack->modified & UDP_MOD_LEN)) {
		udp->len = htons(pack->alloc_len + data->alloc_len);
	}

	/* Find enclosing IP header and do the checksum */
	i = outer_header(hdrs, index, "i6");
	if (hdrs[i] == 'i') {
		if (!(headers[i]->modified & IP_MOD_PROTOCOL)) {
			((ip_header *) (headers[i]->data))->protocol = IPPROTO_UDP;
			headers[i]->modified |= IP_MOD_PROTOCOL;
		}
		if (!(pack->modified & UDP_MOD_CHECK)) {
			udpcsum(headers[i], pack, data);
		}
	} else if (hdrs[i] == '6') {
		/* next header type gets determined in the ipv6 module */
		if (!(pack->modified & UDP_MOD_CHECK)) {
			udp6csum(headers[i], pack, data);
		}

	} else if (!(pack->modified & UDP_MOD_CHECK)) {
		ERROR("UDP checksum not defined when UDP is not embedded in IP")
		return FALSE;
	}

	return TRUE;
}

int
num_opts() {
	return sizeof(udp_opts)/sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return udp_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
