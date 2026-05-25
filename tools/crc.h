#ifndef CRC_H
#define CRC_H

#include <stdint.h>

uint32_t crc16(uint8_t *data, int length);
uint32_t crc32(uint8_t *data, int length);

#endif
