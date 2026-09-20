#include <stdio.h>
#include <stdint.h>
#include "riscv/decoder.h"

using namespace std;

struct ITypeFields {
    unsigned int optcode;
    unsigned int funct3;
    unsigned int rd;
    unsigned int rs1;
    int immediate;
};

static struct ITypeFields extractITypeFields(uint32_t word) {
    struct ITypeFields fields;

    // optcode is in bits 0 to 6
    // word &= 0b1111111
    fields.optcode = word & 0x7F;

    // funct3 is in bits 12 to 14
    // shift right by 12, then keep lowest 3 bits
    fields.funct3 = (word >> 12) & 0x7;

    // rd is the destination register, in bits 7 to 11
    // shift right by 7, and find lowest 5 bits
    fields.rd = (word >> 7) & 0x1F;

    // rs1 is source register, in bits 15 to 19
    fields.rs1 = (word >> 15) & 0x1F;

    // immediate occupies bits 20 to 31
    fields.immediate = (word >> 20) & 0xFFF;

    // since immediate is a signed 12 bit number
    if (fields.immediate >= 2048) fields.immediate -= 4096;

    return fields;
}

int decodeAddi(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct ITypeFields fields = extractITypeFields(word);

    // ADDI has optcode 0x13, and funct3 is 0
    if (fields.optcode != 0x13 || fields.funct3 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "ADDI x%u, x%u, %d",
        fields.rd, fields.rs1, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeSlti(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct ITypeFields fields = extractITypeFields(word);

    if (fields.optcode != 0x13 || fields.funct3 != 2) return 0;

    int length = snprintf(
        output,bufferSize,
        "SLTI x%u, x%u, %d",
        fields.rd, fields.rs1, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeLw(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct ITypeFields fields = extractITypeFields(word);

    if (fields.optcode != 0x03 || fields.funct3 != 2) return 0;

    int length = snprintf(
        output,bufferSize,
        "LW x%u, %d(x%u)",
        fields.rd, fields.immediate, fields.rs1
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeJalr(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct ITypeFields fields = extractITypeFields(word);

    if (fields.optcode != 0x67 || fields.funct3 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "JALR x%u, x%u, %d",
        fields.rd, fields.rs1, fields.immediate
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}
