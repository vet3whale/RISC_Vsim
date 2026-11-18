#include "riscv/pipeline.h"

#include <stdio.h>

// static if these functions are private, and another source file cannot define another of this functions
static bool dependsOn(const Instruction& consumer, const Instruction& producer) {
    // register data dependency only happens if older instruction actually writes a register
    if (!producer.writesRd()) return false; 
    return (consumer.readsRs1() && consumer.rs1 == producer.rd) ||
           (consumer.readsRs2() && consumer.rs2 == producer.rd); // either rs in consumer is same as rd of producer
}

static void forwardOperand(Pipeline& pipeline, PipelineSlot& consumer, bool first,
                    const PipelineSlot& producer, ForwardPath path) {
    // if producer is not valid, result not ready or producer nva write, no forwarding
    if (!producer.valid || !producer.resultReady || !producer.instruction.writesRd()) return;
    
    // first tells the function which source operand it is checking, Rs1 or Rs2
    // we skip forwarding if the consumer either does not read OR the operand has already received a forwarded value
    if (first ? (consumer.aForwarded || !consumer.instruction.readsRs1())
              : (consumer.bForwarded || !consumer.instruction.readsRs2())) return;

    uint8_t source = first ? consumer.instruction.rs1 : consumer.instruction.rs2;
    // if rs is either 0 or it isnt the producer's rd return, no pt checking
    if (source == 0 || source != producer.instruction.rd) return;
    
    // check if producer output is loaded or calc with ALU
    int32_t value = (producer.instruction.op == Operation::LW)
                  ? producer.loadedData : producer.aluResult;
    // pass in the value to be forwarded to the consumer
    if (first) {
        consumer.a = value;
        consumer.aForwarded = true;
    } else {
        consumer.b = value;
        consumer.bForwarded = true;
    }
    if (pipeline.forwardedCount < 16) {
        ForwardEvent& event = pipeline.forwarded[pipeline.forwardedCount++];
        event.path = path;
        event.producer = producer.instruction;
        event.consumer = consumer.instruction;
        event.reg = source;
    }
    ++pipeline.forwardingByPath[path];
    pipeline.forwardedThisCycle[path] |= 1u << source;
}

// try to forward rs1 and rs2. 
static void forwardBoth(Pipeline& pipeline, PipelineSlot& consumer,
                 const PipelineSlot& producer, ForwardPath path) {
    forwardOperand(pipeline, consumer, true, producer, path);
    forwardOperand(pipeline, consumer, false, producer, path);
}

static void executeEx(PipelineSlot& slot, bool& redirect, uint32_t& target) {
    const Instruction& instruction = slot.instruction;
    int32_t a = slot.a;
    int32_t b = slot.b;

    switch (instruction.op) {
        case Operation::ADD: case Operation::SUB: case Operation::AND:
        case Operation::OR: case Operation::XOR: case Operation::SLL:
        case Operation::SRL: case Operation::SLT: case Operation::ADDI:
        case Operation::SLTI:
            slot.aluResult = calculateAluResult(instruction, a, b);
            break;
        case Operation::LW:
        case Operation::SW:
            slot.effectiveAddress = (uint32_t)a + (uint32_t)instruction.immediate;
            break;
        case Operation::BEQ: redirect = (a == b); break;
        case Operation::BNE: redirect = (a != b); break;
        case Operation::BLT: redirect = (a < b); break;
        case Operation::BGE: redirect = (a >= b); break;
        case Operation::JAL: // JAL is Jump and Link
            slot.aluResult = (int32_t)(instruction.address + 4);
            redirect = true;
            break;
        case Operation::JALR: // targeted addr calc: PC = (rs1 + imm) & ~1
            slot.aluResult = (int32_t)(instruction.address + 4);
            redirect = true;
            target = ((uint32_t)a + (uint32_t)instruction.immediate) & ~1u;
            break;
        case Operation::NOP:
        case Operation::RET:
        case Operation::INVALID:
            break;
    }

    // target is the address of the next instruction to fetch, immediate can be 4,
    // but if its a branch can be different 
    if (redirect && instruction.op != Operation::JALR)
        target = instruction.address + (uint32_t)instruction.immediate;
    // This code runs in EX, and at EX, LW would have alr calculated memory address
    // resultReady for LW after DS completes memory read - IMPT
    // writesRD just checks if we wrote any result to slot.
    if (instruction.writesRd() && instruction.op != Operation::LW)
        slot.resultReady = true;
}

// Helpers are private to this file; pipeline.h needs no changes.
static void resetCycleEvents(Pipeline& p) {
    for (int i = 0; i < FORWARD_PATH_COUNT; ++i) p.forwardedThisCycle[i] = 0;
    p.hasStalledInstruction = false;
    p.detectedCount = 0;
    p.forwardedCount = 0;
}

static bool finishRet(Pipeline& p, const PipelineSlot& wb) {
    // RET ends the program after its WB cycle has completed.
    if (!wb.valid || wb.instruction.op != Operation::RET) return false;
    p.finished = true;
    p.machine.halted = true;
    for (int i = 0; i < STAGE_COUNT; ++i) p.stages[i] = PipelineSlot();
    ++p.machine.cycles;
    return true;
}

static void writeBack(Simulator& machine, const PipelineSlot& slot) {
    if (slot.valid && slot.instruction.writesRd())
        machine.registers[slot.instruction.rd] =
            (slot.instruction.op == Operation::LW) ? slot.loadedData : slot.aluResult;
    machine.registers[0] = 0;
}

static void accessMemory(Simulator& machine, PipelineSlot& slot) {
    if (!slot.valid) return;
    if (slot.instruction.op == Operation::LW) {
        std::map<uint32_t, int32_t>::const_iterator data =
            machine.memory.find(slot.effectiveAddress);
        slot.loadedData = (data == machine.memory.end()) ? 0 : data->second;
        slot.resultReady = true;
    } else if (slot.instruction.op == Operation::SW) {
        machine.memory[slot.effectiveAddress] = slot.b;
    }
}

static void forwardStoreData(Pipeline& p, PipelineSlot next[]) {
    PipelineSlot& store = next[DF_STAGE];
    if (!store.valid || store.instruction.op != Operation::SW) return;
    forwardOperand(p, store, false, next[DS_STAGE], DF_DS_TO_EX_DF);
    forwardOperand(p, store, false, next[WB_STAGE], DS_WB_TO_EX_DF);
}

static bool resolveControl(PipelineSlot& slot, uint32_t& target) {
    if (!slot.valid) return false;
    Operation op = slot.instruction.op;
    if (op != Operation::BEQ && op != Operation::BNE &&
        op != Operation::BLT && op != Operation::BGE &&
        op != Operation::JAL && op != Operation::JALR) return false;
    bool redirect = false;
    executeEx(slot, redirect, target);
    return redirect;
}

static void flushYoungerStages(Pipeline& p, PipelineSlot next[], uint32_t target) {
    // branch was resolved in EX, cancel the four younger stages
    for (int stage = IS_STAGE; stage <= EX_STAGE; ++stage) next[stage].stall = true;
    p.machine.pc = target;
    p.fetchStopped = false;
    p.machine.stalls += 4;
    p.branchStalls += 4;
}

static void stopOnError(Pipeline& p) {
    p.finished = true;
    p.failed = true;
    p.machine.halted = true;
}

// Newest producer first. Stores forward only their address operand here.
static void forwardExOperands(Pipeline& p, PipelineSlot& consumer,
                              const PipelineSlot& producer, ForwardPath path) {
    if (consumer.instruction.op == Operation::SW)
        forwardOperand(p, consumer, true, producer, path);
    else
        forwardBoth(p, consumer, producer, path);
}

static void executeStage(Pipeline& p, const PipelineSlot old[], PipelineSlot next[]) {
    next[EX_STAGE] = old[RF_STAGE]; // slot just dereferences to next[EX_STAGE]
    if (!next[EX_STAGE].valid) return;
    // WB can update the register file in this cycle.
    if (next[EX_STAGE].instruction.op == Operation::SW)
        next[EX_STAGE].b = p.machine.registers[next[EX_STAGE].instruction.rs2];

    // check if 3 of the older instruction (since they have not been written back yet) has a result need by the instruction entering EX
    forwardExOperands(p, next[EX_STAGE], next[DF_STAGE], EX_DF_TO_RF_EX); // next[DF_STAGE] is instruction that just completed EX
    forwardExOperands(p, next[EX_STAGE], next[DS_STAGE], DF_DS_TO_RF_EX); // next[DS_STAGE] is instruction completing memory access
    forwardExOperands(p, next[EX_STAGE], next[WB_STAGE], DS_WB_TO_RF_EX); // next[WB_STAGE] is instruction writing its result to a register this cycle

    // Preserve the original timing: apply control redirects on EX -> DF.
    bool ignoredRedirect = false;
    uint32_t ignoredTarget = 0;
    executeEx(next[EX_STAGE], ignoredRedirect, ignoredTarget);
    if (next[EX_STAGE].instruction.op == Operation::INVALID) {
        fprintf(stderr, "Invalid instruction at address %u\n", next[EX_STAGE].instruction.address);
        stopOnError(p);
    }
}

static bool hasLoadHazard(const PipelineSlot old[]) {
    if (!old[ID_STAGE].valid) return false;
    const Instruction& consumer = old[ID_STAGE].instruction;
    for (int operand = 0; operand < 2; ++operand) {
        // Store data is needed in DF, while its address is needed in EX.
        if (operand == 0 ? !consumer.readsRs1() :
            (!consumer.readsRs2() || consumer.op == Operation::SW)) continue;
        uint8_t source = operand == 0 ? consumer.rs1 : consumer.rs2;
        if (source == 0) continue;
        // The first matching writer supplies this operand, even if an older load exists.
        for (int stage = RF_STAGE; stage <= WB_STAGE; ++stage) {
            if (!old[stage].valid || !old[stage].instruction.writesRd() ||
                old[stage].instruction.rd != source) continue;
            if (old[stage].instruction.op == Operation::LW && stage <= EX_STAGE)
                return true;
            break;
        }
    }
    return false;
}

static void stallIssue(Pipeline& p, const PipelineSlot old[], PipelineSlot next[]) {
    next[RF_STAGE].stall = true;
    for (int stage = IF_STAGE; stage <= ID_STAGE; ++stage) next[stage] = old[stage];
    p.stalledInstruction = old[ID_STAGE].instruction;
    p.hasStalledInstruction = true;
    ++p.loadStalls;
    ++p.machine.stalls;
}

static void fetchInstruction(Pipeline& p, PipelineSlot& slot) {
    if (p.fetchStopped) return;
    std::map<uint32_t, Instruction>::const_iterator fetched =
        p.machine.instructions.find(p.machine.pc);
    if (fetched == p.machine.instructions.end()) {
        fprintf(stderr, "No instruction at address %u\n", p.machine.pc);
        stopOnError(p);
        return;
    }
    slot.instruction = fetched->second;
    slot.valid = true;
    if (slot.instruction.op == Operation::RET) p.fetchStopped = true;
    p.machine.pc += 4;
}

static void advanceIssue(Pipeline& p, const PipelineSlot old[], PipelineSlot next[]) {
    next[RF_STAGE] = old[ID_STAGE];
    PipelineSlot& rf = next[RF_STAGE];
    if (rf.valid) {
        rf.a = p.machine.registers[rf.instruction.rs1];
        rf.b = p.machine.registers[rf.instruction.rs2];
    }
    next[ID_STAGE] = old[IS_STAGE];
    // complete the pending fetch while RET moves into decode
    if (p.fetchStopped && old[IS_STAGE].valid &&
        old[IS_STAGE].instruction.op == Operation::RET) p.machine.pc += 4;
    fetchInstruction(p, next[IS_STAGE]);
}

static void detectDependencies(Pipeline& p, const PipelineSlot next[]) {
    // report a dependency as soon as the consumer reaches ID
    if (!next[ID_STAGE].valid) return;
    const Instruction& consumer = next[ID_STAGE].instruction;
    for (int stage = RF_STAGE; stage <= WB_STAGE; ++stage) {
        const Instruction& producer = next[stage].instruction;
        if (!next[stage].valid || !dependsOn(consumer, producer)) continue;
        bool seen = false;
        for (int i = 0; i < p.detectedCount; ++i)
            if (p.detected[i].producer.address == producer.address &&
                p.detected[i].consumer.address == consumer.address) seen = true;
        if (seen || p.detectedCount >= 16) continue;
        DependencyEvent& event = p.detected[p.detectedCount++];
        event.producer = producer;
        event.consumer = consumer;
        event.reg = producer.rd;
    }
}

// in this function:
// 1. read old pipeline state
// 2. determine writes, memory actions, forwarding hazards, and branch results for this cycle
// 3. build next pipeline state
// 4. replace old state and increment cycle count
void Pipeline::step() {
    if (finished || machine.halted) return;

    // STAGE_COUNT is enum, its int value is 8
    // here we are declaring old PipelineSlot + new PipelineSlot
    PipelineSlot old[STAGE_COUNT]; // snapshot before cycle updates
    PipelineSlot next[STAGE_COUNT]; // state being build for the end of the cycle
    for (int i = 0; i < STAGE_COUNT; ++i) old[i] = stages[i];
    resetCycleEvents(*this);
    if (finishRet(*this, old[WB_STAGE])) return;

    // advance older stages first so their results are available for forwarding
    // since second stage of data fetch is done, can writeback to register
    next[WB_STAGE] = old[DS_STAGE];  
    writeBack(machine, next[WB_STAGE]);

    // move from DF to DS, and access memory depending on whether sw or lw
    next[DS_STAGE] = old[DF_STAGE];
    accessMemory(machine, next[DS_STAGE]);
    
    // execution done, forward any needed sw data from older instructions
    next[DF_STAGE] = old[EX_STAGE];
    forwardStoreData(*this, next);

    // here we decide whether to redirect pipeline because of a taken branch/jump or not
    uint32_t target = 0;
    // next[DF_stage] contains instruction that just moved from EX to DF
    if (resolveControl(next[DF_STAGE], target)) { // checks if branch
        // the following cancels 4 younger instructions (they are in RF, ID, IS, IF) by inserting bubbles
        // then sets the PC to target and adds four branch stalls
        flushYoungerStages(*this, next, target);
    } else {
        executeStage(*this, old, next); // no branch simply execute 
        // check if instruction currently in ID depends on a load
        if (hasLoadHazard(old)) stallIssue(*this, old, next);
        else advanceIssue(*this, old, next);
    }

    detectDependencies(*this, next);
    for (int i = 0; i < STAGE_COUNT; ++i) stages[i] = next[i];
    ++machine.cycles;
}
