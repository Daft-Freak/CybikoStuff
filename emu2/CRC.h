#ifndef CRC_H
#define CRC_H

#include <cstdint>

void initCRCTables();

uint32_t crc32(uint8_t *data, int length);

#endif