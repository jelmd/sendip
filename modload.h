#ifndef _MODLOD_H
#define _MODLOD_H

crypto_module *load_crypto_module(char *modname);
sendip_module *load_sendip_module(char *modname, int *cached);
void unload_modules(int verbosity);

#endif
