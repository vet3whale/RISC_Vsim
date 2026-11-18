#include <iostream>
#include <fstream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "riscv/decoder.h"
#include "riscv/trace.h"

using namespace std;

const char* helpText =
    "Usage: RISC-Vsim <inputfilename> <outputfilename> dis|sim [-T m:n]\n"
    "  inputfilename: text file containing one 32-bit binary word per line\n"
    "  outputfilename: file to write the disassembly to\n"
    "  dis: disassemble instructions through RET, then print signed data\n"
    "  sim: run the eight-stage pipeline simulator\n"
    "  optional: -T m:n traces cycles m through n; -T 0:0 prints summary only\n"
    "Example: ./build/RISC-Vsim proj_files/part1/fibonacci_input.txt output.txt sim\n";

int printError(const char* message) {
    cerr << "Error: " << message << '\n' << helpText;
    return 1;
}

int formatInstruction(uint32_t word, unsigned int address, char* output, int bufferSize) {
    if (output == NULL || bufferSize <= 0) return 0;
    output[0] = '\0';

    char assembly[64];
    if (!decodeInstruction(word, assembly, sizeof(assembly))) return 0;

    int length;
    if (word == 0x00008067) { // RET
        // RET == JALR x0, x1, 0
        length = snprintf(output,bufferSize, "RET \t\t//JALR x0, x1, 0");
    } else if (word == 0x00000013) {
        // NOP == ADDI x0, x0, 0
        length = snprintf(output,bufferSize, "NOP \t\t//ADDI x0, x0, 0 ");
    } else {
        int immediate;
        if (sscanf(assembly, "JAL x0, %d", &immediate) == 1) {
            // J is JAL x0, immediate, and the sample prints its target address
            unsigned int target = address + immediate;
            length = snprintf(output,bufferSize, "J\t#%u  //JAL x0, %d", target, immediate);
        }
        else {
            // separate the instruction name from its arguments
            char* arguments = strchr(assembly, ' ');
            if (arguments == NULL) return 0;
            *arguments = '\0';
            arguments++;

            // the sample uses six characters for the instruction name and spaces
            length = snprintf(output,bufferSize, "%-6s%s", assembly, arguments);
        }
    }

    if (length < 0 || length >= bufferSize) return 0;
    return 1;
}

static int parseTraceRange(int argc, char* argv[], uint64_t& firstCycle, uint64_t& lastCycle) {
    firstCycle = 0;
    lastCycle = UINT64_MAX;
    if (argc != 4) {
        const char* range = NULL;
        if (argc == 6 && strcmp(argv[4], "-T") == 0) range = argv[5];
        else if (argc == 5 && strncmp(argv[4], "-T", 2) == 0) range = argv[4] + 2;
        else return printError("Expected -T m:n.");
        if (*range < '0' || *range > '9') return printError("Expected -T m:n.");
        char* end = NULL;
        errno = 0;
        unsigned long long first = strtoull(range, &end, 10);
        if (errno || end == range || *end != ':') return printError("Invalid trace range.");
        const char* second = end + 1;
        if (*second < '0' || *second > '9') return printError("Invalid trace range.");
        errno = 0;
        unsigned long long last = strtoull(second, &end, 10);
        if (errno || end == second || *end != '\0' || first > last)
            return printError("Invalid trace range.");
        firstCycle = first;
        lastCycle = last;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 6)
        return printError("Expected an input file, output file, and operation.");

    const char* inputFileName = argv[1];
    const char* outputFileName = argv[2];
    const char* operation = argv[3];
    bool dis = (strcmp(operation, "dis") == 0);
    bool sim = (strcmp(operation, "sim") == 0);

    if (!dis && !sim) return printError("Invalid operation. Use dis or sim.");
    if (strcmp(inputFileName, outputFileName) == 0) {
        return printError("Input and output filenames must be different.");
    }

    if (dis) {
        if (argc != 4) return printError("Trace mode is only available for sim.");

        ifstream inputFile(inputFileName);
        if (!inputFile) return printError("Cannot open input file.");

        // binary mode keeps the sample's line endings the same on Linux and Windows
        ofstream outputFile(outputFileName, ios::binary);
        if (!outputFile) return printError("Cannot create output file.");

        // Part I disassembler
        unsigned int address = 496;
        unsigned int lineNumber = 0;
        int dataSection = 0;
        int firstLine = 1;
        char line[256];

        while (inputFile.getline(line, sizeof(line))) {
            lineNumber++;

            // remove spaces and tabs before the binary word
            char* binary = line;
            while (*binary == ' ' || *binary == '\t') binary++;

            // remove spaces, tabs, and carriage returns after the binary word
            int length = strlen(binary);
            while (length > 0 && (binary[length - 1] == ' ' ||
                                 binary[length - 1] == '\t' ||
                                 binary[length - 1] == '\r')) {
                binary[--length] = '\0';
            }

            if (length != 32) {
                cerr << "Input line " << lineNumber << ": ";
                return printError("Expected 32 binary digits.");
            }

            uint32_t word = 0;
            for (int i = 0; i < 32; i++) {
                if (binary[i] != '0' && binary[i] != '1') {
                    cerr << "Input line " << lineNumber << ": ";
                    return printError("Invalid binary digit.");
                }
                word = (word << 1) | (binary[i] - '0');
            }

            // decoding part
            char assembly[128];
            if (!dataSection && !formatInstruction(word, address, assembly, sizeof(assembly))) {
                cerr << "Input line " << lineNumber << ": ";
                return printError("Unsupported instruction before RET.");
            }

            if (!firstLine) {
                outputFile << "\r\n";
            }
            firstLine = 0;
        
            if (dataSection) {
                // interpret the word as a signed 32 bit number
                int32_t value = static_cast<int32_t>(word);
                outputFile << binary << "      \t" << address << '\t' << value;
            }
            else {
                // split instruction bits into groups of 6, 6, 5, 3, 5, and 7
                for (int i = 0; i < 32; i++) {
                    if (i == 6 || i == 12 || i == 17 || i == 20 || i == 25) outputFile << ' ';
                    outputFile << binary[i];
                }
                outputFile << '\t' << address << '\t' << assembly;

                // words after RET are data, even if their bits look like instructions
                if (word == 0x00008067) dataSection = 1;
            }

            address += 4;
        }

        if (!inputFile.eof()) return printError("Cannot read input or input line is too long.");
        if (!dataSection) return printError("The instruction sequence must end with RET.");

        outputFile.close();
        if (!outputFile) return printError("Cannot write output file.");

        return 0;
    } else if (sim) {
        uint64_t firstCycle, lastCycle;
        if (parseTraceRange(argc, argv, firstCycle, lastCycle)) return 1;

        Simulator machine;
        if (!machine.loadProgram(inputFileName)) return printError("Cannot load simulation input.");

        FILE* output = fopen(outputFileName, "wb");
        if (output == NULL) return printError("Cannot create output file.");
        
        Pipeline pipeline(machine);
        bool summaryOnly = (firstCycle == 0 && lastCycle == 0 && argc != 4);
        if (!summaryOnly && firstCycle == 0) printCycle(output, pipeline, 0);
        
        while (!pipeline.finished && !machine.halted) {
            pipeline.step();
            if (!summaryOnly && machine.cycles >= firstCycle && machine.cycles <= lastCycle)
                printCycle(output, pipeline, machine.cycles);
        }
        
        printSummary(output, pipeline);
        
        bool writeOk = (fclose(output) == 0);
        if (!writeOk) return printError("Cannot write output file.");
        if (pipeline.failed) return printError("Simulation stopped on an invalid instruction.");
        return 0;
    }
    return 0;
}
