#ifndef CHECHSUM_H
#define CHECHSUM_H

#include <stdint.h>

uint32_t net_checksum_add(int len, uint8_t *buf);
uint16_t net_checksum_finish(uint32_t sum);

/* Returns inet checksum.
 * Requires `buf` to be a multiple of 2 bytes to calculate valid checksum.
 * Therefore you may append `npad` padding bytes to `buf`. */
uint16_t net_checksum_tcpudp(uint16_t length, uint16_t npad, uint16_t proto,
                             void *src, void *dst, void *buf);
                             // uint8_t *src, uint8_t *dst, uint8_t *buf);

#endif // !CHECHSUM_H
