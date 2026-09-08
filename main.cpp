#include <iostream>
#include <fstream>
#include <cstring>

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

    if (!outputFile)
    {
        cout << "Error: Cannot create output file.\n";
        return 1;
    }

    if (strcmp(operation, "dis") == 0)
    {
        // Part I disassembler goes here
    }
    else if (strcmp(operation, "sim") == 0)
    {
        // Part II goes here later
    }
    else
    {
        cout << "Invalid operation. Use dis or sim.\n";
        return 1;
    }

    return 0;
}