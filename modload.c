#define _CRYPTO_MAIN
#define _SENDIP_MAIN

#include <dlfcn.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "c_origin.h"
#include "sendip_module.h"
#include "crypto_module.h"
#include "modload.h"

typedef struct _sm {
	struct _sm *next;
	struct _sm *prev;
	void *handle;
	sendip_module *mod;
} sendip_mod_entry;

typedef struct _cm {
	struct _cm *next;
	struct _cm *prev;
	void *handle;
	crypto_module *mod;
} crypto_mod_entry;

/* paranoid? */
static sendip_mod_entry *first = NULL;
static sendip_mod_entry *last = NULL;
static crypto_mod_entry *first_crypto = NULL;
static crypto_mod_entry *last_crypto = NULL;

static void *
dlopen_module(const char *modname) {
	static char *libdir = NULL;
	char name[MAXPATHLEN];
	void *handle;

	if (libdir == NULL) {
		libdir = get_origin_rel(1, "lib/sendip", SENDIP_LIBS);
	}

	strcpy(name, modname);
	if (NULL == (handle = dlopen(name, RTLD_NOW)) ) {
		char *error0 = strdup(dlerror());
		snprintf(name, MAXPATHLEN, "./%s.so", modname);
		if (NULL == (handle = dlopen(name, RTLD_NOW)) ) {
			char *error1 = strdup(dlerror());
			snprintf(name, MAXPATHLEN, "%s/%s.so", libdir, modname);
			if (NULL == (handle = dlopen(name, RTLD_NOW)) ) {
				char *error2 = strdup(dlerror());
				snprintf(name, MAXPATHLEN, "%s/%s", libdir, modname);
				if (NULL == (handle = dlopen(name, RTLD_NOW)) ) {
					char *error3 = strdup(dlerror());
					fprintf(stderr, "Couldn't open module %s, tried:\n"
						"  %s\n  %s\n  %s\n  %s\n",
						modname, error0, error1, error2, error3);
					free(error3);
					return NULL;
				}
				free(error2);
			}
			free(error1);
		}
		free(error0);
	}
	return handle;
}

crypto_module *
load_crypto_module(const char *modname) {
	crypto_module *newmod;
	crypto_mod_entry *cur;
	void *handle;

	for (cur = first_crypto; cur != NULL; cur = cur->next) {
		if (strcmp(modname, cur->mod->name) == 0)
			return cur->mod;
	}

	newmod = malloc(sizeof(crypto_module));
	if (newmod == NULL) {
		perror("Unable to allocate module memory");
		return NULL;
	}

	if (NULL == (handle = dlopen_module(modname)) ) {
		free(newmod);
		return NULL;
	}
#pragma error_messages (off, E_ASSIGNMENT_TYPE_MISMATCH)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	/* Initialize is optional */
	if (NULL == (newmod->cryptoinit = dlsym(handle, "cryptoinit")) ) {
		fprintf(stderr, "Warning: %s doesn't have an cryptoinit function: %s\n",
			modname, dlerror());
	}
	/* cryptomod is not */
	if (NULL == (newmod->cryptomod = dlsym(handle, "cryptomod")) ) {
		fprintf(stderr, "%s doesn't contain a cryptomod function: %s\n",
			modname, dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
#pragma GCC diagnostic pop
#pragma error_messages (default, E_ASSIGNMENT_TYPE_MISMATCH)
	newmod->name = malloc(strlen(modname) + 1);
	if (newmod->name == NULL) {
		perror("No space left to copy module name");
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	strcpy(newmod->name, modname);

	cur = malloc(sizeof(crypto_mod_entry));
	if (cur == NULL) {
		perror("Unable to add crypto module");
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	cur->prev = last_crypto;
	cur->next = NULL;
	cur->mod = newmod;
	cur->handle = handle;
	last_crypto = cur;
	if (last_crypto->prev)
		last_crypto->prev->next = last_crypto;
	if (!first_crypto)
		first_crypto = last_crypto;

	return newmod;
}

sendip_module *
load_sendip_module(const char *modname, int *cached) {
	sendip_module *newmod;
	sendip_mod_entry *cur;
	void *handle;

	int (*n_opts)(void);
	sendip_option * (*get_opts)(void);
	char (*get_optchar)(void);

	for (cur = first; cur != NULL; cur = cur->next) {
		if (strcmp(modname, cur->mod->name) == 0) {
			*cached = 1;
			return cur->mod;
		}
	}

	*cached = 0;
	newmod = malloc(sizeof(sendip_module));
	if (newmod == NULL) {
		perror("Unable to allocate module memory");
		return NULL;
	}
	if (NULL == (handle = dlopen_module(modname)) ) {
		free(newmod);
		return NULL;
	}
#pragma error_messages (off, E_ASSIGNMENT_TYPE_MISMATCH)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	if (NULL == (newmod->initialize = dlsym(handle, "initialize")) ) {
		fprintf(stderr, "%s doesn't have an initialize function: %s\n",
			modname, dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	if (NULL == (newmod->do_opt = dlsym(handle, "do_opt")) ) {
		fprintf(stderr, "%s doesn't contain a do_opt function: %s\n",
			modname, dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	newmod->set_addr = dlsym(handle, "set_addr"); // don't care if fails
	if (NULL == (newmod->finalize = dlsym(handle, "finalize")) ) {
		fprintf(stderr, "%s\n", dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	if (NULL == (n_opts = dlsym(handle, "num_opts")) ) {
		fprintf(stderr, "%s\n", dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	if (NULL == (get_opts = dlsym(handle, "get_opts")) ) {
		fprintf(stderr, "%s\n", dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	if (NULL == (get_optchar = dlsym(handle, "get_optchar")) ) {
		fprintf(stderr, "%s\n", dlerror());
		dlclose(handle);
		free(newmod);
		return NULL;
	}
#pragma GCC diagnostic pop
#pragma error_messages (default, E_ASSIGNMENT_TYPE_MISMATCH)
	newmod->name = malloc(strlen(modname) + 1);
	if (newmod->name == NULL) {
		perror("No space left to copy module name");
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	strcpy(newmod->name, modname);
	newmod->num_opts = n_opts();
	newmod->optchar = get_optchar();
	/* TODO: check uniqueness */
	newmod->opts = get_opts();

	cur = malloc(sizeof(sendip_mod_entry));
	if (cur == NULL) {
		perror("Unable to add sendip module");
		dlclose(handle);
		free(newmod);
		return NULL;
	}
	cur->prev = last;
	cur->next = NULL;
	cur->mod = newmod;
	cur->handle = handle;
	last = cur;
	if (last->prev)
		last->prev->next = last;
	if (!first)
		first = last;

	return newmod;
}

/* Unload all loaded modules.  Should only by called by _main_ */
void
unload_modules(int verbosity) {
	sendip_mod_entry *lis, *p;
	crypto_mod_entry *lic, *p_crypto;
	p = NULL;
	for (lis = first; lis != NULL; lis = lis->next) {
		if (verbosity)
			printf("Freeing module %s\n", lis->mod->name);
		if (p)
			free(p);
		p = lis;
		free(p->mod->name);
		(void) dlclose(p->handle);
		free(p->mod);
		/* Do _not_ free options - usually static/non-malloced stuff */
	}
	if (p)
		free(p);
	p_crypto = NULL;
	for (lic = first_crypto; lic != NULL; lic = lic->next) {
		if (verbosity)
			printf("Freeing module %s\n", lic->mod->name);
		if (p_crypto)
			free(p_crypto);
		p_crypto = lic;
		free(lic->mod->name);
		(void) dlclose(lic->handle);
		free(lic->mod);
	}
	if (p_crypto)
		free(p_crypto);
}
