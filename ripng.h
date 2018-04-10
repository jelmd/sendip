/* ripng.h */
#ifndef _SENDIP_RIPNG_H
#define _SENDIP_RIPNG_H

/* RIP PACKET STRUCTURES */
typedef struct {
	u_int8_t command;
	u_int8_t version;
	u_int16_t res;
} ripng_header;

typedef struct {
	struct in6_addr prefix;
	u_int16_t tag;
	u_int8_t len;
	u_int8_t metric;
} ripng_entry;

/* Defines for which parts have been modified */
#define RIPNG_MOD_COMMAND   1
#define RIPNG_MOD_VERSION   1<<1
#define RIPNG_MOD_RESERVED  1<<2

/* Options */
sendip_option rip_opts[] = {
	{ "v", 1, "version", "1"},
	{ "c", 1, "command (1=request, 2=response)", "1" },
	{ "r", 1, "reserved field", "0" },
	{ "e", 1, "add entry - address|tag|len|metric", "::|0|128|1"},
	{ "d", 0, "default request (get router's entire routing table", NULL }
};

/* Helpful macros */
#define RIPNG_NUM_ENTRIES(d) \
	(((d)->alloc_len - sizeof(ripng_header)) / sizeof(ripng_entry))
#define RIPNG_ADD_ENTRY(d) {\
	(d)->data = realloc((d)->data, (d)->alloc_len + sizeof(ripng_entry)); \
	(d)->alloc_len += sizeof(ripng_entry); \
}
#define RIPNG_ENTRY(d) ((ripng_entry *) ((u_int32_t *) \
	((d)->data) + ((d)->alloc_len >> 2) - (sizeof(ripng_entry) >> 2)))

#endif  /* _SENDIP_RIPNG_H */
