# RISC-V Simulator

**Part I: Disassembler**  
Author: Karthikeyan Vetrivel

A C++ disassembler for 32-bit RISC-V instruction words. The planned output includes each instruction's address, binary representation, and assembly equivalent.

Part I covers disassembly. Pipeline simulation is planned for Part II.

## Instruction Set

| Category | Instructions |
| --- | --- |
| Arithmetic | `ADD`, `SUB`, `ADDI` |
| Logic | `AND`, `OR`, `XOR` |
| Shifts | `SLL`, `SRL` |
| Comparisons | `SLT`, `SLTI` |
| Memory | `LW`, `SW` |
| Branches | `BEQ`, `BNE`, `BGE`, `BLT` |
| Jumps and return | `JAL`, `JALR`, `RET` |

## Planned Structure

```text
RISC-Vsim/
├── CMakeLists.txt
├── README.md
├── include/
│   └── riscv/
│       └── decoder.h
├── src/
│   ├── main.cpp
│   └── decoder/
│       ├── decoder.cpp
│       ├── r_type.cpp
│       ├── i_type.cpp
│       ├── s_type.cpp
│       ├── b_type.cpp
│       └── j_type.cpp
└── proj_files/
```

## Build and Run

Once the planned source files and CMake configuration are in place:

```bash
cmake -B build
cmake --build build
```

```bash
./build/RISC-Vsim <inputfilename> <outputfilename> dis
```

For example, with an input file named `fibonacci.txt`:

```bash
./build/RISC-Vsim fibonacci.txt output.txt dis
```

## Notes

- Instruction addresses begin at `496`.
- Each instruction is 32 bits and occupies 4 bytes.
- `RET` is treated as `JALR x0, x1, 0`.
