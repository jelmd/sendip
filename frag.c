/** frag.c - (IPv6) fragment header
 *
 * TBD - create a version that works for IPv4 as well.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "sendip_module.h"
#include "common.h"

#include "ipv6ext.h"
#include "frag.h"

/* Character that identifies our options */
const char opt_char = 'F';

sendip_data *
initialize(void)
{
	sendip_data *ret = malloc(sizeof(sendip_data));
	frag_header *frag = malloc(sizeof(frag_header));

	memset(frag,0,sizeof(frag_header));
	ret->alloc_len = sizeof(frag_header);
	ret->data = frag;
	ret->modified = 0;
	return ret;
}

/* The fragment offset and flags are packed into the same 16 bits.
 * This code handles the packing and extraction. For the sake of
 * clarity, we switch from net to host order and back.
 */
void
frag_setoffset(frag_header *frag, u_int16_t offset)
{
	if (!frag)
		return;

	/* Switch to host order first for clarity */
	frag->frag_off = ntohs(frag->frag_off);
	/* Clear the old offset and set the new */
	frag->frag_off &= IPV6_FRAG_FLAGS;
	frag->frag_off |= (offset << IPV6_FRAG_OFFSHIFT);
	/* Now switch back to net order */
	frag->frag_off = htons(frag->frag_off);
}

u_int16_t
frag_getoffset(frag_header *frag)
{
	return frag
		? (ntohs(frag->frag_off) & ~IPV6_FRAG_FLAGS) >> IPV6_FRAG_OFFSHIFT
		: 0;
}

void
frag_setflags(frag_header *frag, u_int16_t flags)
{
	if (!frag)
		return;

	/* Switch to host order first for clarity */
	frag->frag_off = ntohs(frag->frag_off);
	/* Clear the old flags and set the new */
	frag->frag_off &= ~IPV6_FRAG_FLAGS;
	frag->frag_off |= flags;
	/* Now switch back to net order */
	frag->frag_off = htons(frag->frag_off);
}

u_int16_t
frag_getflags(frag_header *frag)
{
	return frag ? ntohs(frag->frag_off) & IPV6_FRAG_FLAGS : 0;
}

bool
do_opt(const char *opt, const char *arg, sendip_data *pack)
{
	frag_header *frag = (frag_header *) pack->data;
	u_int16_t svalue;

	switch (opt[1]) {
	case 'n':	/* Fragment next header */
		frag->nexthdr = name_to_proto(arg);
		pack->modified |= FRAG_MOD_NEXTHDR;
		break;
	case 'r':	/* Fragment reserved (1 byte) */
		pack->modified |= FRAG_MOD_RESV;
		svalue = strtoul(arg, NULL, 0);

		if (svalue > OCTET_MAX) {
			DERROR("frag - resv value too big (%d > %d)", svalue, OCTET_MAX)
			return FALSE;
		}
		frag->reserved = svalue;
		break;
	case 'o':	/* Fragment offset */
		pack->modified |= FRAG_MOD_OFFSET;
		svalue = strtoul(arg, NULL, 0);

		if (svalue > IPV6_FRAG_MAXOFFSET) {
			DERROR("frag -  offset value too big (%d > %d)",
				svalue, IPV6_FRAG_MAXOFFSET)
			return FALSE;
		}
		frag_setoffset(frag, svalue);
		break;
	case 'f':	/* Fragment flags (more fragments+2 reserved) */
		pack->modified |= FRAG_MOD_FLAGS;
		svalue = strtoul(arg, NULL, 0);
		if (svalue > IPV6_FRAG_FLAGS) {
			ERROR("frag - only three flags can be set, hotshot")
			return FALSE;
		}
		frag_setflags(frag, svalue);
		break;
	case 'i':	/* Fragment identification */
		pack->modified |= FRAG_MOD_ID;
		frag->identification = htonl(strtoul(arg, NULL, 0));
		break;
	}
	return TRUE;

}

bool
finalize(char *hdrs, __attribute__((unused)) sendip_data *headers[], int index,
	__attribute__((unused)) sendip_data *data, sendip_data *pack)
{
	frag_header *frag = (frag_header *) pack->data;

	if (!(pack->modified & FRAG_MOD_NEXTHDR))
		frag->nexthdr = header_type(hdrs[index + 1]);
	return TRUE;
}

int
num_opts() {
	return sizeof(frag_opts) / sizeof(sendip_option);
}

sendip_option *
get_opts() {
	return frag_opts;
}

char
get_optchar() {
	return opt_char;
}

/* vim: ts=4 sw=4 filetype=c
 */
