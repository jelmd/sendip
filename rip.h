/** rip.h
 */
#ifndef _SENDIP_RIP_H
#define _SENDIP_RIP_H

/* RIP PACKET STRUCTURES */
typedef struct {
	u_int8_t command;
	u_int8_t version;
	u_int16_t res;
} rip_header;

typedef struct {
	u_int16_t addressFamily;
	u_int16_t routeTagOrAuthenticationType;
	u_int32_t address;
	u_int32_t subnetMask;
	u_int32_t nextHop;
	u_int32_t metric;
} rip_options;

/* Defines for which parts have been modified */
#define RIP_MOD_COMMAND		1
#define RIP_MOD_VERSION		1<<1
#define RIP_MOD_RESERVED	1<<2

/* Options */
sendip_option rip_opts[] = {
	{ "v", 1, "version", "2" },
	{ "c", 1, "command (1=request, 2=response, 3=traceon (obsolete),\n"
		"               4=traceoff (obsolete), 5=poll (undocumented),\n"
		"               6=poll entry (undocumented)", "1" },
	{ "e", 1,"add entry - family|tag|address|subnet_mask|next_hop|metric",
		"2:0:0.0.0.0:255.255.255.0:0.0.0.0:16" },
	{ "a", 1, "add auth entry - type:password", NULL },
	{ "d", 0, "default request (router's entire routing table)", NULL },
	{ "r", 1, "reserved field", "0" }
};

/* Helpful macros */
#define RIP_NUM_ENTRIES(d) \
	(((d)->alloc_len - sizeof(rip_header)) / sizeof(rip_options))
#define RIP_ADD_ENTRY(d) { \
	(d)->data = realloc((d)->data, (d)->alloc_len + sizeof(rip_options)); \
	(d)->alloc_len += sizeof(rip_options);\
}
#define RIP_OPTION(d) ((rip_options *) ((u_int32_t *) \
	((d)->data) + ((d)->alloc_len >> 2) - (sizeof(rip_options) >> 2)))

#endif  /* _SENDIP_RIP_H */

/* vim: ts=4 sw=4 filetype=c
 */
