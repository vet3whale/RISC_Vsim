#ifndef RISCV_DECODER_H
#define RISCV_DECODER_H

#include <stdint.h>

// following are decoder functions for i_type
int decodeAddi(uint32_t word, char *output, int bufferSize);

#endif