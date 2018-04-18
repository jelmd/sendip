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
	{ "v", 1, "Version", "2" },
	{ "c", 1, "Command", "1" },
	{ "e", 1, "Add entry (family|tag|address|subnet_mask|next_hop|metric)",
		"\n2|0|0.0.0.0|255.255.255.0|0.0.0.0|16" },
	{ "a", 1, "Add auth entry (type|password)", NULL },
	{ "d", 0, "Create a default request", NULL },
	{ "r", 1, "Reserved field", "0" }
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
