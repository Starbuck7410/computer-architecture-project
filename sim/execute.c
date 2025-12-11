#include "general_utils.h"


void execute_stage(Core * core){
    // Is core halted? Is pipeline stalled? 
    // Is instruction not even using the ALU?
    // SKIP.
    // or else...

    // Perform the stage
    if (core->pipe.execute.inst.opcode == OP_ADD){
        // Do shit
    }
    // Reminders:
    // register number is at core->pipe.execute.inst.rx
    // register are at core->reg_file.regs[reg]; READ ONLY!
    // core->pipe.execute.result = result;
    // This will go in the writeback stage into rd.
    // No return value, modify core as a pointer.

}