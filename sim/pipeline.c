#include "pipeline.h"
#include "memory.h"
#include "bus.h"

// Helper: Does this opcode WRITE to register RD?
static bool opcode_writes_rd(Opcode op) {
    switch (op) {
        case OP_ADD: case OP_SUB: case OP_AND: case OP_OR:
        case OP_XOR: case OP_MUL: case OP_SLL: case OP_SRA: 
        case OP_SRL: case OP_LW: 
            return true;
        default: 
            return false;
    }
}

// Helper: Does this opcode READ from register RS?
static bool opcode_reads_rs(Opcode op) {
    // Almost all ops use RS as a source (Arithmetic, Branch, Load, Store)
    // JAL and HALT do not.
    if (op == OP_JAL || op == OP_HALT) return false;
    return true;
}

// Helper: Does this opcode READ from register RT?
static bool opcode_reads_rt(Opcode op) {
    switch (op) {
        // R-Type Arithmetic
        case OP_ADD: case OP_SUB: case OP_AND: case OP_OR:
        case OP_XOR: case OP_MUL: case OP_SLL: case OP_SRA: case OP_SRL:
        // Branches compare RS and RT
        case OP_BEQ: case OP_BNE: case OP_BLT: case OP_BGT: case OP_BLE: case OP_BGE:
            return true;
        // LW uses RT as Destination (Write), not Source (Read) -> False
        // SW uses RD as Data Source, RS as Addr. RT is unused -> False
        default: 
            return false;
    }
}

// Helper: Does this opcode READ from register RD? (Special case for SW)
static bool opcode_reads_rd(Opcode op) {
    // Your SW implementation uses RD as the value to store.
    return (op == OP_SW);
}

void execute_stage(Core * core){
    if (core == NULL) return;
    if (core->pipe.execute.active == 0) return;

    Instruction *inst = &core->pipe.execute.inst;
    int32_t rt_val = core->regs[inst->rt];
    int32_t rs_val = core->regs[inst->rs];
    int32_t results = 0;

    switch (inst->opcode) {
        case OP_ADD: results = rs_val + rt_val; break;
        case OP_SUB: results = rs_val - rt_val; break;
        case OP_AND: results = rs_val & rt_val; break;
        case OP_OR:  results = rs_val | rt_val; break;
        case OP_XOR: results = rs_val ^ rt_val; break;
        case OP_MUL: results = rs_val * rt_val; break;
        case OP_SLL: results = rs_val << rt_val; break;
        case OP_SRA: results = rs_val >> rt_val; break;
        case OP_SRL: results = (int32_t)((uint32_t)rs_val >> rt_val); break;
        case OP_LW:
        case OP_SW:
             // Calc effective address: rs + imm
             results = rs_val + inst->imm;
             break;
        default: break;
    }
    core->pipe.execute.result = results;
}

void decode_stage(Core * core){
    if (core == NULL) return;
    if (core->pipe.decode.active == 0) return;
    if (core->pipe.decode.stall) return; 

    Instruction * inst = &core->pipe.decode.inst;
    uint32_t binary_raw = inst->binary_value;

    inst->opcode = (Opcode)((binary_raw >> 24) & 0xFF);
    inst->rd = (uint8_t)((binary_raw >> 20) & 0xF);
    inst->rs = (uint8_t)((binary_raw >> 16) & 0xF);
    inst->rt = (uint8_t)((binary_raw >> 12) & 0xF);
    
    int32_t imm12 = binary_raw & 0xFFF;
    if (imm12 & 0x800) imm12 |= 0xFFFFF000;
    inst->imm = imm12;

    int32_t rs_val = core->regs[inst->rs];
    int32_t rt_val = core->regs[inst->rt];

    // --- HAZARD DETECTION (RAW) ---
    bool hazard = false;
    // We check Execute, Mem, and WB stages for instructions writing to registers we need
    PipelineStage * stages[3] = { &core->pipe.execute, &core->pipe.mem, &core->pipe.wb };
    
    for (int i = 0; i < 3; ++i) {
        if (stages[i]->active && opcode_writes_rd(stages[i]->inst.opcode)) {
            uint8_t dest_reg = stages[i]->inst.rd;
            if (dest_reg == 0) continue; // Writing to R0 is effectively no-op

            // Check specific source registers required by current opcode
            if (opcode_reads_rs(inst->opcode) && inst->rs == dest_reg) hazard = true;
            if (opcode_reads_rt(inst->opcode) && inst->rt == dest_reg) hazard = true;
            if (opcode_reads_rd(inst->opcode) && inst->rd == dest_reg) hazard = true;
        }
    }

    if (hazard) {
        core->pipe.decode.stall = true;
        core->stats.decode_stall++;
        return; // STALL!
    }

    // Branch / Jump Handling
    bool taken = false;
    uint32_t target = inst->imm & 0x3FF; 

    switch (inst->opcode) {
        case OP_BEQ: if (rs_val == rt_val) taken = true; break;
        case OP_BNE: if (rs_val != rt_val) taken = true; break;
        case OP_BLT: if (rs_val < rt_val) taken = true; break;
        case OP_BGT: if (rs_val > rt_val) taken = true; break;
        case OP_BLE: if (rs_val <= rt_val) taken = true; break;
        case OP_BGE: if (rs_val >= rt_val) taken = true; break;
        case OP_JAL:
            // JAL writes Link address to R15 immediately (Sim simplification)
            core->regs[15] = core->pc; 
            taken = true;
            break;
        default: break;
    }

    if (taken) {
        core->pc = target;
        // Flush Fetch (the instruction currently in fetch is wrong)
        core->pipe.fetch.active = false;
    }
}

void fetch_stage(Core * core){
    if (core == NULL || core->halted) return;
    
    // If stalled, we cannot fetch new instructions
    if (core->pipe.decode.stall || core->pipe.mem.stall) return;

    uint32_t pc = core->pc & 0x3FF;
    core->pipe.fetch.inst.binary_value = core->imem[pc];
    core->pipe.fetch.pc = pc;
    core->pipe.fetch.active = true;
    
    core->pc = (pc + 1) & 0x3FF;
}

void memory_stage(Core * core){
    if (core == NULL) return;
    
    // 1. Resolve Existing Stall
    if (core->pipe.mem.stall) {
        if (core->bus_interface.request_done) {
            core->bus_interface.request_done = false;
            
            // Retry the operation
            uint32_t addr = core->pipe.mem.result;
            bool success = false;
            
            if (core->pipe.mem.inst.opcode == OP_LW) {
                if (is_cache_hit(&core->cache, addr)) {
                    core->pipe.mem.result = read_word_from_cache(&core->cache, addr);
                    core->stats.read_misses++; 
                    success = true;
                } else {
                     // Still missed (rare, maybe evicted by snoop?), retry bus
                     send_bus_read_request(core, addr, false);
                }
            } else if (core->pipe.mem.inst.opcode == OP_SW) {
                uint32_t data = core->regs[core->pipe.mem.inst.rd];
                if (write_word_to_cache(core, addr, data)) {
                    core->stats.write_misses++;
                    success = true;
                }
                // If false, write_word sent a new upgrade request automatically
            }

            if (success) {
                core->pipe.mem.stall = false;
            }
        }
        return; 
    }

    // 2. Normal Execution
    if (!core->pipe.mem.active) return;

    Opcode op = core->pipe.mem.inst.opcode;
    uint32_t addr = core->pipe.mem.result; 
    
    if (op == OP_LW) {
        if (is_cache_hit(&core->cache, addr)) {
            core->pipe.mem.result = read_word_from_cache(&core->cache, addr);
            core->stats.read_hits++;
        } else {
            send_bus_read_request(core, addr, false);
            core->pipe.mem.stall = true;
        }
    } else if (op == OP_SW) {
        uint32_t val = core->regs[core->pipe.mem.inst.rd];
        if (!write_word_to_cache(core, addr, val)) {
            core->pipe.mem.stall = true; // Stall for ownership/miss
        } else {
            core->stats.write_hits++;
        }
    }
}

void writeback_stage(Core* core) {
    if (core == NULL || !core->pipe.wb.active) return;

    Instruction* inst = &core->pipe.wb.inst;
    core->stats.instructions++;

    if (inst->opcode == OP_HALT) {
        core->halted = true;
        return;
    }

    if (opcode_writes_rd(inst->opcode)) {
        if (inst->rd != 0) {
            core->regs[inst->rd] = core->pipe.wb.result;
        }
    }
}