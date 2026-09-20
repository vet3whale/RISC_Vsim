#include <stdio.h>
#include <stdint.h>
#include "riscv/decoder.h"

using namespace std;

struct BTypeFields {
    unsigned int optcode;
    unsigned int funct3;
    unsigned int rs1;
    unsigned int rs2;
    int immediate;
};

static struct BTypeFields extractBTypeFields(uint32_t word) {
    struct BTypeFields fields;

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

    // immediate bit 0 is always 0
    // immediate bits 1 to 4 are in bits 8 to 11
    // immediate bits 5 to 10 are in bits 25 to 30
    // immediate bit 11 is in bit 7
    // immediate bit 12 is in bit 31
    fields.immediate = (((word >> 8) & 0xF) << 1) |
                       (((word >> 25) & 0x3F) << 5) |
                       (((word >> 7) & 0x1) << 11) |
                       (((word >> 31) & 0x1) << 12);

    // since immediate is a signed 13 bit number
    if (fields.immediate >= 4096) fields.immediate -= 8192;

    return fields;
}

int decodeBeq(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct BTypeFields fields = extractBTypeFields(word);

    // BEQ has optcode 0x63, and funct3 is 0
    if (fields.optcode != 0x63 || fields.funct3 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "BEQ x%u, x%u, %d",
        fields.rs1, fields.rs2, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeBne(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct BTypeFields fields = extractBTypeFields(word);

    if (fields.optcode != 0x63 || fields.funct3 != 1) return 0;

    int length = snprintf(
        output,bufferSize,
        "BNE x%u, x%u, %d",
        fields.rs1, fields.rs2, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeBlt(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct BTypeFields fields = extractBTypeFields(word);

    if (fields.optcode != 0x63 || fields.funct3 != 4) return 0;

    int length = snprintf(
        output,bufferSize,
        "BLT x%u, x%u, %d",
        fields.rs1, fields.rs2, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeBge(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct BTypeFields fields = extractBTypeFields(word);

    if (fields.optcode != 0x63 || fields.funct3 != 5) return 0;

    int length = snprintf(
        output,bufferSize,
        "BGE x%u, x%u, %d",
        fields.rs1, fields.rs2, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}
