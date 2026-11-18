#ifndef RISCV_DECODER_H
#define RISCV_DECODER_H

#include <stdint.h>
#include "riscv/instruction.h"

int decodeInstruction(uint32_t word, char *output, int bufferSize); // part 1

// function overloading done here because we want a way to fill in the Instruction struct
bool decodeInstruction(uint32_t word, uint32_t address, Instruction& result); // part 2

// following are decoder functions for i_type
int decodeAddi(uint32_t word, char *output, int bufferSize);
int decodeSlti(uint32_t word, char *output, int bufferSize);
int decodeLw(uint32_t word, char *output, int bufferSize);
int decodeJalr(uint32_t word, char *output, int bufferSize);

// following are decoder functions for r_type
int decodeAdd(uint32_t word, char *output, int bufferSize);
int decodeSub(uint32_t word, char *output, int bufferSize);
int decodeAnd(uint32_t word, char *output, int bufferSize);
int decodeOr(uint32_t word, char *output, int bufferSize);
int decodeXor(uint32_t word, char *output, int bufferSize);
int decodeSll(uint32_t word, char *output, int bufferSize);
int decodeSrl(uint32_t word, char *output, int bufferSize);
int decodeSlt(uint32_t word, char *output, int bufferSize);

// following are decoder functions for s_type
int decodeSw(uint32_t word, char *output, int bufferSize);

// following are decoder functions for b_type
int decodeBeq(uint32_t word, char *output, int bufferSize);
int decodeBne(uint32_t word, char *output, int bufferSize);
int decodeBlt(uint32_t word, char *output, int bufferSize);
int decodeBge(uint32_t word, char *output, int bufferSize);

// following are decoder functions for j_type
int decodeJal(uint32_t word, char *output, int bufferSize);

#endif
