#include "pipeline.h"



void execute_stage(Core * core){
    // Is core halted? Is pipeline stalled? 
    // Is instruction not even using the ALU?
    // SKIP.
    // or else...
    if (core == NULL) return; //first check the the Core pointer works
    if (core->pipe.execute.active == 0) return; //Now check if were halted or bubbled
    // Perform the stage
    Instruction *inst = &core->pipe.execute.inst;
    int32_t rt_val = core->regs[core->pipe.execute.inst.rt];
    int32_t rs_val = core->regs[core->pipe.execute.inst.rs];
    int32_t results = core->pipe.execute.result;
    uint16_t addr = (core->regs[core->pipe.execute.inst.rd]) & 0b1111111111; //R[rd][low bits 9:0]
    switch (inst->opcode) {
        case OP_ADD:
            results = rs_val + rt_val;
            break;
        case OP_SUB:
            results = rs_val - rt_val;
            break;
        case OP_AND:
            results = rs_val & rt_val;
            break;
        case OP_OR:
            results = rs_val | rt_val;
            break;
        case OP_XOR:
            results = rs_val ^ rt_val;
            break;
        case OP_MUL:
            results = rs_val * rt_val;
            break;
        case OP_SLL:
            results = (int32_t)((uint32_t)rs_val << rt_val);
            break;
        case OP_SRA:
            results = rs_val >> rt_val;
            break;
        case OP_SRL:
            results = (int32_t)((uint32_t)rs_val >> rt_val);
            break;
    }
    core->pipe.execute.result = results;
    return;
    // Reminders:
    // register number is at core->pipe.execute.inst.rx
    // register are at core->reg_file.regs[reg]; READ ONLY!
    // core->pipe.execute.result = result;
    // This will go in the writeback stage into rd.
    // No return value, modify core as a pointer.

}

// helper: which opcodes write a destination register (rd)
static bool opcode_writes_rd(Opcode op) {
    switch (op) {
    case OP_ADD: case OP_SUB: case OP_AND: case OP_OR:
    case OP_XOR: case OP_MUL:
    case OP_SLL: case OP_SRA: case OP_SRL:
    case OP_LW:
        return true;
    default:
        return false;
    }
}

void decode_stage(Core * core){
    // The decode function should handle:
    // Decoding the instruction (duh):
    //   It is going to get the instruction in 
    //   core->pipe.decode.inst, and only the 
    //   core->pipe.decode.inst.binary_value field
    //   will be filled. You must make sure to fill the others.
    // Check for branches:
    //   Check for branches and update the PC if applicable.
    // Handle data hazards:
    //   If there is a hazard, this instruction must pass on a bubble.
    
    // Bonus points if you add a debug print for when register 0 will be
    // written to, because that should not happen in normal code. Please 
    // use the DEBUG_PRINT() macro if you do...

    if (core == NULL) return; //first check the the Core pointer works
    if (core->pipe.decode.active == 0) return; //Now check if were halted or bubbled

    // Perform the stage
    Instruction* inst = &core->pipe.decode.inst;
    uint32_t binary_raw = inst->binary_value;

    inst->opcode = (Opcode)((binary_raw >> 24) & 0xFF);
    inst->rd = (uint8_t)((binary_raw >> 20) & 0xF);
    inst->rs = (uint8_t)((binary_raw >> 16) & 0xF);
    inst->rt = (uint8_t)((binary_raw >> 12) & 0xF);
    uint16_t addr = (core->regs[core->pipe.execute.inst.rd]) & 0b1111111111; //R[rd][low bits 9:0]

    // sign-extend 12-bit immediate
    int32_t imm12 = binary_raw & 0xFFF;
    if (imm12 & 0x800) imm12 |= 0xFFFFF000;
    inst->imm = imm12;

    // Register values
    int32_t rs_val = core->regs[inst->rs];
    int32_t rt_val = core->regs[inst->rt];
    // rd_val is the register number (0..15)

    // Writing to register 0 should not happen in normal code.
    if (inst->rd == 0 && opcode_writes_rd(inst->opcode)) {
        DEBUG_PRINT("Warning: instruction attempts to write R0 (ignored)\n");
        core->pipe.decode.active = false;
        return;
    }

    // ---------- Data hazard detection ----------
    // If any previous instruction in Execute/Mem/WB will write to a register that this
    // instruction reads (rs or rt), then stall.
    // and keep Decode active (so the instruction stays in decode for the next cycle).
    bool hazard = false;
    PipelineStage* stages_to_check[3] = { &core->pipe.execute, &core->pipe.mem, &core->pipe.wb };
    PipelineStage* s = stages_to_check[0];
    for (int i = 0; i < 3 && !hazard; ++i) {
        s = stages_to_check[i];
        if (s->active) {
            uint8_t prev_rd = s->inst.rd;
            if (prev_rd != 0 && opcode_writes_rd(s->inst.opcode)) {
                if (prev_rd == inst->rs || prev_rd == inst->rt) {
                    hazard = true;
                }
            }
        }
    }

    if (hazard) {
        core->pipe.execute.active = false;
        core->stats.decode_stall += 1;
        // Do NOT advance PC here (fetch/decode will be retried next cycle)
        return;
    }


    switch (inst->opcode) {
    // ----- Branches  ----- 

    case OP_BEQ:
        if (rs_val == rt_val) core->pc = addr;
        break;
    case OP_BNE:
        if (rs_val != rt_val) core->pc = addr;
        break;
    case OP_BLT:
        if (rs_val < rt_val) core->pc = addr;
        break;
    case OP_BGT:
        if (rs_val > rt_val) core->pc = addr;
        break;
    case OP_BLE:
        if (rs_val <= rt_val) core->pc = addr;
        break;
    case OP_BGE:
        if (rs_val >= rt_val) core->pc = addr;
        break;
    case OP_JAL:
        // Save next PC in R15
        core->regs[15] = core->pc;
        core->pc = addr;
        break;

    default:
        break;
    }

    return;
}
