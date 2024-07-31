#ifndef CRC_H
#define CRC_H

#include <cstdint>

uint32_t crc32(uint8_t *data, int length);

uint16_t fsChecksum(uint8_t *data, int length);

#endif