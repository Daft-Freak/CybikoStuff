#ifndef ENCODING_H
#define ENCODING_H

#include <stdbool.h>
#include <stdint.h>

void encodeBMC(uint8_t *in, uint8_t *out, int length);

bool decodeLZSS(uint8_t *in, uint8_t *out, int inLength, int outLength);

#endif
