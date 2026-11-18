#ifndef RISCV_PIPELINE_H
#define RISCV_PIPELINE_H

#include <stdint.h>
#include "riscv/simulator.h"

enum PipelineStage {
    IF_STAGE, IS_STAGE, ID_STAGE, RF_STAGE,
    EX_STAGE, DF_STAGE, DS_STAGE, WB_STAGE,
    STAGE_COUNT
};

enum ForwardPath {
    EX_DF_TO_RF_EX,
    DF_DS_TO_EX_DF,
    DF_DS_TO_RF_EX,
    DS_WB_TO_EX_DF,
    DS_WB_TO_RF_EX,
    FORWARD_PATH_COUNT
};

struct DependencyEvent {
    Instruction producer;
    Instruction consumer;
    uint8_t reg = 0;
};

struct ForwardEvent {
    ForwardPath path = EX_DF_TO_RF_EX;
    Instruction producer;
    Instruction consumer;
    uint8_t reg = 0;
};
struct PipelineSlot {
    Instruction instruction;
    bool valid = false;
    bool stall = false; // A stall bubble; an empty slot has both flags false.
    int32_t a = 0;
    int32_t b = 0;
    int32_t aluResult = 0;
    int32_t loadedData = 0;
    uint32_t effectiveAddress = 0;
    bool resultReady = false;
    bool aForwarded = false;
    bool bForwarded = false;
};

struct Pipeline {
    Simulator& machine;
    PipelineSlot stages[STAGE_COUNT];
    bool fetchStopped = false;
    bool finished = false;
    bool failed = false;

    uint64_t loadStalls = 0;
    uint64_t branchStalls = 0;
    uint64_t forwardingByPath[FORWARD_PATH_COUNT] = {};
    uint32_t forwardedThisCycle[FORWARD_PATH_COUNT] = {}; // Bit i means register xi.
    DependencyEvent detected[16];
    int detectedCount = 0;
    ForwardEvent forwarded[16];
    int forwardedCount = 0;
    Instruction stalledInstruction;
    bool hasStalledInstruction = false;

    explicit Pipeline(Simulator& simulator) : machine(simulator) {}
    void step();
};

#endif
