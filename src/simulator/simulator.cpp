#include "riscv/simulator.h"
#include "riscv/decoder.h"

#include <stdio.h>
#include <string.h>

int32_t calculateAluResult(const Instruction& instruction, int32_t a, int32_t b) {
    switch (instruction.op) {
        case Operation::ADD:  return (int32_t)((uint32_t)a + (uint32_t)b);
        case Operation::SUB:  return (int32_t)((uint32_t)a - (uint32_t)b);
        case Operation::AND:  return a & b;
        case Operation::OR:   return a | b;
        case Operation::XOR:  return a ^ b;
        case Operation::SLL:  return (int32_t)((uint32_t)a << ((uint32_t)b & 31));
        case Operation::SRL:  return (int32_t)((uint32_t)a >> ((uint32_t)b & 31));
        case Operation::SLT:  return (a < b) ? 1 : 0;
        case Operation::ADDI: return (int32_t)((uint32_t)a + (uint32_t)instruction.immediate);
        case Operation::SLTI: return (a < instruction.immediate) ? 1 : 0;
        default: return 0; // Called only for arithmetic instructions.
    }
}

bool Simulator::loadProgram(const char* filename) {
    if (filename == NULL) return false;

    FILE* file = fopen(filename, "r");
    if (file == NULL) return false;

    std::map<uint32_t, Instruction> newInstructions;
    std::map<uint32_t, int32_t> newMemory;
    uint32_t address = START_ADDRESS;
    bool dataSection = false;
    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        char* bits = line;
        while (*bits == ' ' || *bits == '\t') ++bits;

        size_t length = strlen(bits);
        while (length > 0 && (bits[length - 1] == ' ' ||
                              bits[length - 1] == '\t' ||
                              bits[length - 1] == '\r' ||
                              bits[length - 1] == '\n')) {
            bits[--length] = '\0';
        }

        if (length != 32) {
            fclose(file);
            return false;
        }

        uint32_t word = 0;
        for (int i = 0; i < 32; ++i) {
            if (bits[i] != '0' && bits[i] != '1') {
                fclose(file);
                return false;
            }
            word = (word << 1) | (bits[i] - '0'); // add each bit to word
        }

        if (dataSection) {
            newMemory[address] = (int32_t)word;
        } else {
            Instruction instruction;
            if (!decodeInstruction(word, address, instruction)) {
                fclose(file);
                return false;
            }
            newInstructions[address] = instruction;
            if (instruction.op == Operation::RET) dataSection = true;
        }

        address += 4;
    }

    bool readOk = !ferror(file);
    bool closeOk = (fclose(file) == 0);
    if (!readOk || !closeOk || !dataSection) return false;

    instructions.swap(newInstructions);
    memory.swap(newMemory);
    for (int i = 0; i < 32; ++i) registers[i] = 0;
    pc = START_ADDRESS;
    cycles = 0;
    stalls = 0;
    halted = false;
    return true;
}

void Simulator::stepSequential() {
    if (halted) return;

    std::map<uint32_t, Instruction>::const_iterator current = instructions.find(pc);
    if (current == instructions.end()) {
        fprintf(stderr, "No instruction at address %u\n", pc);
        halted = true;
        return;
    }

    const Instruction& instruction = current->second;
    registers[0] = 0;
    const int32_t a = registers[instruction.rs1];
    const int32_t b = registers[instruction.rs2];
    uint32_t nextPc = pc + 4;
    int32_t result = 0;
    bool writeRd = false;

    switch (instruction.op) {
        case Operation::NOP:
            break;
        case Operation::RET:
            halted = true;
            ++cycles;
            return;

            
        // The sequential and pipelined paths use the same ALU calculation.
        case Operation::ADD: case Operation::SUB: case Operation::AND:
        case Operation::OR: case Operation::XOR: case Operation::SLL:
        case Operation::SRL: case Operation::SLT: case Operation::ADDI:
        case Operation::SLTI:
            result = calculateAluResult(instruction, a, b);
            writeRd = true;
            break;
        case Operation::LW: {
            uint32_t address = (uint32_t)a + (uint32_t)instruction.immediate;
            std::map<uint32_t, int32_t>::const_iterator data = memory.find(address);
            result = (data == memory.end()) ? 0 : data->second;
            writeRd = true;
            break;
        }
        case Operation::SW: {
            uint32_t address = (uint32_t)a + (uint32_t)instruction.immediate;
            memory[address] = b;
            break;
        }

        case Operation::BEQ:
            if (a == b) nextPc = pc + (uint32_t)instruction.immediate;
            break;
        case Operation::BNE:
            if (a != b) nextPc = pc + (uint32_t)instruction.immediate;
            break;
        case Operation::BLT:
            if (a < b) nextPc = pc + (uint32_t)instruction.immediate;
            break;
        case Operation::BGE:
            if (a >= b) nextPc = pc + (uint32_t)instruction.immediate;
            break;
        case Operation::JAL:
            result = (int32_t)(pc + 4);
            writeRd = true;
            nextPc = pc + (uint32_t)instruction.immediate;
            break;
        case Operation::JALR:
            result = (int32_t)(pc + 4);
            writeRd = true;
            nextPc = ((uint32_t)a + (uint32_t)instruction.immediate) & ~1u;
            break;

        case Operation::INVALID:
        default:
            fprintf(stderr, "Invalid instruction at address %u\n", pc);
            halted = true;
            return;
    }

    if (writeRd && instruction.rd != 0) registers[instruction.rd] = result;
    registers[0] = 0;
    pc = nextPc;
    ++cycles;
}
