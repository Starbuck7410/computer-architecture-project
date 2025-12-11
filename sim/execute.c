#include "general_utils.h"


void execute_stage(Core * core){
    // Is core halted? Is pipeline stalled? 
    // Is instruction not even using the ALU?
    // SKIP.
    // or else...
    if (core == NULL) return; //first check the the Core pointer works
    if (core->pipe.execute.active == 0) return; //Now check if were halted or bubbled
    // Perform the stage
    Instruction *inst = &core->pipe.execute.inst;
    int32_t rt_val = core->reg_file.regs[core->pipe.execute.inst.rt];
    int32_t rs_val = core->reg_file.regs[core->pipe.execute.inst.rs];
    int32_t results = core->pipe.execute.result;
    uint16_t addr = (core->reg_file.regs[core->pipe.execute.inst.rd]) & 0b1111111111; //R[rd][low bits 9:0]
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
        // ----- Branches (move to decode) ----- 
        case OP_BEQ:
            if (rs_val == rt_val) results = addr;
            break;
        case OP_BNE:
            if (rs_val != rt_val) results = addr;
            break;
        case OP_BLT:
            if (rs_val < rt_val) results = addr;
            break;
        case OP_BGT:
            if (rs_val > rt_val) results = addr;
            break;
        case OP_BLE:
            if (rs_val <= rt_val) results = addr;
            break;
        case OP_BGE:
            if (rs_val >= rt_val) results = addr;
            break;          
        default:
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
}