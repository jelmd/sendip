#ifndef _CRYPTO_MODULE_H
#define _CRYPTO_MODULE_H

/* The loaded module - see load_sendip_module() */
typedef struct {
	char *name;
	bool (*cryptoinit)(void *priv);
	bool (*cryptomod)(void *priv, char *hdrs, sendip_data *headers[],
		int index, sendip_data *data, sendip_data *pack);
} crypto_module;

/* Prototypes */
#ifndef _CRYPTO_MAIN
bool cryptoinit(sendip_data *pack);
bool cryptomod(void *priv, char *hdrs, sendip_data *headers[],
	int index, sendip_data *data, sendip_data *pack);
#endif  /* _CRYPTO_MAIN */

#endif  /* _CRYPTO_MODULE_H */
