#ifndef RISCV_TRACE_H
#define RISCV_TRACE_H

#include <stdio.h>
#include <stdint.h>
#include "riscv/pipeline.h"

void printCycle(FILE* output, const Pipeline& pipeline, uint64_t cycle);
void printSummary(FILE* output, const Pipeline& pipeline);

#endif
