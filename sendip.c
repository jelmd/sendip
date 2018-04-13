/** sendip.c - main program code for sendip
 * Copyright 2001 Mike Ricketts <mike@earth.li>
 * License: see LICENSE
 */

#define _SENDIP_MAIN

#include <netdb.h>

/* everything else */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#ifdef __sun  /* for EVILNESS workaround */
#include "ipv4.h"
#endif /* __sun */

/* Use our own getopt to ensure consistent behaviour on all platforms */
#include "gnugetopt.h"

#include "sendip_module.h"
#include "crypto_module.h"
#include "common.h"
#include "modload.h"
#include "ipv6.h"
#include "dump.h"

/* housekeeping for loaded modules and their packet data */
typedef struct _sml {
	struct _sml *next;
	struct _sml *prev;
	sendip_module *mod;
	sendip_data *pack;
	int num_opts;		/* number of options for the module to keep in mind */
} sendip_mod_li;
static sendip_mod_li *first;
static sendip_mod_li *last;

/* sockaddr_storage struct is not defined everywhere, so here is our own
	nasty version
*/
typedef struct {
	u_int16_t ss_family;
	u_int32_t ss_align;
	char ss_padding[122];
} _sockaddr_storage;

static int num_opts = 0;

static char *progname;

static int
sendpacket(sendip_data *data, char *hostname, int af_type, bool verbose,
	char dump, const char *sockopts)
{
	_sockaddr_storage *to = malloc(sizeof(_sockaddr_storage));
	const char *p;

	/* socket stuff */
	int s;										/* socket for sending */
	int sent = -2;								/* number of bytes sent */
	bool sethdrincl = FALSE, setipv6opts = FALSE;
	const int on = 1;

	/* hostname stuff */
	struct hostent *host = NULL;				/* result of gethostbyname2 */

	/* casts for specific protocols */
	struct sockaddr_in *to4 = (struct sockaddr_in *) to;	/* IPv4 */
	struct sockaddr_in6 *to6 = (struct sockaddr_in6 *) to;	/* IPv6 */
	int tolen;

	if (to == NULL) {
		PERROR("Unable to send packet")
		return -3;
	}
	if ((host = gethostbyname2(hostname, af_type)) == NULL) {
		PERROR("gethostbyname2('%s', %d) failed", hostname, af_type)
		free(to);
		return -1;
	}
	memset(to, 0, sizeof(_sockaddr_storage));

	switch (af_type) {
	case AF_INET:
		to4->sin_family = host->h_addrtype;
		memcpy(&to4->sin_addr, host->h_addr, host->h_length);
		tolen = sizeof(struct sockaddr_in);
		sethdrincl = TRUE;
		break;
	case AF_INET6:
		to6->sin6_family = host->h_addrtype;
		memcpy(&to6->sin6_addr, host->h_addr, host->h_length);
		tolen = sizeof(struct sockaddr_in6);
		setipv6opts = TRUE;
		break;
	default:
		return -2;
	}

	if (dump) {
		char buf[BUFSIZ];

		bdump(data->data, data->alloc_len, buf, BUFSIZ, dump == 'h');
		printf("Final packet data:\n%s\n", buf);
	}

	if ((s = socket(af_type, SOCK_RAW, IPPROTO_RAW)) < 0) {
		PERROR("Couldn't open RAW socket")
		free(to);
		return -1;
	}

	/* Set socket options */
	if (verbose)
		printf("Setting socket options:\n");
	for (p = sockopts == NULL ? "" : sockopts; *p != '\0'; p++) {
		switch (*p) {
		case 'b':
			if (verbose)
				printf(" SO_BROADCAST\n");
			if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
				PERROR("Couldn't setsockopt SO_BROADCAST")
				goto error;
			}
			break;
		case 'i':
			sethdrincl = FALSE;
			break;
		case '6':
			setipv6opts = FALSE;
			break;
		default:
			DWARN("Invalid socket option '%c' ignored", *p)
			break;
		}
	}
	if (sethdrincl) {
		if (verbose)
			printf(" IP_HDRINCL\n");
		if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
			PERROR("Couldn't setsockopt IP_HDRINCL")
			goto error;
		}
	}
	if (setipv6opts) {
		ipv6_header *iphdr = (ipv6_header *) data->data;
		if (verbose)
			printf(" IPV6_UNICAST_HOPS\n");
		/* Setting various IPV6 header option requires using setsockopt, as
		   in RFCs 3493, 3542, 2292. */
		if (setsockopt(s, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
			&(iphdr->ip6_hlim), sizeof(iphdr->ip6_hlim)) < 0)
		{
			PERROR("Couldnt set Sock options for IPv6:Hop Limit")
			goto error;
		}
	}

	/* On Solaris, it seems that the only way to send IP options or packets
		with a faked IP header length is to:
		setsockopt(IP_OPTIONS) with the IP option data and size
		decrease the total length of the packet accordingly
		I'm sure this *shouldn't* work.  But it does.
	*/
#ifdef __sun
	if ((*((char *) (data->data)) & 0x0F) != 5) {
		ip_header *iphdr = (ip_header *) data->data;
		int optlen = iphdr->header_len * 4 - 20;

		if (verbose) {
			printf(" IP_OPTIONS\n"
				"Solaris workaround enabled for %d IP option bytes\n", optlen);
		}
		iphdr->tot_len = htons(ntohs(iphdr->tot_len) - optlen);

		if (setsockopt(s, IPPROTO_IP, IP_OPTIONS,
			(char *)(data->data) + 20, optlen))
		{
			PERROR("Couldn't setsockopt IP_OPTIONS")
			goto error;
		}
	}
#endif /* __sun */

	/* Send the packet */
	sent =
		sendto(s, (char *) data->data, data->alloc_len, 0, (void *) to, tolen);
	if (sent == data->alloc_len) {
		if (verbose)
			printf("Sent %d bytes to %s\n", sent, hostname);
	} else if (sent < 0) {
		PERROR("Packet not sent")
	} else if (verbose) {
		DWARN("Only sent %d/%d bytes to %s", sent, data->alloc_len, hostname)
	}
error:
	free(to);
	close(s);
	return sent;
}

static void print_usage(void) {
	sendip_mod_li *e;
	int i;
	char lbuf[LINE_MAX];

	printf(
"\nUsage: %s [-hVv] [-D otype] [-d data] [-f datafile] [-L count] [-T time]\\"
"\n         \t[-S socket_opts] [-p module]... [module_option]... hostname\n"
"\n"
"Packet data, header fields:\n"
"  fF  .. next line from file F\n"
"  rN  .. N random bytes\n"
"  zN  .. N zero (nul) bytes\n"
"  tN  .. timestamp with zero padded tail of length N\n"
"  0xH .. data in hex\n"
"  0N  .. data in octal\n"
"  *   .. literal string, decimal number, IP addr, etc. depending on opt type\n"
"\nSocket options:\n"
"  b .. set SO_BROADCAST option\n"
"  i .. IPv4: disable IP header inclusion\n"
"  6 .. IPv6: disable IPv6 headers (IPV6_UNICAST_HOPS)\n"
"\n\n"
"Modules available at compile time:\n"
"    ipv4 ipv6 icmp tcp udp bgp rip ripng ntp\n"
"    ah dest esp frag gre hop route sctp wesp\n"
, progname);

	for (e = first; e != NULL; e = e->next) {
		if (e->num_opts == 0)
			continue;
		sendip_module *mod = e->mod;
		char *shortname = strrchr(mod->name, '/');

		if (!shortname)
			shortname = mod->name;
		else
			++shortname;
		printf("\n\nArguments for module %s:\n", shortname);
		for (i = 0; i < e->num_opts; i++) {
			snprintf(lbuf, LINE_MAX, "-%c%s %c", mod->optchar,
				mod->opts[i].optname, mod->opts[i].arg ? 'x' : ' ');
			printf("  %10s   %s.", lbuf, mod->opts[i].description);
			if (mod->opts[i].def)
				printf("  Default: %s\n", mod->opts[i].def);
			else
				printf("\n");
		}
	}

}

static void
unload_mods(bool freeit, int verbosity) {
	sendip_mod_li *e, *p;

	p = NULL;
	for (e = first; e != NULL; e = e->next) {
		if (p)
			free(p);
		p = e;
		if (freeit)
			free(p->pack->data);
		free(p->pack);
	}
	if (p)
		free(p);
	unload_modules(verbosity);
	fargs_destroy();
}

int
main(int argc, char **const argv) {
	int i;

	struct option *opts = NULL;
	char *sockopts = NULL;
	int longindex = 0, usage = 0;
	char rbuff[31];

	bool verbosity = FALSE;

	char *data = NULL, dump = '\0';
	int datafile = -1;
	int datalen = 0;
	char *datarg = NULL;

	sendip_module *mod;
	sendip_mod_li *e, *current_e;
	int optc;

	int num_modules = 0;

	sendip_data packet;

	int loopcount = 1;
	unsigned int delaytime = 0;
	
	num_opts = 0;	
	first = last = NULL;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		progname++;

	/* magic random seed that gives 4 really random octets */
	srandom(time(NULL) ^ (getpid() + (42 << 15)));

	/* First, get all the builtin options, and load the modules */
	gnuopterr = 0;
	gnuoptind = 0;
	while(gnuoptind < argc
		&& (EOF != (optc = gnugetopt(argc, argv, "-p:vD:d:hf:L:S:T:V"))))
	{
		switch (optc) {
		case 'V':
			printf("sendip v%s - see https://github.com/jelmd/sendip\n",
				VERSION);
			return 0;
		case 'S':
			sockopts = strdup(gnuoptarg);
			if (sockopts == NULL) {
				PERROR("Couldn't allocate memory for socket options")
				return 1;
			}
			break;
		case 'L':
			loopcount = atoi(gnuoptarg);
			break;
		case 'T':
			delaytime = atoi(gnuoptarg);
			break;
		case 'p':
			if ((mod = load_sendip_module(gnuoptarg, &i)) != NULL) {
				e = malloc(sizeof(sendip_mod_li));
				if (e == NULL) {
					PERROR("Unable to process option -p ...");
					return 1;
				}
				e->prev = last;
				e->next = NULL;
				e->pack = NULL;
				e->mod = mod;
				e->num_opts = i ? 0 : mod->num_opts;
				last = e;
				if (last->prev)
					last->prev->next = last;
				if (!first)
					first = last;
				num_modules++;
				num_opts += e->num_opts;
			}
			break;
		case 'v':
			verbosity = TRUE;
			break;
		case 'D':
			dump = gnuoptarg[0] == 'h' ? 'h' : 'd';
			break;
		case 'd':
			if (datafile == -1) {
				char sdata[BUFSIZ];

				datarg = gnuoptarg;						/* save for regen */
				datalen = opt2val(sdata, datarg, BUFSIZ);
				data = (char *) malloc(datalen);
				if (data == NULL) {
					PERROR("Unable to process option -d ...")
					return 1;
				}
				memcpy(data, sdata, datalen);
			} else {
				ERROR("Only one -d or -f option can be given")
				usage = 1;
			}
			break;
		case 'h':
			usage = 2;
			break;
		case 'f':
			if (data == NULL) {
				datafile = open(gnuoptarg, O_RDONLY);
				if (datafile == -1) {
					PERROR("Couldn't open data file")
					WARN("No data will be included")
				} else {
					datalen = lseek(datafile, 0, SEEK_END);
					if (datalen == -1) {
						PERROR("Error reading data file: lseek()")
						WARN("No data will be included")
						datalen = 0;
					} else if (datalen == 0) {
						WARN("Data file is empty.\nNo data will be included")
					} else {
						data = mmap(NULL, datalen, PROT_READ, MAP_SHARED, datafile, 0);
						if (data == MAP_FAILED) {
							PERROR("Couldn't read data file: mmap()")
							WARN("No data will be included")
							data = NULL;
							datalen = 0;
						}
					}
				}
			} else {
				ERROR("Only one -d or -f option can be given")
				usage = 1;
			}
			break;
		case '?':
		case ':':
			/* skip any further characters in this option
				this is so that -tonop doesn't cause a -p option
			*/
			nextchar = NULL; gnuoptind++;
			break;
		}
	}

/* loop - need to start before module opt processing ... */
while (--loopcount >= 0) {

	/* Build the getopt listings */
	opts = malloc((1 + num_opts) * sizeof(struct option));
	if (opts == NULL) {
		PERROR("Unable to process module options")
		return 1;
	}
	memset(opts, 0, (1 + num_opts) * sizeof(struct option));
	i = 0;
	for (e = first; e != NULL; e = e->next) {
		mod = e->mod;
		int j;
		char *s;   // nasty kludge because option.name is const
		for (j = 0; j < e->num_opts; j++) {
			/* +2 on next line is one for the char, one for the trailing null */
			opts[i].name = s = malloc(strlen(mod->opts[j].optname) + 2);
			sprintf(s, "%c%s", mod->optchar, mod->opts[j].optname);
			opts[i].has_arg = mod->opts[j].arg;
			opts[i].flag = NULL;
			opts[i].val = mod->optchar;
			i++;
		}
	}
	if (verbosity)
		printf("Added %d options\n", num_opts);

	/* Initialize all */
	for (e = first; e != NULL; e = e->next) {
		mod = e->mod;
		if (verbosity)
			printf("Initializing module %s\n", mod->name);
		if (e->pack)
			free(e->pack);	/* may contain data when looping */
		e->pack = mod->initialize();
	}

	/* Get opt like getopt_long, but '-' as well as '--' can indicate a long
	 * option. If an option that starts with '-' (not '--') doesn't match a
	 * long option, but does match a short option, it is parsed as a short
	 * option instead.
	 *
	 * Apply options to the most recently invoked module first to allow
	 * separate arguments for multiply-invoked modules, e.g. for creating ipip
	 * tunneled packets.
	 */
	gnuopterr = 1;
	gnuoptind = 0;
	current_e = NULL;
	while(EOF != (optc =
		_getopt_internal(argc, argv, "p:vD:d:hf:L:S:T:V", opts, &longindex, 1)))
	{
		switch (optc) {
		case 'p':
			current_e = (current_e) ? current_e->next : first;
			break;
		case 'v':
		case 'D':
		case 'd':
		case 'f':
		case 'h':
		case 'L':
		case 'S':
		case 'T':
		case 'V':
			/* Processed above */
			break;
		case ':':
			usage = 1;
			DERROR("Option '%s' requires an argument", opts[longindex].name)
			break;
		case '?':
			usage = 1;
			DERROR("Option starting with '%c' not recognized\n", gnuoptopt)
			break;
		default:
			/* check current mod first */
			mod = NULL;
			if (current_e->mod->optchar == optc) {
				mod = current_e->mod;
				e = current_e;
			} else {
				for (e = first; e != NULL; e = e->next) {
					if (e->mod->optchar == optc) {
						mod = e->mod;
						break;
					}
				}
			}
			if (mod) {
				/* Random option arguments */
				if (gnuoptarg != NULL && strcmp(gnuoptarg,"r") == 0) {
					/* need a 32 bit number, but random() is signed and
					   nonnegative so only 31bits - we simply repeat one */
					unsigned long r = (unsigned long) random() << 1;
					r += (r & 0x00000040) >> 6;
					sprintf(rbuff, "%lu", r);
					gnuoptarg = rbuff;
				}

				if (!mod->do_opt(opts[longindex].name, gnuoptarg, e->pack)) {
					usage = 1;
				}
			}
			break;
		}
	}

	/* gnuoptind is the first thing that is not an option - should have exactly
	   one hostname... */
	if (usage == 0) {
		if (argc != gnuoptind + 1) {
 			usage = 1;
			if (argc - gnuoptind < 1) {
				ERROR("No hostname specified")
			} else {
				ERROR("More than one hostname specified")
			}
		} else if (first && first->mod->set_addr) {
			first->mod->set_addr(argv[gnuoptind], first->pack);
		}
	}

	/* free opts now we have finished with it */
	for (i = 0; i <= num_opts; i++) {
		if (opts[i].name != NULL)
			/* little trick to workaround const compiler warning */
			free((void *) (unsigned long) opts[i].name);
	}
	free(opts); /* don't need them any more */

	if (usage) {
		print_usage();
		unload_mods(TRUE, verbosity);
		if (datafile != -1) {
			munmap(data, datalen);
			close(datafile);
			datafile = -1;
		}
		if (datarg)
			free(data);
		free(sockopts);
		return 0;
	}


	/* EVIL EVIL EVIL! */
	/* Stick all the bits together - we allow finalize to shrink, but
	 * not expand the packet size and not to change the data location.
	 * Of course, any finalize which does so is responsible for pulling back
	 * all the later packet data into the area that will be sent.
	 *
	 * All of this is to accommodate esp, which needs to put its trailer after
	 * the packet data, with some padding for alignment. Since esp can't know
	 * how much padding will be needed until the rest of the packet is filled
	 * out, it preallocates an excess of padding first, and then trims in
	 * finalize to the amount actually needed.
	 */
	packet.data = NULL;
	packet.alloc_len = 0;
	packet.modified = 0;
	for (e = first; e != NULL; e = e->next) {
		packet.alloc_len += e->pack->alloc_len;
	}
	if (data != NULL)
		packet.alloc_len += datalen;
	packet.data = malloc(packet.alloc_len);
	for (i = 0, e = first; e != NULL; e = e->next) {
		memcpy((char *) packet.data + i, e->pack->data, e->pack->alloc_len);
		free(e->pack->data);
		e->pack->data = (char *)packet.data + i;
		i += e->pack->alloc_len;
	}

	/* Add any data */
	if (data != NULL)
		memcpy((char *) packet.data + i, data, datalen);

	/* Finalize from inside out */
	{
		char hdrs[num_modules];
		sendip_data *headers[num_modules];
		sendip_data d;

		d.alloc_len = datalen;
		d.data = (char *) packet.data+packet.alloc_len - datalen;

		for (i = 0, e = first; e != NULL; e = e->next, i++) {
			hdrs[i] = e->mod->optchar;
			headers[i] = e->pack;
		}

		for (i = num_modules - 1, e = last; e != NULL; e = e->prev, i--) {
			if (verbosity)
				printf("Finalizing module %s\n", e->mod->name);
			/* Remove this header from enclosing list but don't erase the
			 * header type, so that it's available to upper-level headers where
			 * needed. Instead, we tell the upper-level headers where they are
			 * in the list.
			 * wesp needs to see the esp header info, so we can't erase that,
			 * either. */
			/* headers[i] = NULL; */
			e->mod->finalize(hdrs, headers, i, &d, e->pack);

			/* Get everything ready for the next call */
			d.data = (char *) d.data - e->pack->alloc_len;
			d.alloc_len += e->pack->alloc_len;
		}
		/* Trim back the packet length if need be */
		if (d.alloc_len < packet.alloc_len)
			packet.alloc_len = d.alloc_len;
	}
	/* We could (and should?) free any leftover priv data here. */

	/* And send the packet */
	{
		int af_type;
		if (first == NULL) {
			if (data == NULL) {
				ERROR("Nothing specified to send!")
				print_usage();
				free(packet.data);
				unload_mods(FALSE, verbosity);
				free(sockopts);
				return 1;
			} else {
				af_type = AF_INET;
			}
		}
		else if (first->mod->optchar == 'i')
			af_type = AF_INET;
		else if (first->mod->optchar == '6')
			af_type = AF_INET6;
		else {
			ERROR("Either IPv4 or IPv6 must be the outermost packet")
			free(packet.data);
			unload_mods(FALSE, verbosity);
			free(sockopts);
			return 1;
		}
		i = sendpacket(&packet, argv[gnuoptind], af_type, verbosity, dump,
			sockopts);
		free(packet.data);
	}
	/* Regenerate data on subsequent loop calls */
	if (loopcount && datarg) {
		char sdata[BUFSIZ];
		int newlen;

		newlen = opt2val(sdata, datarg, BUFSIZ);
		if (newlen > datalen) {
			free(data);
			if ((data = (char *) malloc(datalen)) == NULL) {
				PERROR("Unable to allocate data memory")
				break;
			}
		}
		datalen = newlen;
		memcpy(data, sdata, datalen);
	}
	if (loopcount && delaytime)
		sleep(delaytime);
} /* end of loop */

	unload_mods(FALSE, verbosity);
	if (datafile != -1) {
		munmap(data, datalen);
		close(datafile);
		datafile = -1;
	}
	if (datarg)
		free(data);
	free(sockopts);

	return 0;
}

/* vim: ts=4 sw=4 filetype=c
 */
