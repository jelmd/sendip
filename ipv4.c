/** ipv4.c - IPV4 code for sendip
 * Created by Mike Ricketts <mike@earth.li>
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
#include "ipv4.h"

/* Character that identifies our options */
const char opt_char = 'i';

static void
ipcsum(sendip_data *ip_hdr) {
	ip_header *ip = (ip_header *) ip_hdr->data;
	ip->check = 0;
	ip->check = csum((u_int16_t *) ip_hdr->data, ip_hdr->alloc_len);
}

/* This builds a source route format option from an argument */
static u_int8_t
buildroute(char *data) {
	char *data_out = data;
	char *data_in = data;
	char *next;
	u_int8_t p = '0';
	int i;
	/* First, the first 2 bytes give us the pointer */
	for (i = 0; i < 2; i++) {
		p <<= 4;
		if ( '0' <= *data_in && *data_in <= '9') {
			p += *data_in - '0';
		} else if ('A' <= *data_in && *data_in <= 'F') {
			p += *data_in - 'A' + 0x0A;
		} else if ('a'<=*data_in && *data_in<='f') {
			p += *data_in - 'a' + 0x0a;
		} else {
			ERROR("ipv4 - first 2 chars of record route options must be "
				"hex pointer")
			return 0;
		}
		data_in++;
	}
	*(data_out++) = p;

	/* Now loop through IP addresses... */
	if (*data_in != ':') {
		ERROR("ipv4 - third char of a record route option must be a ':'")
		return 0;
	}
	data_in++;
	next = data_in;
	while (next) {
		u_int32_t ip;
		next = strchr(data_in, ':');
		if (next) {
			*(next++) = 0;
		}
		ip = opt2v4(data_in, strlen(data_in));
		memcpy(data_out, &ip, 4);
		data_out += 4;
		data_in = next;
	}

	return (data_out - data);
}

/* This bears an incredible resemblance to the TCP addoption function... */
static void
addoption(u_int8_t copy, u_int8_t class, u_int8_t num, u_int8_t len,
	u_int8_t *data, sendip_data *pack)
{
	/* opt is copy flag (1bit) + class (2 bit) + number (5 bit) */
	u_int8_t opt = ((copy & 1 ) << 7) | ((class & 3) << 5) | (num & 31);
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
	ip_header *ip = malloc(sizeof(ip_header));
	memset(ip, 0, sizeof(ip_header));
	ret->alloc_len = sizeof(ip_header);
	ret->data = ip;
	ret->modified = 0;
	return ret;
}

bool
set_addr(char *hostname, sendip_data *pack) {
	ip_header *ip = (ip_header *) pack->data;
	struct hostent *host = gethostbyname2(hostname, AF_INET);
	if (!(pack->modified & IP_MOD_SADDR)) {
		ip->saddr = inet_addr("127.0.0.1");
	} 
	if (!(pack->modified & IP_MOD_DADDR)) {
		if (host == NULL)
			return FALSE;

		if (host->h_length != sizeof(ip->daddr)) {
			ERROR("ipv4 - destination address is the wrong size!!!")
			return FALSE;
		}
		memcpy(&(ip->daddr), host->h_addr, host->h_length);
	}
	return TRUE;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack) {
	ip_header *iph = (ip_header *) pack->data;
	switch (opt[1]) {
	case 's':
		iph->saddr = opt2v4(arg, strlen(arg));
		pack->modified |= IP_MOD_SADDR;
		break;
	case 'd':
		iph->daddr = opt2v4(arg, strlen(arg));
		pack->modified |= IP_MOD_DADDR;
		break;
	case 'h':
		iph->header_len = opt2inth(arg, NULL, 1)  & 0xF;
		pack->modified |= IP_MOD_HEADERLEN;
		break;
	case 'v':
		iph->version = opt2inth(arg, NULL, 1) & 0xF;
		pack->modified |= IP_MOD_VERSION;
		break;
	case 'y':
		iph->tos = opt2inth(arg, NULL, 1);
		pack->modified |= IP_MOD_TOS;
		break;
	case 'l':
		/* For reasons unbeknownst to science, the tot_len and frag_off fields
		   in FreeBSD have to be passed in host, rather than network byte order.
		   They are flipped (if need be) in the kernel before transmission. */
#ifdef __FreeBSD
		iph->tot_len = opt2inth(arg, NULL, 2);
#else
		iph->tot_len = opt2intn(arg, NULL, 2);
#endif
		pack->modified |= IP_MOD_TOTLEN;
		break;
	case 'i':
		iph->id = opt2intn(arg, NULL, 2);
		pack->modified |= IP_MOD_ID;
		break;
	case 'f':
		if (opt[2]) {
			/* Note: *arg & 1 is what we want because:
				if arg == "0", *arg & 1 == 0
				if arg == "1", *arg & 1 == 1
				otherwise, it doesn't really matter ...
			*/
			switch (opt[2]) {
			case 'r':
				iph->res = *arg & 1;
				pack->modified |= IP_MOD_RES;
				break;
			case 'd':
				iph->df = *arg & 1;
				pack->modified |= IP_MOD_DF;
				break;
			case 'm':
				iph->mf = *arg & 1;
				pack->modified |= IP_MOD_MF;
				break;
			}
		} else {
			IP_SET_FRAGOFF(iph, opt2inth(arg, NULL, 4) & (u_int16_t) 0x1FFF)
			pack->modified |= IP_MOD_FRAGOFF;
			break;
		}
		break;
	case 't':
		iph->ttl = opt2intn(arg, NULL, 1);
		pack->modified |= IP_MOD_TTL;
		break;
	case 'p':
	   iph->protocol = opt2intn(arg, NULL, 1);
		pack->modified |= IP_MOD_PROTOCOL;
		break;
	case 'c':
		iph->check = opt2intn(arg, NULL, 2);
		pack->modified |= IP_MOD_CHECK;
		break;
	case 'o':
		/* IP options */
		if (strcmp(opt + 2, "num") == 0) {
			/* Other options (auto length) */
			u_int8_t cp, cls, num, len;
			char *src = malloc(strlen(arg) + 3);
			char *dst = malloc((strlen(arg) >> 1) + 2);
			if (src == NULL || dst == NULL) {
				PERROR("Unable to process ipv4 '-o num' option")
				free(src); free(dst);
				return FALSE;
			}
			sprintf(src, "0x%s", arg);
			len = str2val(dst, src, sizeof(dst));
			cp =  (*dst & 0x80) >> 7;
			cls = (*dst & 0x60) >> 5;
			num = (*dst & 0x1F);
			addoption(cp, cls, num, len + 1, (u_int8_t *) dst + 1, pack);
			free(src);
			free(dst);
		} else if (strcmp(opt + 2, "eol") == 0) {
			/* End of list */
			addoption(0, 0, 0, 1, NULL, pack);
		} else if (strcmp(opt + 2, "nop") == 0) {
			/* NOP */
			addoption(0, 0, 1, 1, NULL, pack);
		} else if (strcmp(opt + 2, "rr") == 0) {
			/* Record route. Format is the same as for loose source route */
			char *data = strdup(arg);
			u_int8_t len;
			if (data == NULL) {
				PERROR("ipv4 - unable to process option '-orr'")
				return FALSE;
			}
			len = buildroute(data);
			if (len == 0) {
				free(data);
				return FALSE;
			} else {
				addoption(0, 0, 7, len + 2, (u_int8_t *) data, pack);
				free(data);
			}
		} else if (strcmp(opt + 2, "ssr") == 0) {
			/* Strict Source Route. Format is identical to loose source route */
			char *data = strdup(arg);
			u_int8_t len;
			if (!data) {
				PERROR("ipv4 - unable to process '-ossr' option")
				return FALSE;
			}
			len = buildroute(data);
			if (len == 0) {
				free(data);
				return FALSE;
			} else {
				addoption(1, 0, 9, len + 2, (u_int8_t *) data, pack);
				free(data);
			}
		} else if (strcmp(opt + 2, "lsr") == 0) {
			/* Loose Source Route
			 * Format is:
			 *  type (131, 8bit)
			 *  length (automatic, 8bit)
			 *  pointer (>=4, 8bit)
			 *  ip address0 (32bit)
			 *  ip address1 (32bit)
			 *  ...
			 */
			char *data = strdup(arg);
			u_int8_t len;
			if (!data) {
				PERROR("ipv4 - unable to process '-olsr' option")
				return FALSE;
			}
			len = buildroute(data);
			if (len == 0) {
				free(data);
				return FALSE;
			} else {
				addoption(1, 0, 3, len + 2, (u_int8_t *) data, pack);
				free(data);
			}
		} else if (strcmp(opt + 2, "ts") == 0) {
			/* Time stamp (RFC791)
			 * Format is:
			 *  type (68, 8bit)
			 *  length (automatic, 8bit)
			 *  pointer (8bit)
			 *  overflow (4bit), flag (4bit)
			 *  if (flag) ip1 (32bit)
			 *  timestamp1 (32bit)
			 *  if (flag) ip2 (32bit)
			 *  timestamp2 (32bit)
			 *  ...
			 */
			char *data = strdup(arg);
			char *data_in = data;
			char *data_out = data;
			char *next;
			u_int8_t p = 0;
			int i;
			if (data == NULL) {
				PERROR("ipv4 - unable to process option '-o ts'")
				return FALSE;
			}

			/* First, get the pointer */
			for (i = 0; i < 2; i++) {
				p <<= 4;
				if ('0' <= *data_in && *data_in <= '9') {
					p += *data_in - '0';
				} else if ('A' <= *data_in && *data_in <= 'F') {
					p += *data_in - 'A' + 0x0A;
				} else if ('a' <= *data_in && *data_in <= 'f') {
					p += *data_in - 'a' + 0x0a;
				} else {
					ERROR("ipv4 - first 2 chars of IP timestamp must be "
						"hex pointer")
					free(data);
					return FALSE;
				}
				data_in++;
			}
			*(data_out++) = p;
			
			/* Skip a : */
			if (*(data_in++) != ':') {
				ERROR("ipv4 - third char of IP timestamp must be ':'")
				free(data);
				return FALSE;
			}

			/* Get the overflow and skip a : */
			next = strchr(data_in, ':');
			if (!next) {
				ERROR("ipv4 - timestamp option incorrect")
				free(data);
				return FALSE;
			}
			*(next++) = 0;
			i = opt2inth(data_in, NULL, 1);
			if (i > 15) {
				ERROR("ipbv4 - timestamp overflow (max. 15)")
				free(data);
				return FALSE;
			}
			*data_out = (u_int8_t) (i << 4);
			data_in = next;
			
			/* Now get the flag and skip a : */
			next = strchr(data_in, ':');
			if (!next) {
				ERROR("ipv4 - timestamp option incorrect")
				free(data);
				return FALSE;
			}
			*(next++) = 0;
			i = opt2inth(data_in, NULL, 1);
			if (i > 15) {
				ERROR("ipv4 - timestamp flag too big (max. 15)")
				free(data);
				return FALSE;
			} else if (i != 0 && i != 1 && i != 3) {
				DWARN("ipv4 - timestamp flag value %d not understood", i)
			}
			*data_out += (u_int8_t) i;
			data_in = next;
			data_out++;

			/* Fill in (ip?) timestamp pairs */
			while (next) {
				u_int32_t ts;
				if (i) { /* if we need IPs */
					u_int32_t ip;
					next = strchr(data_in, ':');
					if (!next) {
						ERROR("ipv4 -  address in IP timestamp option must be "
							"followed by a timestamp")
						free(data);
						return FALSE;
					}
					*(next++) = 0;
					ip = opt2v4(data_in, next - data_in);
					memcpy(data_out, &ip, 4);
					data_out += 4;
					data_in = next;
				}
				next = strchr(next, ':');
				if (next)
					*(next++) = 0;
				ts = opt2intn(data_in, NULL, 4);
				memcpy(data_out, &ts, 4);
				data_out += 4;
				data_in = next;
			}
			addoption(0, 2, 4, data_out - data + 2, (u_int8_t *) data, pack);
			free(data);
			/* End of timestamp parsing */
		} else if (strcmp(opt + 2, "sid") == 0) {
			/* Stream ID (RFC791) */
			u_int16_t sid = opt2intn(arg, NULL, 2);
			addoption(1, 0, 8, 4, (u_int8_t *) &sid, pack);
		} else {
			DERROR("ipv4 - unsupported IP option '%s' val '%s'", opt, arg)
			return FALSE;
		}
		break;

	default:
		DERROR("ipv4 - unknown IP option '%s'", opt)
		return FALSE;
	}
	return TRUE;
}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[], int index,
	sendip_data *data, sendip_data *pack)
{
	ip_header *iph = (ip_header *) pack->data;

	if (!(pack->modified & IP_MOD_VERSION)) {
		iph->version = 4;
	}
	if (!(pack->modified & IP_MOD_HEADERLEN)) {
		iph->header_len = (pack->alloc_len + 3) / 4;
	}
	if (!(pack->modified & IP_MOD_TOTLEN)) {
		iph->tot_len = pack->alloc_len + data->alloc_len;
#ifndef __FreeBSD
		iph->tot_len = htons(iph->tot_len);
#endif
	}
	if (!(pack->modified & IP_MOD_ID)) {
		iph->id = rand();
	}
	if (!(pack->modified & IP_MOD_TTL)) {
		iph->ttl = 255;
	}
	if (!(pack->modified & IP_MOD_CHECK)) {
		ipcsum(pack);
	}
	if (!(pack->modified & IP_MOD_PROTOCOL)) {
		/* the actual type of following header */
		iph->protocol = header_type(hdrs[index + 1]);
	}
	return TRUE;
}

int
num_opts() {
	return sizeof(ip_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return ip_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
