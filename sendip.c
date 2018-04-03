/* sendip.c - main program code for sendip
 * Copyright 2001 Mike Ricketts <mike@earth.li>
 * Distributed under the GPL.  See LICENSE.
 * Bug reports, patches, comments etc to mike@earth.li
 * ChangeLog since 2.0 release:
 * 27/11/2001 compact_string() moved to compact.c
 * 27/11/2001 change search path for libs to include <foo>.so
 * 23/01/2002 make random fields more random (Bryan Croft <bryan@gurulabs.com>)
 * 10/08/2002 detect attempt to use multiple -d and -f options
 * ChangeLog since 2.2 release:
 * 24/11/2002 compile on archs requiring alignment
 * ChangeLog since 2.3 release:
 * 21/04/2003 random data (Anand (Andy) Rao <andyrao@nortelnetworks.com>)
 * ChangeLog since 2.4 release:
 * 21/04/2003 fix errors detected by valgrind
 * 28/07/2003 fix compile error on solaris
 */

#define _SENDIP_MAIN

/* socket stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/* everything else */
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h> /* isprint */

#include "sendip_module.h"
#include "crypto_module.h"
#include "modload.h"

#ifdef __sun  /* for EVILNESS workaround */
#include "ipv4.h"
#endif /* __sun */

/* Use our own getopt to ensure consistent behaviour on all platforms */
#include "gnugetopt.h"

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

static int num_opts=0;

static char *progname;

static int sendpacket(sendip_data *data, char *hostname, int af_type,
							 bool verbose) {
	_sockaddr_storage *to = malloc(sizeof(_sockaddr_storage));
	int tolen;

	/* socket stuff */
	int s;                            /* socket for sending       */

	/* hostname stuff */
	struct hostent *host = NULL;      /* result of gethostbyname2 */

	/* casts for specific protocols */
	struct sockaddr_in *to4 = (struct sockaddr_in *)to; /* IPv4 */
	struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)to; /* IPv6 */

	int sent;                         /* number of bytes sent */

	if(to==NULL) {
		perror("OUT OF MEMORY!\n");
		return -3;
	}
	memset(to, 0, sizeof(_sockaddr_storage));

	if ((host = gethostbyname2(hostname, af_type)) == NULL) {
		char buf[256];
		sprintf(buf, "gethostbyname2('%s', %d) failed.", hostname, af_type);
		perror(buf);
		free(to);
		return -1;
	}

	switch (af_type) {
	case AF_INET:
		to4->sin_family = host->h_addrtype;
		memcpy(&to4->sin_addr, host->h_addr, host->h_length);
		tolen = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		to6->sin6_family = host->h_addrtype;
		memcpy(&to6->sin6_addr, host->h_addr, host->h_length);
		tolen = sizeof(struct sockaddr_in6);
		break;
	default:
		return -2;
		break;
	}

	if(verbose) { 
		int i, j;  
		printf("Final packet data:\n");
		for(i=0; i<data->alloc_len; ) {
			for(j=0; j<4 && i+j<data->alloc_len; j++)
				printf("%02X ", ((unsigned char *)(data->data))[i+j]); 
			printf("  ");
			for(j=0; j<4 && i+j<data->alloc_len; j++) {
				int c=(int) ((unsigned char *)(data->data))[i+j];
				printf("%c", isprint(c)?((char *)(data->data))[i+j]:'.'); 
			}
			printf("\n");
			i+=j;
		}
	}

	if ((s = socket(af_type, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("Couldn't open RAW socket");
		free(to);
		return -1;
	}
	/* Need this for OpenBSD, shouldn't cause problems elsewhere */
	/* TODO: should make it a command line option */
	if(af_type == AF_INET) { 
		const int on=1;
		if (setsockopt(s, IPPROTO_IP,IP_HDRINCL,(const void *)&on,sizeof(on)) <0) { 
			perror ("Couldn't setsockopt IP_HDRINCL");
			free(to);
			close(s);
			return -2;
		}
	}

	/* On Solaris, it seems that the only way to send IP options or packets
		with a faked IP header length is to:
		setsockopt(IP_OPTIONS) with the IP option data and size
		decrease the total length of the packet accordingly
		I'm sure this *shouldn't* work.  But it does.
	*/
#ifdef __sun
	if((*((char *)(data->data))&0x0F) != 5) {
		ip_header *iphdr = (ip_header *)data->data;

		int optlen = iphdr->header_len*4-20;

		if(verbose) 
			printf("Solaris workaround enabled for %d IP option bytes\n", optlen);

		iphdr->tot_len = htons(ntohs(iphdr->tot_len)-optlen);

		if(setsockopt(s,IPPROTO_IP,IP_OPTIONS,
						  (void *)(((char *)(data->data))+20),optlen)) {
			perror("Couldn't setsockopt IP_OPTIONS");
			free(to);
			close(s);
			return -2;
		}
	}
#endif /* __sun */

	/* Send the packet */
	sent = sendto(s, (char *)data->data, data->alloc_len, 0, (void *)to, tolen);
	if (sent == data->alloc_len) {
		if(verbose) printf("Sent %d bytes to %s\n",sent,hostname);
	} else {
		if (sent < 0)
			perror("sendto");
		else {
			if(verbose) fprintf(stderr, "Only sent %d of %d bytes to %s\n", 
									  sent, data->alloc_len, hostname);
		}
	}
	free(to);
	close(s);
	return sent;
}

static void print_usage(void) {
	sendip_mod_li *e;
	int i;
	printf("Usage: %s [-v] [-d data] [-h] [-f datafile] "
		"[-p module] [module options] hostname\n", progname);
	printf(
"  -d data\tadd this data as a string to the end of the packet\n"
"  -f datafile\tread packet data from file\n"
"  -h\t\tprint this help message and exit\n"
"  -p module\tload the specified module (see below)\n"
"  -v\t\tbe verbose\n"
"\n\n"
"Packet data, and argument values for many header fields, may\n"
"specified as\n"
"rN to generate N random(ish) data bytes;\n"
"zN to generate N zero (nul) data bytes;\n"
"0x or 0X followed by hex digits;\n"
"0 followed by octal digits;\n"
"decimal number for decimal digits;\n"
"any other stream of bytes, taken literally.\n"
"\n\n"
"Modules are loaded in the order the -p option appears.  The headers from\n"
"each module are put immediately inside the headers from the previous module\n"
"in the final packet.  For example, to embed bgp inside tcp inside ipv4, do\n"
"sendip -p ipv4 -p tcp -p bgp ...\n"
"\n\n"
"Modules may be repeated to create multiple instances of a given header\n"
"type. For example, to create an ipip tunneled packet (ipv4 inside ipv4), do\n"
"sendip -p ipv4 <outer header arguments> -p ipv4 <inner header arguments> ...\n"
"In the case of repeated modules, arguments are applied to the closest matching\n"
"module in the command line.\n"
"\n\n"
"Modules available at compile time:\n"
"    ipv4 ipv6 icmp tcp udp bgp rip ripng ntp\n"
"    ah dest esp frag gre hop route sctp wesp.\n"
"\n");
	for(e = first; e != NULL; e = e->next) {
		if (e->num_opts == 0)
			continue;
		sendip_module *mod = e->mod;
		char *shortname = strrchr(mod->name, '/');

		if (!shortname)
			shortname = mod->name;
		else
			++shortname;
		printf("\n\nArguments for module %s:\n", shortname);
		for (i=0; i < e->num_opts; i++) {
			printf("   -%c%s %c\t%s\n", mod->optchar,
					  mod->opts[i].optname, mod->opts[i].arg ? 'x' : ' ',
					  mod->opts[i].description);
			if (mod->opts[i].def)
				printf("   \t\t  Default: %s\n", mod->opts[i].def);
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
}

int main(int argc, char *const argv[]) {
	int i;

	struct option *opts=NULL;
	int longindex=0;
	char rbuff[31];

	bool usage=FALSE, verbosity=FALSE;

	char *data=NULL;
	int datafile=-1;
	int datalen=0;
	bool randomflag=FALSE;

	sendip_module *mod;
	sendip_mod_li *e, *current_e;
	int optc;

	int num_modules=0;

	sendip_data packet;
	
	num_opts = 0;	
	first = last = NULL;

	progname=argv[0];

	/* magic random seed that gives 4 really random octets */
	srandom(time(NULL) ^ (getpid()+(42<<15)));

	/* First, get all the builtin options, and load the modules */
	gnuopterr=0; gnuoptind=0;
	while(gnuoptind<argc && (EOF != (optc=gnugetopt(argc,argv,"-p:vd:hf:")))) {
		switch(optc) {
		case 'p':
			if (mod = load_sendip_module(gnuoptarg, &i)) {
				e = malloc(sizeof(sendip_mod_li));
				if (e == NULL) {
					perror("Unable to process option -p ...");
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
			verbosity=TRUE;
			break;
		case 'd':
			if (data == NULL) {
				char *datarg;

				/* normal data, rN for random, zN for nul (zero) string */
				datalen = stringargument(gnuoptarg, &datarg);
				data = (char *) malloc(datalen);
				if (data == NULL) {
					perror("Unable to process option -d ...");
					return 1;
				}
				memcpy(data, datarg, datalen);
			} else {
				fprintf(stderr,"Only one -d or -f option can be given\n");
				usage = TRUE;
			}
			break;
		case 'h':
			usage=TRUE;
			break;
		case 'f':
			if(data == NULL) {
				datafile=open(gnuoptarg,O_RDONLY);
				if(datafile == -1) {
					perror("Couldn't open data file");
					fprintf(stderr,"No data will be included\n");
				} else {
					datalen = lseek(datafile,0,SEEK_END);
					if(datalen == -1) {
						perror("Error reading data file: lseek()");
						fprintf(stderr,"No data will be included\n");
						datalen=0;
					} else if(datalen == 0) {
						fprintf(stderr,"Data file is empty\nNo data will be included\n");
					} else {
						data = mmap(NULL,datalen,PROT_READ,MAP_SHARED,datafile,0);
						if(data == MAP_FAILED) {
							perror("Couldn't read data file: mmap()");
							fprintf(stderr,"No data will be included\n");
							data = NULL;
							datalen=0;
						}
					}
				}
			} else {
				fprintf(stderr,"Only one -d or -f option can be given\n");
				usage = TRUE;
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

	/* Build the getopt listings */
	opts = malloc((1+num_opts)*sizeof(struct option));
	if(opts==NULL) {
		perror("OUT OF MEMORY!\n");
		return 1;
	}
	memset(opts,'\0',(1+num_opts)*sizeof(struct option));
	i=0;
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
	gnuopterr=1;
	gnuoptind=0;
	current_e = NULL;
	while(EOF != (optc=_getopt_internal(argc,argv,"p:vd:hf:",opts,&longindex,1))) {
		switch(optc) {
		case 'p':
			current_e = (current_e) ? current_e->next : first;
			break;
		case 'v':
		case 'd':
		case 'f':
		case 'h':
			/* Processed above */
			break;
		case ':':
			usage=TRUE;
			fprintf(stderr,"Option %s requires an argument\n",
					  opts[longindex].name);
			break;
		case '?':
			usage=TRUE;
			fprintf(stderr,"Option starting %c not recognized\n",gnuoptopt);
			break;
		default:
			/* check current mod first */
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
				if(gnuoptarg != NULL && !strcmp(gnuoptarg,"r")) {
					/* need a 32 bit number, but random() is signed and
						nonnegative so only 31bits - we simply repeat one */
					unsigned long r = (unsigned long)random()<<1;
					r+=(r&0x00000040)>>6;
					sprintf(rbuff,"%lu",r);
					gnuoptarg = rbuff;
				}

				if (!mod->do_opt(opts[longindex].name, gnuoptarg, e->pack)) {
					usage=TRUE;
				}
			}
			break;
		}
	}

	/* gnuoptind is the first thing that is not an option - should have exactly
		one hostname...
	*/
	if (argc != gnuoptind+1) {
 		usage=TRUE;
		if (argc-gnuoptind < 1)
			fprintf(stderr, "No hostname specified\n");
		else
			fprintf(stderr, "More than one hostname specified\n");
	} else if (first && first->mod->set_addr) {
		first->mod->set_addr(argv[gnuoptind], first->pack);
	}

	/* free opts now we have finished with it */
	for(i=0;i<(1+num_opts);i++) {
		if(opts[i].name != NULL) free((void *)opts[i].name);
	}
	free(opts); /* don't need them any more */

	if(usage) {
		print_usage();
		unload_mods(TRUE, verbosity);
		if(datafile != -1) {
			munmap(data,datalen);
			close(datafile);
			datafile=-1;
		}
		if(randomflag) free(data);
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
	if(data != NULL) packet.alloc_len+=datalen;
	packet.data = malloc(packet.alloc_len);
	for(i = 0, e = first; e != NULL; e = e->next) {
		memcpy((char *)packet.data + i, e->pack->data, e->pack->alloc_len);
		free(e->pack->data);
		e->pack->data = (char *)packet.data+i;
		i += e->pack->alloc_len;
	}

	/* Add any data */
	if(data != NULL) memcpy((char *)packet.data+i,data,datalen);
	if(datafile != -1) {
		munmap(data,datalen);
		close(datafile);
		datafile=-1;
	}
	if(randomflag) free(data);

	/* Finalize from inside out */
	{
		char hdrs[num_modules];
		sendip_data *headers[num_modules];
		sendip_data d;

		d.alloc_len = datalen;
		d.data = (char *)packet.data+packet.alloc_len-datalen;

		for(i = 0, e = first; e != NULL; e = e->next, i++) {
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
			d.data = (char *)d.data - e->pack->alloc_len;
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
		if(first==NULL) {
			if(data == NULL) {
				fprintf(stderr,"Nothing specified to send!\n");
				print_usage();
				free(packet.data);
				unload_mods(FALSE, verbosity);
				return 1;
			} else {
				af_type = AF_INET;
			}
		}
		else if (first->mod->optchar == 'i') af_type = AF_INET;
		else if (first->mod->optchar == '6') af_type = AF_INET6;
		else {
			fprintf(stderr,"Either IPv4 or IPv6 must be the outermost packet\n");
			unload_mods(FALSE, verbosity);
			free(packet.data);
			return 1;
		}
		i = sendpacket(&packet,argv[gnuoptind],af_type,verbosity);
		free(packet.data);
	}
	unload_mods(FALSE, verbosity);

	return 0;
}
