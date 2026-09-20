#include <stdio.h>
#include "riscv/decoder.h"

int decodeInstruction(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    // optcode is in bits 0 to 6
    unsigned int optcode = word & 0x7F;

    switch (optcode) {
        case 0x13:
            return decodeAddi(word, output, bufferSize) ||
                   decodeSlti(word, output, bufferSize);
        case 0x03:
            return decodeLw(word, output, bufferSize);
        case 0x67:
            return decodeJalr(word, output, bufferSize);
        case 0x33:
            return decodeAdd(word, output, bufferSize) ||
                   decodeSub(word, output, bufferSize) ||
                   decodeAnd(word, output, bufferSize) ||
                   decodeOr(word, output, bufferSize) ||
                   decodeXor(word, output, bufferSize) ||
                   decodeSll(word, output, bufferSize) ||
                   decodeSrl(word, output, bufferSize) ||
                   decodeSlt(word, output, bufferSize);
        case 0x23:
            return decodeSw(word, output, bufferSize);
        case 0x63:
            return decodeBeq(word, output, bufferSize) ||
                   decodeBne(word, output, bufferSize) ||
                   decodeBlt(word, output, bufferSize) ||
                   decodeBge(word, output, bufferSize);
        case 0x6F:
            return decodeJal(word, output, bufferSize);
        default:
            return 0;
    }
}
