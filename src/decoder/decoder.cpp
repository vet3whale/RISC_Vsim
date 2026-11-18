#include <stdio.h>
#include "riscv/decoder.h"

// part 1
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

// part 2: decode the instruction fields for simulation
bool decodeInstruction(uint32_t word, uint32_t address, Instruction& result) {
    Instruction decoded;
    decoded.word = word;
    decoded.address = address;

    unsigned int opcode = word & 0b1111111;
    unsigned int funct3 = (word >> 12) & 0b111;
    unsigned int funct7 = (word >> 25) & 0b1111111;

    switch (opcode) {
        case 0b0010011: // ADDI, SLTI
        case 0b0000011: // LW
        case 0b1100111: { // JALR
            decoded.rd = (word >> 7) & 0b11111;
            decoded.rs1 = (word >> 15) & 0b11111;
            decoded.immediate = (word >> 20) & 0b111111111111;
            if (decoded.immediate >= 2048) decoded.immediate -= 4096;

            if (opcode == 0b0010011) {
                switch (funct3) {
                    case 0b000: decoded.op = (word == 0x00000013) ? Operation::NOP : Operation::ADDI; break;
                    case 0b010: decoded.op = Operation::SLTI; break;
                    default: return false;
                }
            } else if (opcode == 0b0000011 && funct3 == 0b010) {
                decoded.op = Operation::LW;
            } else if (opcode == 0b1100111 && funct3 == 0b000) {
                decoded.op = (word == 0x00008067) ? Operation::RET : Operation::JALR;
            } else {
                return false;
            }
            break;
        }

        case 0b0110011: { // R-type arithmetic
            decoded.rd = (word >> 7) & 0b11111;
            decoded.rs1 = (word >> 15) & 0b11111;
            decoded.rs2 = (word >> 20) & 0b11111;

            if (funct3 == 0b000 && funct7 == 0b0100000) {
                decoded.op = Operation::SUB;
            } else if (funct7 == 0b0000000) {
                switch (funct3) {
                    case 0b000: decoded.op = Operation::ADD; break;
                    case 0b001: decoded.op = Operation::SLL; break;
                    case 0b010: decoded.op = Operation::SLT; break;
                    case 0b100: decoded.op = Operation::XOR; break;
                    case 0b101: decoded.op = Operation::SRL; break;
                    case 0b110: decoded.op = Operation::OR; break;
                    case 0b111: decoded.op = Operation::AND; break;
                    default: return false;
                }
            } else {
                return false;
            }
            break;
        }

        case 0b0100011: { // SW
            if (funct3 != 0b010) return false;
            decoded.op = Operation::SW;
            decoded.rs1 = (word >> 15) & 0b11111;
            decoded.rs2 = (word >> 20) & 0b11111;
            decoded.immediate = ((word >> 7) & 0b11111) |
                                (((word >> 25) & 0b1111111) << 5);
            if (decoded.immediate >= 2048) decoded.immediate -= 4096;
            break;
        }

        case 0b1100011: { // Branches
            switch (funct3) {
                case 0b000: decoded.op = Operation::BEQ; break;
                case 0b001: decoded.op = Operation::BNE; break;
                case 0b100: decoded.op = Operation::BLT; break;
                case 0b101: decoded.op = Operation::BGE; break;
                default: return false;
            }
            decoded.rs1 = (word >> 15) & 0b11111;
            decoded.rs2 = (word >> 20) & 0b11111;
            decoded.immediate = (((word >> 8) & 0b1111) << 1) |
                                (((word >> 25) & 0b111111) << 5) |
                                (((word >> 7) & 0b1) << 11) |
                                (((word >> 31) & 0b1) << 12);
            if (decoded.immediate >= 4096) decoded.immediate -= 8192;
            break;
        }

        case 0b1101111: { // JAL
            decoded.op = Operation::JAL;
            decoded.rd = (word >> 7) & 0b11111;
            decoded.immediate = (((word >> 21) & 0b1111111111) << 1) |
                                (((word >> 20) & 0b1) << 11) |
                                (((word >> 12) & 0b11111111) << 12) |
                                (((word >> 31) & 0b1) << 20);
            if (decoded.immediate >= 1048576) decoded.immediate -= 2097152;
            break;
        }

        default:
            return false;
    }

    result = decoded;
    return true;
}
