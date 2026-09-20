#include <stdio.h>
#include <stdint.h>
#include "riscv/decoder.h"

using namespace std;

struct STypeFields {
    unsigned int optcode;
    unsigned int funct3;
    unsigned int rs1;
    unsigned int rs2;
    int immediate;
};

static struct STypeFields extractSTypeFields(uint32_t word) {
    struct STypeFields fields;

    // optcode is in bits 0 to 6
    // word &= 0b1111111
    fields.optcode = word & 0x7F;

    // funct3 is in bits 12 to 14
    // shift right by 12, then keep lowest 3 bits
    fields.funct3 = (word >> 12) & 0x7;

    // rs1 is source register, in bits 15 to 19
    fields.rs1 = (word >> 15) & 0x1F;

    // rs2 is source register, in bits 20 to 24
    fields.rs2 = (word >> 20) & 0x1F;

    // immediate bits 0 to 4 are in bits 7 to 11
    // immediate bits 5 to 11 are in bits 25 to 31
    fields.immediate = ((word >> 7) & 0x1F) |
                       (((word >> 25) & 0x7F) << 5);

    // since immediate is a signed 12 bit number
    if (fields.immediate >= 2048) fields.immediate -= 4096;

    return fields;
}

int decodeSw(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct STypeFields fields = extractSTypeFields(word);

    // SW has optcode 0x23, and funct3 is 2
    if (fields.optcode != 0x23 || fields.funct3 != 2) return 0;

    int length = snprintf(
        output,bufferSize,
        "SW x%u, %d(x%u)",
        fields.rs2, fields.immediate, fields.rs1
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}
