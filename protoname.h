#ifndef _PROTONAME_H
#define _PROTONAME_H

const char * proto_to_name(u_int8_t proto, int nolookup);
u_int8_t name_to_proto(const char *s);

#endif  /* _PROTONAME_H */
