/** tcp.c - tcp support for sendip
 * Created by Mike Ricketts <mike@earth.li>
 * TCP options taken from code by Alexander Talos <at@atat.at>
 */

#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#include "sendip_module.h"
#include "common.h"

#include "tcp.h"
#include "ipv4.h"
#include "ipv6.h"

/* Character that identifies our options */
const char opt_char = 't';

static void
tcpcsum(sendip_data *ip_hdr, sendip_data *tcp_hdr, sendip_data *data) {
	tcp_header *tcp = (tcp_header *) tcp_hdr->data;
	ip_header  *ip  = (ip_header *) ip_hdr->data;
	u_int16_t *buf = malloc(12 + tcp_hdr->alloc_len + data->alloc_len);
	u_int8_t *temp = (u_int8_t *) buf;

	if (temp == NULL) {
		PERROR("TCP checksum not computed")
		return;
	}
	/* Set up the pseudo header */
	tcp->check = 0;
	memcpy(temp, &(ip->saddr), sizeof(u_int32_t));
	memcpy(&(temp[4]), &(ip->daddr), sizeof(u_int32_t));
	temp[8] = 0;
	temp[9] = ip->protocol;
	temp[10] = ((tcp_hdr->alloc_len + data->alloc_len) & 0xFF00) >> 8;
	temp[11] = ((tcp_hdr->alloc_len + data->alloc_len) & 0x00FF);
	/* Copy the TCP header and data */
	memcpy(temp + 12, tcp_hdr->data, tcp_hdr->alloc_len);
	memcpy(temp + 12 + tcp_hdr->alloc_len, data->data, data->alloc_len);
	/* CheckSum it */
	tcp->check = csum(buf, 12 + tcp_hdr->alloc_len + data->alloc_len);
	free(buf);
}

static void
tcp6csum(sendip_data *ipv6_hdr, sendip_data *tcp_hdr, sendip_data *data)
{
	tcp_header *tcp = (tcp_header *) tcp_hdr->data;
	ipv6_header  *ipv6  = (ipv6_header *) ipv6_hdr->data;
	struct ipv6_pseudo_hdr phdr;

	u_int16_t *buf =
		malloc(sizeof(phdr) + tcp_hdr->alloc_len + data->alloc_len);
	u_int8_t *temp = (u_int8_t *) buf;
	if (temp == NULL) {
		PERROR("TCP checksum not computed")
		return;
	}

	/* Set up the pseudo header */
	tcp->check = 0;
	memset(&phdr, 0, sizeof(phdr));
	memcpy(&phdr.source, &ipv6->ip6_src, sizeof(struct in6_addr));
	memcpy(&phdr.destination, &ipv6->ip6_dst, sizeof(struct in6_addr));
	phdr.ulp_length = IPPROTO_TCP;
	
	memcpy(temp, &phdr, sizeof(phdr));

	/* Copy the TCP header and data */
	memcpy(temp + sizeof(phdr), tcp_hdr->data, tcp_hdr->alloc_len);
	memcpy(temp + sizeof(phdr) + tcp_hdr->alloc_len, data->data,
		data->alloc_len);

	/* CheckSum it */
	tcp->check = csum(buf, sizeof(phdr) + tcp_hdr->alloc_len + data->alloc_len);
	free(buf);
}

static void
addoption(u_int8_t opt, u_int8_t len, u_int8_t *data, sendip_data *pack)
{
	pack->data = realloc(pack->data, pack->alloc_len + len);
	*((u_int8_t *) pack->data + pack->alloc_len) = opt;
	if (len > 1)
		*((u_int8_t *) pack->data + pack->alloc_len + 1) = len;
	if (len > 2)
		memcpy((u_int8_t *) pack->data + pack->alloc_len + 2, data, len - 2);
	pack->alloc_len += len;
}

sendip_data *
initialize(void) {
	sendip_data *ret = malloc(sizeof(sendip_data));
	tcp_header *tcp = malloc(sizeof(tcp_header));
	memset(tcp, 0, sizeof(tcp_header));
	ret->alloc_len = sizeof(tcp_header);
	ret->data = tcp;
	ret->modified = 0;
	return ret;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	tcp_header *tcp = (tcp_header *)pack->data;

	switch (opt[1]) {
	case 's':
		tcp->source = opt2intn(arg, NULL, 2);
		pack->modified |= TCP_MOD_SOURCE;
		break;
	case 'd':
		tcp->dest = opt2intn(arg, NULL, 2);
		pack->modified |= TCP_MOD_DEST;
		break;
	case 'n':
		tcp->seq = opt2intn(arg, NULL, 4);
		pack->modified |= TCP_MOD_SEQ;
		break;
	case 'a':
		tcp->ack_seq = opt2intn(arg, NULL, 4);
		pack->modified |= TCP_MOD_ACKSEQ;
		if (!(pack->modified & TCP_MOD_ACK)) {
			tcp->ack = 1;
			pack->modified |= TCP_MOD_ACK;
		}
		break;
	case 't':
		tcp->off = opt2intn(arg, NULL, 1) & 0xF;
		pack->modified |= TCP_MOD_OFF;
		break;
	case 'r':
		tcp->res = opt2intn(arg, NULL, 1) & 0xF;
		pack->modified |= TCP_MOD_RES;
		break;
	case 'f':
		switch (opt[2]) {
		case 'e':
			tcp->ecn = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_ECN;
			break;
		case 'c':
			tcp->cwr = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_CWR;
			break;
		case 'u':
			tcp->urg = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_URG;
			break;
		case 'a':
			tcp->ack = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_ACK;
			break;
		case 'p':
			tcp->psh = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_PSH;
			break;
		case 'r':
			tcp->rst = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_RST;
			break;
		case 's':
			tcp->syn = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_SYN;
			break;
		case 'f':
			tcp->fin = (u_int16_t) *arg & 1;
			pack->modified |= TCP_MOD_FIN;
			break;
		default:
			DERROR("TCP flag '-f%c' unknown", opt[2])
			return FALSE;
		}
		break;
	case 'w':
		tcp->window = opt2intn(arg, NULL, 2);
		pack->modified |= TCP_MOD_WINDOW;
		break;
	case 'c':
		tcp->check = opt2intn(arg, NULL, 2);
		pack->modified |= TCP_MOD_CHECK;
		break;
	case 'u':
		tcp->urg_ptr = opt2intn(arg, NULL, 2);
		pack->modified |= TCP_MOD_URGPTR;
		if (!(pack->modified & TCP_MOD_URG)) {
			tcp->urg = 1;
			pack->modified |= TCP_MOD_URG;
		}
		break;
	case 'o':
		/* TCP OPTIONS */
		if (strcmp(opt + 2, "num") == 0) {
			/* Other options (auto length) */
			int len;
			char *src = malloc(strlen(arg) + 3);
			char *dst = malloc((strlen(arg) >> 1) + 2);
			if (src == NULL || dst == NULL) {
				PERROR("Unable to process tcp '-o num' option")
				free(src); free(dst);
				return FALSE;
			}
			sprintf(src, "0x%s", arg);
			len = str2val(dst, src, sizeof(dst));
			if (len == 1)
				addoption(*dst, 1, NULL, pack);
			else
				addoption(*dst, len + 1, (u_int8_t *) dst + 1, pack);
			free(src);
			free(dst);
		} else if (strcmp(opt + 2, "eol") == 0) {
			/* End of options list RFC 793 kind 0, no length */
			addoption(0, 1, NULL, pack);
		} else if (strcmp(opt + 2, "nop") == 0) {
			/* No op RFC 793 kind 1, no length */
			addoption(1, 1, NULL, pack);
		} else if (strcmp(opt + 2, "mss") == 0) {
			/* Maximum segment size RFC 793 kind 2 */
			u_int16_t mss = opt2intn(arg, NULL, 2);
			addoption(2, 4, (u_int8_t *) &mss, pack);
		} else if (strcmp(opt + 2, "wscale") == 0) {
			/* Window scale rfc1323 */
			u_int8_t wscale = opt2inth(arg, NULL, 1);
			addoption(3, 3, &wscale, pack);
		} else if (strcmp(opt + 2, "sackok") == 0) {
			/* Selective Acknowledge permitted rfc1323 */
			addoption(4, 2, NULL, pack);
		} else if (strcmp(opt + 2, "sack") == 0) {
		   /* Selective Acknowledge rfc1323 */
			char *s, *p, *q, c;
			u_int32_t le = 0, re, e = 0;
			u_int8_t *comb, *cpos;
			int count = 1;
			size_t len, i;

			/* get the number of re:le pairs and copy arg to local buffer */
			len = strlen(arg) + 1;
			s = malloc(len * sizeof(char));
			if (s == NULL) {
				PERROR("Unable to process tcp sack option value")
				return FALSE;
			}
			p = s;
			for (i = 0; i < len; i++, p++, arg++) {
				*p = *arg;
				if (*p == ',')
					count++;
			}
			comb = malloc(count * 2 * sizeof(u_int32_t));
			if (comb == NULL) {
				PERROR("Unable to process tcp sack option pair")
				free(s);
				return FALSE;
			}
			cpos = comb;

#define SACKERR \
	DERROR("Invalid TCP sack value in '%s' (pos: %ld)", s, q - s); \
	free(s); \
	free(p); \
	return FALSE;

			p = q = s;
			while (*p != '\0') {
				if (*q == ':') {
					if (e != 0) {
						SACKERR
					}
					q = '\0';
					le = opt2intn(p, NULL, 4);
					p = ++q;
					e++;
				} else if (*q == ',' || *q == '\0') {
					if (e == 0) {
						le = 0;
					} else if (e > 1) {
						SACKERR
					}
					c = *q;
					q = '\0';
					re = opt2intn(p, NULL, 4);
					memcpy(cpos, &le, 4);
					cpos += 4;
					memcpy(cpos, &re, 4);
					cpos += 4;
					if (c == '\0')
						break;
					p = ++q;
					e = 0;
				} else {
					q++;
				}
			}
			addoption(5, sizeof(comb) + 2, comb, pack);
			free(s);
			free(comb);
		} else if (strcmp(opt + 2, "ts") == 0) {
			/* Timestamp rfc1323 */
			u_int32_t tsval = 0, tsecr = 0;
			u_int8_t comb[8];
			if (2 != sscanf(arg, "%u:%u", &tsval, &tsecr)) {
				DERROR("Invalid value '%s' for tcp timestamp option", arg)
				return FALSE;
			}
			tsval = htonl(tsval);
			memcpy(comb, &tsval, 4);
			tsecr = htonl(tsecr);
			memcpy(comb + 4, &tsecr, 4);
			addoption(8, 10, comb, pack);
		} else {
			/* Unrecognized -to* */
			DERROR("unsupported TCP Option '%s'", opt)
			return FALSE;
		} 
		break;
		
	default:
		DERROR("unknown TCP option '-%c'", opt[1])
		return FALSE;
	}

	return TRUE;
}

bool
finalize(char *hdrs, sendip_data *headers[], int index, sendip_data *data,
	sendip_data *pack)
{
	tcp_header *tcp = (tcp_header *) pack->data;
	int i;
	
	/* Set relevant fields */
	if (!(pack->modified & TCP_MOD_SEQ)) {
		tcp->seq = (u_int32_t) rand();
	}
	if (!(pack->modified & TCP_MOD_OFF)) {
		tcp->off = (u_int16_t)((pack->alloc_len + 3) / 4) & 0x0F;
	}
	if (!(pack->modified & TCP_MOD_SYN)) {
		tcp->syn = 1;
	}
	if (!(pack->modified & TCP_MOD_WINDOW)) {
		tcp->window = htons((u_int16_t) 65535);
	}

	/* Find enclosing IP header and do the checksum */
	i = outer_header(hdrs, index, "i6");
	if (hdrs[i] == 'i') {
		if (!(headers[i]->modified & IP_MOD_PROTOCOL)) {
			((ip_header *) (headers[i]->data))->protocol = IPPROTO_TCP;
			headers[i]->modified |= IP_MOD_PROTOCOL;
		}
		if (!(pack->modified & TCP_MOD_CHECK)) {
			tcpcsum(headers[i],pack,data);
		}
	} else if (hdrs[i] == '6') {
		/* next header type gets determined in the ipv6 module */
		if (!(pack->modified & TCP_MOD_CHECK)) {
			tcp6csum(headers[i], pack, data);
		}
	} else if (!(pack->modified & TCP_MOD_CHECK)) {
		ERROR("TCP checksum not defined when TCP is not embedded in IP")
		return FALSE;
	}
	return TRUE;
}

int
num_opts() {
	return sizeof(tcp_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return tcp_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
