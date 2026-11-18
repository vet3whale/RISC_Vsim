// include/riscv/simulator.h
#ifndef RISCV_SIMULATOR_H
#define RISCV_SIMULATOR_H

#include <stdint.h>
#include <map>
#include "riscv/instruction.h"

#define START_ADDRESS 496

int32_t calculateAluResult(const Instruction& instruction, int32_t a, int32_t b);

struct Simulator {
    int32_t registers[32] = {}; // x0–x31
    std::map<uint32_t, Instruction> instructions; // Byte address -> instruction
    std::map<uint32_t, int32_t> memory;           // Byte address -> data word

    uint32_t pc = START_ADDRESS;
    uint64_t cycles = 0;
    uint64_t stalls = 0;
    bool halted = false;

    bool loadProgram(const char* filename);
    void stepSequential();
};

#endif