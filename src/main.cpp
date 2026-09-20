#include <iostream>
#include <fstream>
#include <cstring>

#include "riscv/decoder.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: RISC-Vsim inputfilename outputfilename operation\n";
        return 1;
    }

    const char* inputFileName = argv[1];
    const char* outputFileName = argv[2];
    const char* operation = argv[3];

    ifstream inputFile(inputFileName);

    if (!inputFile) {
        cout << "Error: Cannot open input file.\n";
        return 1;
    }

    ofstream outputFile(outputFileName);

    if (!outputFile) {
        cout << "Error: Cannot create output file.\n";
        return 1;
    }

    if (strcmp(operation, "dis") == 0) {
        // Part I disassembler
        unsigned int address = 496;
        char binary[33];

        while (inputFile.peek() != EOF) {
            inputFile.width(sizeof(binary));
            if (!(inputFile >> binary)) break;
            if (strlen(binary) != 32) {
                cout << "Error: Expected 32 binary digits." << endl;
                return 1;
            }
            
            uint32_t word = 0;

            for (int i = 0; i < 32; i++) {
                if (binary[i] != '0' && binary[i] != '1') {
                    cout << "Error: Invalid binary digit." << endl;
                    return 1;
                }
                word = (word << 1) | (binary[i] - '0');
            }
            
            // decoding part
            char assembly[64];
            if (decodeAddi(word, assembly, sizeof(assembly))) {
                outputFile << assembly;
            } else {
                outputFile << "[not decoded yet]";
            }

            outputFile << '\n';

            address += 4;
        }
    }
    else if (strcmp(operation, "sim") == 0) {
    }
    else {
        cout << "Invalid operation. Use dis or sim.\n";
        return 1;
    }

    return 0;
}
