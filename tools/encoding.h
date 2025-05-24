#ifndef ENCODING_H
#define ENCODING_H

#include <stdbool.h>
#include <stdint.h>

void encodeBoot(uint8_t *in, uint8_t *out, int length);

bool decodeLZSS(uint8_t *in, uint8_t *out, int inLength, int outLength);
int encodeLZSS(uint8_t *in, uint8_t *out, int inLength);

// used by apps and the boot logo
bool decodeBMC(uint8_t *in, uint8_t *out, int inLength, int outLength);
int encodeBMC(uint8_t *in, uint8_t *out, int inLength, int maxOutLength);

#endif
