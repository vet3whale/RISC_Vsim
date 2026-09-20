#include <stdio.h>
#include <stdint.h>
#include "riscv/decoder.h"

using namespace std;

int decodeAddi(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    // optcode is in bits 0 to 6
    // word &= 0b1111111
    unsigned int optcode = word & 0x7F;

    // funct3 is in bits 12 to 14
    // shift right by 12, then keep lowest 3 bits
    unsigned int funct3 = (word >> 12) & 0x7;

    // ADDI has optcode 0x13, and funct3 is 0
    if (optcode != 0x13 || funct3 != 0) return 0;

    // rd is the destination register, in bits 7 to 11
    // shift right by 7, and find lowest 5 bits
    unsigned int rd = (word >> 7) & 0x1F;

    // rs1 is source register, in bits 15 to 19
    unsigned int rs1 = (word >> 15) & 0x1F;

    // immediate occupies bits 20 to 31
    int immediate = (word >> 20) & 0xFFF;

    // since immediate is a signed 12 bit number
    if (immediate >= 2048) immediate -= 4096;

    int length = snprintf(
        output,bufferSize,
        "ADDI x%u, x%u, %d",
        rd, rs1, immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}