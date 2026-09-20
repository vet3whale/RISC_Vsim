#include <stdio.h>
#include <stdint.h>
#include "riscv/decoder.h"

using namespace std;

struct JTypeFields {
    unsigned int optcode;
    unsigned int rd;
    int immediate;
};

static struct JTypeFields extractJTypeFields(uint32_t word) {
    struct JTypeFields fields;

    // optcode is in bits 0 to 6
    // word &= 0b1111111
    fields.optcode = word & 0x7F;

    // rd is the destination register, in bits 7 to 11
    // shift right by 7, and find lowest 5 bits
    fields.rd = (word >> 7) & 0x1F;

    // immediate bit 0 is always 0
    // immediate bits 1 to 10 are in bits 21 to 30
    // immediate bit 11 is in bit 20
    // immediate bits 12 to 19 are in bits 12 to 19
    // immediate bit 20 is in bit 31
    fields.immediate = (((word >> 21) & 0x3FF) << 1) |
                       (((word >> 20) & 0x1) << 11) |
                       (word & 0xFF000) |
                       (((word >> 31) & 0x1) << 20);

    // since immediate is a signed 21 bit number
    if (fields.immediate >= 1048576) fields.immediate -= 2097152;

    return fields;
}

int decodeJal(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct JTypeFields fields = extractJTypeFields(word);

    // JAL has optcode 0x6F
    if (fields.optcode != 0x6F) return 0;

    int length = snprintf(
        output,bufferSize,
        "JAL x%u, %d",
        fields.rd, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}
