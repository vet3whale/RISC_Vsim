#ifndef RISCV_INSTRUCTION_H
#define RISCV_INSTRUCTION_H

#include <stdint.h>

enum class Operation {
    INVALID,
    NOP,
    RET,
    ADD, SUB, AND, OR, XOR, SLL, SRL, SLT,
    ADDI, SLTI,
    LW, SW,
    BEQ, BNE, BLT, BGE,
    JAL, JALR
};

struct Instruction {
    uint32_t word = 0; // Original 32-bit instruction
    uint32_t address = 0; // Address of this instruction
    Operation op = Operation::INVALID;

    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    int32_t immediate = 0;   // Sign-extended immediate, in bytes for jumps/branches

    bool readsRs1() const {
        switch (op) {
            case Operation::ADD: case Operation::SUB: case Operation::AND:
            case Operation::OR: case Operation::XOR: case Operation::SLL:
            case Operation::SRL: case Operation::SLT: case Operation::ADDI:
            case Operation::SLTI: case Operation::LW: case Operation::SW:
            case Operation::BEQ: case Operation::BNE: case Operation::BLT:
            case Operation::BGE: case Operation::JALR:
                return true;
            default:
                return false;
        }
    }

    bool readsRs2() const {
        switch (op) {
            case Operation::ADD: case Operation::SUB: case Operation::AND:
            case Operation::OR: case Operation::XOR: case Operation::SLL:
            case Operation::SRL: case Operation::SLT: case Operation::SW:
            case Operation::BEQ: case Operation::BNE: case Operation::BLT:
            case Operation::BGE:
                return true;
            default:
                return false;
        }
    }

    bool writesRd() const {
        if (rd == 0) return false;
        switch (op) {
            case Operation::ADD: case Operation::SUB: case Operation::AND:
            case Operation::OR: case Operation::XOR: case Operation::SLL:
            case Operation::SRL: case Operation::SLT: case Operation::ADDI:
            case Operation::SLTI: case Operation::LW: case Operation::JAL:
            case Operation::JALR:
                return true;
            default:
                return false;
        }
    }
};

#endif