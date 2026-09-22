#include <stdio.h>
#include "riscv/decoder.h"

int decodeInstruction(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    // optcode is in bits 0 to 6
    unsigned int optcode = word & 0x7F;

    switch (optcode) { // with reference to ../cstruct_images/complete_struct.png
        case 0b0010011:
            return decodeAddi(word, output, bufferSize) ||
                   decodeSlti(word, output, bufferSize);
        case 0b0000011:
            return decodeLw(word, output, bufferSize);
        case 0b1100111:
            return decodeJalr(word, output, bufferSize);
        case 0b0110011:
            return decodeAdd(word, output, bufferSize) ||
                   decodeSub(word, output, bufferSize) ||
                   decodeAnd(word, output, bufferSize) ||
                   decodeOr(word, output, bufferSize) ||
                   decodeXor(word, output, bufferSize) ||
                   decodeSll(word, output, bufferSize) ||
                   decodeSrl(word, output, bufferSize) ||
                   decodeSlt(word, output, bufferSize);
        case 0b0100011:
            return decodeSw(word, output, bufferSize);
        case 0b1100011:
            return decodeBeq(word, output, bufferSize) ||
                   decodeBne(word, output, bufferSize) ||
                   decodeBlt(word, output, bufferSize) ||
                   decodeBge(word, output, bufferSize);
        case 0b1101111:
            return decodeJal(word, output, bufferSize);
        default:
            return 0;
    }
}
