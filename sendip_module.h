#ifndef _SENDIP_MODULE_H
#define _SENDIP_MODULE_H

#include <stdio.h>   // for fprintf
#include "types.h"

/* Options */
typedef struct {
	const char *optname;
	const bool arg;
	const char *description;
	const char *def;
} sendip_option;

/* Data */
typedef struct {
	void *data;
	int alloc_len;
	unsigned int modified;
	void *private;		/* Untouched by sendip main */
} sendip_data;

/* The loaded module - see load_sendip_module() */
typedef struct {
	char *name;
	char optchar;
	int num_opts;

	sendip_option *opts;
	sendip_data * (*initialize)(void);
	bool (*do_opt)(const char *optstring, const char *optarg, sendip_data *pack);
	bool (*set_addr)(char *hostname, sendip_data *pack);
	bool (*finalize)(char *hdrs, sendip_data *headers[], int index,
		sendip_data *data, sendip_data *pack);
} sendip_module;

/* Prototypes */
#ifndef _SENDIP_MAIN
sendip_data *initialize(void);
bool do_opt(char *optstring, char *optarg, sendip_data *pack);
bool set_addr(char *hostname, sendip_data *pack);
bool finalize(char *hdrs, sendip_data *headers[], int index, sendip_data *data,
				  sendip_data *pack);
int num_opts(void);
sendip_option *get_opts(void);
char get_optchar(void);

#endif  /* _SENDIP_MAIN */

#define usage_error(x) fprintf(stderr,x)

extern u_int16_t csum(u_int16_t *packet, int packlen);
extern int compact_string(char *data_out);

#define MAXRAND	8192	/* maximum length of random data */
u_int8_t * randombytes(int length);
int stringargument(char *input, char **output);

const char * proto_to_name(u_int8_t proto, int nolookup);
u_int8_t name_to_proto(char *s);
u_int8_t header_type(const char hdr_char);
int outer_header(const char *hdrs, int index, const char *choices);
int inner_header(const char *hdrs, int index, const char *choices);

extern u_int16_t csumv(u_int16_t *packet[], int packlen[]);

#endif  /* _SENDIP_MODULE_H */
