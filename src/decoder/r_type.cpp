#include <stdio.h>
#include <stdint.h>
#include "riscv/decoder.h"

using namespace std;

struct RTypeFields {
    unsigned int optcode;
    unsigned int funct3;
    unsigned int rd;
    unsigned int rs1;
    unsigned int rs2;
    unsigned int funct7;
};

static struct RTypeFields extractRTypeFields(uint32_t word) {
    struct RTypeFields fields;

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

    // rs2 is source register, in bits 20 to 24
    fields.rs2 = (word >> 20) & 0x1F;

    // funct7 is in bits 25 to 31
    // shift right by 25, then keep lowest 7 bits
    fields.funct7 = (word >> 25) & 0x7F;

    return fields;
}

int decodeAdd(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    // ADD has optcode 0x33, and funct3 is 0, and funct7 is 0
    if (fields.optcode != 0x33 || fields.funct3 != 0 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "ADD x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeSub(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 0 || fields.funct7 != 0x20) return 0;

    int length = snprintf(
        output,bufferSize,
        "SUB x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeAnd(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 7 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "AND x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeOr(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 6 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "OR x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeXor(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 4 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "XOR x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeSll(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 1 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "SLL x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeSrl(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 5 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "SRL x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}

int decodeSlt(uint32_t word, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    struct RTypeFields fields = extractRTypeFields(word);

    if (fields.optcode != 0x33 || fields.funct3 != 2 || fields.funct7 != 0) return 0;

    int length = snprintf(
        output,bufferSize,
        "SLT x%u, x%u, x%u",
        fields.rd, fields.rs1, fields.rs2
    );

    if (length < 0 || length >= bufferSize) return 0;

    return 1;
}
