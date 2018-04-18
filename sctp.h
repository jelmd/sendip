/** sctp.h - stream control transmission protocol
 *
 * stream control transmission protocol (RFC 4960)
 *
 * Many of the structures and definitions here are taken/adapted from
 * the Linux kernel sctp implementation. Of course, this implementation
 * uses entirely different code (other than the crc32 calculation).
 */
#ifndef _SENDIP_SCTP_H
#define _SENDIP_SCTP_H

#include <asm/byteorder.h>

/* Overall header */
typedef struct sctp {
	u_int16_t	source;
	u_int16_t	dest;
	u_int32_t	vtag;
	u_int32_t	checksum;
} sctp_header;

/* Chunk header */
typedef struct sctp_chunk {
	u_int8_t type;
	u_int8_t flags;
	u_int16_t length;
} sctp_chunk_header;

/* Chunk types */
typedef enum {
	SCTP_CID_DATA				= 0,
	SCTP_CID_INIT				= 1,
	SCTP_CID_INIT_ACK			= 2,
	SCTP_CID_SACK				= 3,
	SCTP_CID_HEARTBEAT			= 4,
	SCTP_CID_HEARTBEAT_ACK		= 5,
	SCTP_CID_ABORT				= 6,
	SCTP_CID_SHUTDOWN			= 7,
	SCTP_CID_SHUTDOWN_ACK		= 8,
	SCTP_CID_ERROR				= 9,
	SCTP_CID_COOKIE_ECHO		= 10,
	SCTP_CID_COOKIE_ACK			= 11,
	SCTP_CID_ECN_ECNE			= 12,
	SCTP_CID_ECN_CWR			= 13,
	SCTP_CID_SHUTDOWN_COMPLETE	= 14,

	/* AUTH Extension Section 4.1 */
	SCTP_CID_AUTH				= 0x0F,

	/* PR-SCTP Sec 3.2 */
	SCTP_CID_FWD_TSN			= 0xC0,

	/* Use hex, as defined in ADDIP sec. 3.1 */
	SCTP_CID_ASCONF				= 0xC1,
	SCTP_CID_ASCONF_ACK			= 0x80,
} sctp_cid_t; /* enum */

/* Section 3.2
 * Chunk Types are encoded such that the highest-order two bits specify
 * the action that must be taken if the processing endpoint does not
 * recognize the Chunk Type.
 */
typedef enum {
	SCTP_CID_ACTION_DISCARD		= 0x00,
	SCTP_CID_ACTION_DISCARD_ERR	= 0x40,
	SCTP_CID_ACTION_SKIP		= 0x80,
	SCTP_CID_ACTION_SKIP_ERR	= 0xc0,
} sctp_cid_action_t;

enum { SCTP_CID_ACTION_MASK = 0xc0, };

/* Init header */
typedef struct sctp_inithdr {
	u_int32_t init_tag;
	u_int32_t a_rwnd; /* advertised receiver window credit */
	u_int16_t num_outbound_streams;
	u_int16_t num_inbound_streams;
	u_int32_t initial_tsn;
	u_int8_t  params[];
} sctp_inithdr_t;

/* Data header */
typedef struct sctp_datahdr {
	u_int32_t tsn;
	u_int16_t stream;
	u_int16_t ssn;
	u_int32_t ppid;
	u_int8_t  payload[];
} sctp_datahdr_t;

/* TLV parameter types */
typedef enum {
	/* RFC 2960 Section 3.3.5 */
	SCTP_PARAM_HEARTBEAT_INFO			= 1,
	/* RFC 2960 Section 3.3.2.1 */
	SCTP_PARAM_IPV4_ADDRESS				= 5,
	SCTP_PARAM_IPV6_ADDRESS				= 6,
	SCTP_PARAM_STATE_COOKIE				= 7,
	SCTP_PARAM_UNRECOGNIZED_PARAMETERS	= 8,
	SCTP_PARAM_COOKIE_PRESERVATIVE		= 9,
	SCTP_PARAM_HOST_NAME_ADDRESS		= 11,
	SCTP_PARAM_SUPPORTED_ADDRESS_TYPES	= 12,
	SCTP_PARAM_ECN_CAPABLE				= 0x8000,

	/* AUTH Extension Section 3 */
	SCTP_PARAM_RANDOM					= 0x8002,
	SCTP_PARAM_CHUNKS					= 0x8003,
	SCTP_PARAM_HMAC_ALGO				= 0x8004,

	/* Add-IP: Supported Extensions, Section 4.2 */
	SCTP_PARAM_SUPPORTED_EXT			= 0x8008,

	/* PR-SCTP Sec 3.1 */
	SCTP_PARAM_FWD_TSN_SUPPORT			= 0xc000,

	/* Add-IP Extension. Section 3.2 */
	SCTP_PARAM_ADD_IP					= 0xc001,
	SCTP_PARAM_DEL_IP					= 0xc002,
	SCTP_PARAM_ERR_CAUSE				= 0xc003,
	SCTP_PARAM_SET_PRIMARY				= 0xc004,
	SCTP_PARAM_SUCCESS_REPORT			= 0xc005,
	SCTP_PARAM_ADAPTATION_LAYER_IND		= 0xc006,

} sctp_param_t; /* enum */

/* The generic Linux structure just includes the type and length.
 * Data fields are in the type-specific structures.
 */
typedef struct sctp_paramhdr {
	u_int16_t	type;
	u_int16_t	length;
} sctp_paramhdr_t;

/* Individual TLV parameter types. Note that only a handful of these
 * are currently implemented in sendip!
 */

/* Section 3.3.2.1. IPv4 Address Parameter (5) */
typedef struct sctp_ipv4addr_param {
	sctp_paramhdr_t param_hdr;
	struct in_addr addr;
} sctp_ipv4addr_param_t;

/* Section 3.3.2.1. IPv6 Address Parameter (6) */
typedef struct sctp_ipv6addr_param {
	sctp_paramhdr_t param_hdr;
	struct in6_addr addr;
} sctp_ipv6addr_param_t;

/* Section 3.3.2.1 Cookie Preservative (9) */
typedef struct sctp_cookie_preserve_param {
	sctp_paramhdr_t param_hdr;
	u_int32_t lifespan_increment;
} sctp_cookie_preserve_param_t;

/* Section 3.3.2.1 Host Name Address (11) */
typedef struct sctp_hostname_param {
	sctp_paramhdr_t param_hdr;
	uint8_t hostname[];
} sctp_hostname_param_t;

/* Section 3.3.2.1 Supported Address Types (12) */
typedef struct sctp_supported_addrs_param {
	sctp_paramhdr_t param_hdr;
	u_int16_t types[];
} sctp_supported_addrs_param_t;

/* Appendix A. ECN Capable (32768) */
typedef struct sctp_ecn_capable_param {
	sctp_paramhdr_t param_hdr;
} sctp_ecn_capable_param_t;

/* ADDIP Section 3.2.6 Adaptation Layer Indication */
typedef struct sctp_adaptation_ind_param {
	struct sctp_paramhdr param_hdr;
	u_int32_t adaptation_ind;
} sctp_adaptation_ind_param_t;

/* ADDIP Section 4.2.7 Supported Extensions Parameter */
typedef struct sctp_supported_ext_param {
	struct sctp_paramhdr param_hdr;
	u_int8_t chunks[];
} sctp_supported_ext_param_t;

/* AUTH Section 3.1 Random */
typedef struct sctp_random_param {
	sctp_paramhdr_t param_hdr;
	u_int8_t random_val[];
} sctp_random_param_t;

/* AUTH Section 3.2 Chunk List */
typedef struct sctp_chunks_param {
	sctp_paramhdr_t param_hdr;
	u_int8_t chunks[];
} sctp_chunks_param_t;


/* AUTH Section 3.3 HMAC Algorithm */
typedef struct sctp_hmac_algo_param {
	sctp_paramhdr_t param_hdr;
	u_int16_t hmac_ids[];
} sctp_hmac_algo_param_t;

/* RFC 2960.  Section 3.3.3 Initiation Acknowledgement (INIT ACK) (2):
 *   The INIT ACK chunk is used to acknowledge the initiation of an SCTP
 *   association.
 */

/* Section 3.3.3.1 State Cookie (7) */
typedef struct sctp_cookie_param {
	sctp_paramhdr_t p;
	u_int8_t body[];
} sctp_cookie_param_t;


/* Section 3.3.3.1 Unrecognized Parameters (8) */
typedef struct sctp_unrecognized_param {
	sctp_paramhdr_t param_hdr;
	sctp_paramhdr_t unrecognized;
} sctp_unrecognized_param_t;


/* Forward TSN supported (0xC000, 49152) - mere presence seems to imply
 * capability, just like with ECN, so there are no other fields.
 */
typedef struct sctp_forward_tsn_param {
	sctp_paramhdr_t param_hdr;
} sctp_forward_tsn_param_t;


#define SCTP_MOD_SOURCE		(1)
#define SCTP_MOD_DEST		(1<<1)
#define SCTP_MOD_VTAG		(1<<2)
#define SCTP_MOD_CHECKSUM	(1<<3)
/* Don't bother noting setting of chunk fields with flags */

/* Options */
sendip_option sctp_opts[] = {
	{ "s", 1, "Source port", "0" },
	{ "d", 1, "Destination port", "0" },
	{ "v", 1, "Verification tag", "0 if init chunk, 1 otherwise" },
	{ "c", 1, "Checksum", "Correct" },
	{ "T", 1, "Chunk type", "0" },
	{ "F", 1, "Chunk flags", "0" },
	{ "L", 1, "Chunk length", "Correct" },
	{ "D", 1, "Append data to chunk", NULL },
	{ "I", 1, "Add INIT chunk (tag:rwnd:nout:nin:tsn)", "1|1000|1|1|1" },
	{ "4", 1, "IPv4 endpoint address parameter", NULL },
	{ "6", 1, "IPv6 endpoint address parameter", NULL },
	{ "C", 1, "Cookie life-span increment parameter", NULL },
	{ "H", 1, "Hostname parameter", NULL },
	{ "A", 1, "Supported address types parameter", NULL },
	{ "E", 0, "Add ECN capable parameter", NULL },
	{ "W", 0, "Add forward TSN supported parameter", NULL },
	{ "Y", 1, "Adaptation layer indication parameter", NULL },
};

#endif  /* _SENDIP_SCTP_H */

/* vim: ts=4 sw=4 filetype=c
 */
