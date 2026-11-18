#include "riscv/trace.h"
#include "riscv/decoder.h"
#include <inttypes.h>

static const char* stageNames[] = {"IF","IS","ID","RF","EX","DF","DS","WB"};
static const char* pathNames[] = {"EX/DF -> RF/EX","DF/DS -> EX/DF","DF/DS -> RF/EX","DS/WB -> EX/DF","DS/WB -> RF/EX"};

static void printInstruction(FILE* out, const Instruction& instruction) {
    if (instruction.op == Operation::NOP) { fprintf(out, "NOP"); return; }
    if (instruction.op == Operation::RET) { fprintf(out, "RET"); return; }
    char text[128];
    if (decodeInstruction(instruction.word, text, sizeof(text))) fprintf(out, "%s", text);
    else fprintf(out, "INVALID");
}
static int32_t displayedResult(const PipelineSlot& slot) {
    if (!slot.valid) return 0;
    const Instruction& instruction = slot.instruction;
    if (instruction.op == Operation::LW)
        return slot.resultReady ? slot.loadedData : (int32_t)slot.effectiveAddress;
    if (instruction.op == Operation::SW) return (int32_t)slot.effectiveAddress;
    if (instruction.op == Operation::BEQ || instruction.op == Operation::BNE ||
        instruction.op == Operation::BLT || instruction.op == Operation::BGE ||
        instruction.op == Operation::JAL)
        return (int32_t)(instruction.address + (uint32_t)instruction.immediate);
    if (instruction.op == Operation::JALR)
        return (int32_t)(((uint32_t)slot.a + (uint32_t)instruction.immediate) & ~1u);
    return slot.aluResult;
}
static void printState(FILE* out, const Pipeline& p) {
    fprintf(out, "Integer registers:\n");
    for (int i=0; i<32; ++i) {
        fprintf(out, "R%d\t%d\t", i, p.machine.registers[i]);
        if (i%4==3) fputc('\n', out);
    }
    fprintf(out, "\nData memory:\n");
    uint32_t dataStart = 600;
    if (!p.machine.instructions.empty() && p.machine.instructions.rbegin()->first >= 600)
        dataStart = p.machine.instructions.rbegin()->first + 4;
    for (std::map<uint32_t,int32_t>::const_iterator it=p.machine.memory.lower_bound(dataStart);
         it!=p.machine.memory.end(); ++it)
        fprintf(out, "%u: %d\n", it->first, it->second);
    fprintf(out, "\nTotal Stalls:\n*Loads\t: %" PRIu64 "\n*Branches: %" PRIu64
            "\n*Other\t: %" PRIu64 "\n", p.loadStalls, p.branchStalls,
            p.machine.stalls-p.loadStalls-p.branchStalls);
    fprintf(out, "\nTotal Forwardings:\n");
    for (int i=0; i<FORWARD_PATH_COUNT; ++i)
        fprintf(out, "* %s : %" PRIu64 "\n", pathNames[i], p.forwardingByPath[i]);
}
void printCycle(FILE* out, const Pipeline& p, uint64_t cycle) {
    fprintf(out, "***** Cycle #%" PRIu64 "***********************************************\n", cycle);
    fprintf(out, "Current PC = %u:\n\nPipeline Status:\n", p.machine.pc);
    for (int i=0; i<STAGE_COUNT; ++i) {
        const PipelineSlot& s=p.stages[i];
        fprintf(out, "* %s : ", stageNames[i]);
        if (s.stall) fprintf(out, "**STALL**");
        else if (!s.valid) fprintf(out, i==IF_STAGE ? "<unknown>" : "NOP");
        else if (i==IF_STAGE || i==IS_STAGE) {
            uint32_t word=s.instruction.word;
            fprintf(out, "<Fetched %02X %02X %02X %02X>", (word>>24)&255,
                    (word>>16)&255, (word>>8)&255, word&255);
        } else printInstruction(out, s.instruction);
        fputc('\n', out);
    }
    fprintf(out, "\nStall Instruction: ");
    if (p.hasStalledInstruction) printInstruction(out, p.stalledInstruction);
    else fprintf(out, "(none)");
    fprintf(out, "\n\nForwarding:\n Detected: ");
    if (p.detectedCount==0) fprintf(out, "(none)");
    for (int i=0; i<p.detectedCount; ++i) {
        if (i) fprintf(out, ", ");
        fputc('(',out); printInstruction(out,p.detected[i].producer);
        fprintf(out, ") to ("); printInstruction(out,p.detected[i].consumer); fputc(')',out);
    }
    fprintf(out, "\n Forwarded:\n");
    for (int i=0; i<FORWARD_PATH_COUNT; ++i) {
        fprintf(out, " * %s : ", pathNames[i]);
        uint32_t bits=p.forwardedThisCycle[i];
        if (!bits) fprintf(out, "(none)");
        for (int r=1; r<32; ++r) if (bits&(1u<<r)) fprintf(out, "R%d ", r);
        fputc('\n',out);
    }
    const PipelineSlot& rf=p.stages[RF_STAGE];
    const PipelineSlot& ex=p.stages[EX_STAGE];
    const PipelineSlot& ds=p.stages[DS_STAGE];
    fprintf(out, "\nPipeline Registers:\n* IF/IS.NPC\t: %u\n", p.machine.pc+4);
    if (p.stages[IS_STAGE].valid) {
        uint32_t word = p.stages[IS_STAGE].instruction.word;
        fprintf(out, "* IS/ID.IR\t: <%02X %02X %02X %02X>\n",
                (word>>24)&255, (word>>16)&255, (word>>8)&255, word&255);
    } else fprintf(out, "* IS/ID.IR\t: 0\n");
    fprintf(out, "* RF/EX.A\t: %d\n* RF/EX.B\t: %d\n", rf.valid?rf.a:0, rf.valid?rf.b:0);
    fprintf(out, "* EX/DF.ALUout\t: %d\n* EX/DF.B\t: %d\n",
            displayedResult(ex), ex.valid?ex.b:0);
    fprintf(out, "* DS/WB.ALUout-LMD\t : %d\n\n",
            displayedResult(ds));
    printState(out,p); fputc('\n',out);
}
void printSummary(FILE* out, const Pipeline& p) {
    uint32_t retAddress = 0;
    for (std::map<uint32_t, Instruction>::const_iterator it = p.machine.instructions.begin();
         it != p.machine.instructions.end(); ++it)
        if (it->second.op == Operation::RET) {
            retAddress = it->first;
            break;
        }
    fprintf(out, "**** Summary ************************************************\n");
    fprintf(out, "BREAK PC = %u\nTotal Cycles Simulated = %" PRIu64 "\n",
            retAddress, p.machine.cycles);
    printState(out,p);
}
