#ifndef _SENDIP_MODULE_H
#define _SENDIP_MODULE_H

#include <stdbool.h>

/* Options */
typedef struct {
	const char *optname;		/* sub option character */
	const bool arg;				/* expects an argument */
	const char *description;	/* short description of the option */
	const char *def;			/* default value to show in the description */
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

#endif  /* _SENDIP_MODULE_H */
