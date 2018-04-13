/** headers.c */

#include <string.h>

#include "headers.h"

struct sendip_headers sendip_headers[] = {
	{'H', IPPROTO_HOPOPTS},		/* h already taken */
	{'F', IPPROTO_FRAGMENT},	/* f already taken */
	{'g', IPPROTO_GRE},
	{'e', IPPROTO_ESP},			/* TBD */
	{'a', IPPROTO_AH},
	{'c', IPPROTO_ICMPV6},
	{'t', IPPROTO_TCP},
	{'u', IPPROTO_UDP},
	{'i', IPPROTO_IPIP},		/* for 4-in-4 tunnels */
	{'6', IPPROTO_IPV6},		/* for 6-in-4 tunnels */
	{'d', IPPROTO_DSTOPTS},
	{'o', IPPROTO_ROUTING},		/* sorry, r and R already taken */
	{'s', IPPROTO_SCTP},
	{'w', IPPROTO_WESP},
	{'\0', IPPROTO_NONE},		/* stop here at opt char '\0' */

	/* These are placeholders */
	{'n', IPPROTO_NONE},		/* ntp */
	{'r', IPPROTO_NONE},		/* rip */
	{'R', IPPROTO_NONE},		/* ripng */
	/* These are base flags and can't be used for headers:
	 * D d f h L p S T V v
	 */
};

u_int8_t
header_type(const char hdr_char)
{
	int i;

	for (i = 0; sendip_headers[i].opt_char; ++i)
		if (hdr_char == sendip_headers[i].opt_char)
			return sendip_headers[i].ipproto;
	return IPPROTO_NONE;
}

int
outer_header(const char *hdrs, int index, const char *choices)
{
	int i;

	for (i = index - 1; i >= 0; --i) {
		if (strchr(choices, hdrs[i]))
			return i;
	}
	return -1;
}

int
inner_header(const char *hdrs, int index, const char *choices)
{
	int i;

	for (i = index + 1; hdrs[i]; ++i) {
		if (strchr(choices, hdrs[i]))
			return i;
	}
	return -1;
}

/* vim: ts=4 sw=4 filetype=c
 */
