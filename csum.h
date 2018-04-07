#ifndef _CSUM_H
#define _CSUM_H

#include "types.h"

u_int16_t csum (u_int16_t *packet, int packlen);
u_int16_t csumv(u_int16_t *packet[], int packlen[]);

#endif /* _CSUM_H */
