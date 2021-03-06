/** gre.c - Generic Routing Encapsulation
 * By Mark Carson
 */

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <asm/byteorder.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "sendip_module.h"
#include "common.h"

#include "ipv6ext.h"
#include "gre.h"

/* Character that identifies our options */
const char opt_char = 'g';

/* Our initial allocation only includes the non-optional fields. Others
 * are added in response to the appropriate options.
 */
sendip_data *
initialize(void)
{
	sendip_data *ret = malloc(sizeof(sendip_data));
	gre_header *gre = malloc(sizeof(gre_header));
	memset(gre, 0, sizeof(gre_header));
	ret->alloc_len = sizeof(gre_header);
	ret->data = gre;
	ret->modified = 0;
	return ret;
}

/* There's an unfortunate collision between the way GRE lays out
 * fields and the way sendip handles space allocation. Since sendip
 * gives no space allocation other than the actual space in the
 * packet (and a single word of "flags"), everything must be
 * more or less in place at the end of processing each option,
 * as there is no way of telling whether this is the last option
 * or not. But the layout of a GRE header depends on which
 * options are included. Hence, we have to juggle fields each
 * time.
 *
 * I could simplify all this by adding a provision for auxiliary
 * allocated space in sendip. During option processing, all the
 * optional fields would be stuck into the auxiliary space; only
 * in finalize would they be laid out into the packet. This is
 * how the Linux kernel handles GRE, as an example.
 *
 * I may still do that, but for now I will try doing everything
 * in place. Wish me luck!
 */
int
gre_where(u_int16_t flags, u_int16_t addflag)
{
	int where;

	/* Let's be as stupidly explicit as possible. Order of options:
	 *
	 * 0: checksum : offset
	 * 1: key
	 * 2: sequence
	 * 3: routing
	 */
	if (addflag == htons(GRE_CSUM)) {
		where = 0;
	} else if (addflag == htons(GRE_KEY)) {
		where = (flags & htons(GRE_CSUM)) ? 1 : 0;
	} else if (addflag == htons(GRE_SEQ)) {
		where = (flags & htons(GRE_CSUM)) ? 1 : 0;
		if (flags & htons(GRE_KEY))
			++where;
	} else if (addflag == htons(GRE_ROUTING)) {
		/* Routing always requires checksum/offset field to be allocated */
		/*if (flags & GRE_CSUM)*/
		where = (flags & htons(GRE_KEY)) ? 2 : 1;
		if (flags & htons(GRE_SEQ))
			++where;
	} else {
		where = 0;
	}
	return where;
}

gre_header *
gre_resize(sendip_data *pack, u_int16_t flags, u_int16_t addflag)
{
	u_int16_t size;
	int ourspot, endspot;
	u_int16_t newflags = (flags | addflag);
	gre_header *gre = (gre_header *) pack->data;

	/* First, check whether we already have one of these */
	if (newflags == flags)
		return gre;

	/* Oy, is this stupid or what? */
	size = sizeof(gre_header);
	if (newflags & htons(GRE_CSUM))
		size += sizeof(u_int32_t);
	if (newflags & htons(GRE_KEY))
		size += sizeof(u_int32_t);
	if (newflags & htons(GRE_SEQ))
		size += sizeof(u_int32_t);
	if (newflags & htons(GRE_ROUTING)) {
		size += sizeof(u_int32_t);
		/* Routing requires the space for checksum/offset
		 * to be allocated as well. We'll just make a
		 * recursive call to handle it.
		 */
		if (!(newflags & htons(GRE_CSUM))) {
			gre = gre_resize(pack, flags, htons(GRE_CSUM));
			size += sizeof(u_int32_t);
		}
	}

	/* Allocate any additional space needed */
	if (pack->alloc_len >= size)
		return gre; /* ?? */

	pack->data = realloc(pack->data, size);
	pack->alloc_len = size;
	gre = (gre_header *) pack->data;

	/* Now shove any preexisting options down to allow space for
	 * our new one.
	 */
	ourspot = gre_where(flags, addflag);
	endspot = (size - sizeof(gre_header)) / sizeof(u_int32_t);
	while (--endspot > ourspot) {
		gre->gre_info.thirtytwo[endspot] = gre->gre_info.thirtytwo[endspot - 1];
	}
	/* Zero out the new option just for fun */
	gre->gre_info.thirtytwo[ourspot] = 0;
	return gre;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack)
{
	gre_header *gre = (gre_header *) pack->data;
	u_int16_t svalue;

	switch (opt[1]) {
	case 'c':
		if (gre->gre_flag & htons(GRE_CSUM)) {
			ERROR("gre - already have a checksum specified")
			return FALSE;
		}
		gre = gre_resize(pack, gre->gre_flag, htons(GRE_CSUM));
		gre->gre_flag |= htons(GRE_CSUM);
		/* There's only one place for the checksum field to go! */
		gre->gre_info.sixteen[GRE_CHECKSUM_FIELD] = opt2intn(arg, NULL, 2);
		pack->modified |= GRE_MOD_CHECKSUM;
		break;
	case 'C':
		if (gre->gre_flag & htons(GRE_CSUM)) {
			ERROR("gre - already have a checksum specified")
			return FALSE;
		}
		gre = gre_resize(pack, gre->gre_flag, htons(GRE_CSUM));
		gre->gre_flag |= htons(GRE_CSUM);
		/* Actual checksum will be computed in finalize */
		/* Note we do *not* set the pack flag here, so we will
		 * know below that we need to compute a value.
		 */
		break;
	case 'r':
		gre = gre_resize(pack, gre->gre_flag, htons(GRE_ROUTING));
		gre->gre_info.thirtytwo[gre_where(gre->gre_flag, htons(GRE_ROUTING))] =
			opt2intn(arg, NULL, 4);
		gre->gre_flag |= htons(GRE_ROUTING);
		pack->modified |= GRE_MOD_ROUTING;
		break;
	case 'k':
		gre = gre_resize(pack, gre->gre_flag, htons(GRE_KEY));
		gre->gre_info.thirtytwo[gre_where(gre->gre_flag, htons(GRE_KEY))] =
			opt2intn(arg, NULL, 4);
		gre->gre_flag |= htons(GRE_KEY);
		pack->modified |= GRE_MOD_KEY;
		break;
	case 's':
		gre = gre_resize(pack, gre->gre_flag, htons(GRE_SEQ));
		gre->gre_info.thirtytwo[gre_where(gre->gre_flag, htons(GRE_SEQ))] =
			opt2intn(arg, NULL, 4);
		gre->gre_flag |= htons(GRE_SEQ);
		pack->modified |= GRE_MOD_SEQUENCE;
		break;
	case 'S':
		/* Logically, this only makes sense if routing info is supplied,
		   but of course we won't enforce this. */
		gre->gre_flag |= htons(GRE_STRICT);
		pack->modified |= GRE_MOD_STRICT;
		break;
	case 'e':
		svalue = opt2inth(arg, NULL, 2);
		if (svalue > GRE_MAX_REC) {
			DERROR("gre - recursion limit too big (%d > %d)",
				svalue, GRE_MAX_REC)
			return FALSE;
		}
		gre->gre_flag &= ~htons(GRE_REC);
		gre->gre_flag |= (htons(GRE_REC) & htons(svalue << GRE_REC_SHIFT));
		pack->modified |= GRE_MOD_RECURSION;
		break;
	case 'v':
		svalue = opt2intn(arg, NULL, 2);
		if (svalue > GRE_MAX_VERSION) {
			DERROR("gre - version number too big (%d > %d)",
				svalue, GRE_MAX_VERSION)
			return FALSE;
		}
		gre->gre_flag &= ~htons(GRE_VERSION);
		gre->gre_flag |= (htons(GRE_VERSION) & htons(svalue));
		pack->modified |= GRE_MOD_VERSION;
		break;
	case 'p':
		gre->gre_protocol = opt2intn(arg, NULL, 2);
		pack->modified |= GRE_MOD_PROTOCOL;
		break;
	case 'o':
		/* This is tricky - in order to have an offset field,
		 * we have to have a checksum field. But we don't mark
		 * the latter as set, so we can properly check setting
		 * of it later. This does mean we could end up
		 * constructing a bad header (checksum/offset fields
		 * allocated, but no flag set). Of course, the purpose
		 * of this code is to be able to construct bad headers.
		 */
		gre = gre_resize(pack, gre->gre_flag, htons(GRE_CSUM));
		/* There's only one place for the offset field to go! */
		gre->gre_info.sixteen[GRE_OFFSET_FIELD] = htonl(opt2inth(arg, NULL, 2));
		pack->modified |= GRE_MOD_OFFSET;
		break;
	}
	return TRUE;
}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[], int index,
	sendip_data *data, sendip_data *pack)
{
	gre_header *gre = (gre_header *) pack->data;

	/* Note that GRE uses the Ethernet protocol value and not the IP one!
	   Because it's "generic," you see. */
	if (!(pack->modified & GRE_MOD_PROTOCOL)) {
		switch (hdrs[inner_header(hdrs, index, "i6")]) {
		case 'i':
			gre->gre_protocol = htons(ETH_P_IP);
			break;
		case '6':
			gre->gre_protocol = htons(ETH_P_IPV6);
			break;
		}
	}
	if ((gre->gre_flag & htons(GRE_CSUM))
		&& !(pack->modified & GRE_MOD_CHECKSUM))
	{
		/* We need to compute the checksum */
		u_int16_t *vec[3];
		int lens[3];

		vec[0] = pack->data; lens[0] = pack->alloc_len;
		vec[1] = data->data; lens[1] = data->alloc_len;
		vec[2] = NULL; lens[2] = 0;
		gre->gre_info.sixteen[GRE_CHECKSUM_FIELD] = csumv(vec, lens);
	}
	return TRUE;

}

int
num_opts() {
	return sizeof(gre_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return gre_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
