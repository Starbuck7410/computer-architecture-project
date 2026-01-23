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

// For hazard detection we need to know the actual destination register.
// JAL writes to R15 (link), while arithmetic/lw write to RD.
static bool stage_writes_reg(const PipelineStage* st, uint8_t* out_dst) {
    if (!st || !st->active) return false;
    if (st->inst.opcode == OP_JAL) {
        *out_dst = 15;
        return true;
    }
    if (opcode_writes_rd(st->inst.opcode)) {
        *out_dst = st->inst.rd;
        return true;
    }
    return false;
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
    // SW uses RD as the value-to-store.
    // Branches/JAL use RD as the target PC register.
    switch (op) {
        case OP_SW:
        case OP_BEQ: case OP_BNE: case OP_BLT: case OP_BGT: case OP_BLE: case OP_BGE:
        case OP_JAL:
            return true;
        default:
            return false;
    }
}

void execute_stage(Core * core){
    if (core == NULL) return;
    if (core->pipe.execute.active == 0) return;

    Instruction *inst = &core->pipe.execute.inst;
     int32_t rs_val = (inst->rs != 1) ? core->regs[inst->rs] : inst->imm;
    int32_t rt_val = (inst->rt != 1) ? core->regs[inst->rt] : inst->imm;
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
        case OP_JAL:
            // Link value is the next sequential instruction address (10-bit PC)
            results = (int32_t)((core->pipe.execute.pc + 1) & 0x3FF);
            break;
        case OP_LW:
        case OP_SW:
             // Calc effective address: rs + rt
             results = rs_val + rt_val;
             break;
        default: break;
    }
    core->pipe.execute.result = results;
}

void decode_stage(Core * core){
    if (core == NULL) return;
    if (core->pipe.decode.active == 0) return;

    // Important: don't "stick" forever. Re-evaluate hazards every cycle.
    core->pipe.decode.stall = false;

    Instruction * inst = &core->pipe.decode.inst;
    uint32_t binary_raw = inst->binary_value;

    inst->opcode = (Opcode)((binary_raw >> 24) & 0xFF);
    inst->rd = (uint8_t)((binary_raw >> 20) & 0xF);
    inst->rs = (uint8_t)((binary_raw >> 16) & 0xF);
    inst->rt = (uint8_t)((binary_raw >> 12) & 0xF);
    
    int32_t imm12 = binary_raw & 0xFFF;
    if (imm12 & 0x800) imm12 |= 0xFFFFF000;
    inst->imm = imm12;

    // R0 is hard-wired to 0, and R1 is the sign-extended immediate of the
    // instruction in DECODE
    core->pending_imm_write = true;
    core->pending_imm_value = inst->imm;

    int32_t rs_val = (inst->rs != 1) ? core->regs[inst->rs] : inst->imm;
    int32_t rt_val = (inst->rt != 1) ? core->regs[inst->rt] : inst->imm;
    int32_t rd_val = (inst->rd != 1) ? core->regs[inst->rd] : inst->imm;

    // --- HAZARD DETECTION (RAW) ---
    bool hazard = false;
    // No forwarding, and register writes are only visible on the NEXT cycle.
    // Therefore, we must stall if the needed source reg is being written by
    // an instruction currently in EXEC, MEM, or WB.
    PipelineStage * stages[3] = { &core->pipe.execute, &core->pipe.mem, &core->pipe.wb };
    
    for (int i = 0; i < 3; ++i) {
        uint8_t dest_reg = 0;
        if (stage_writes_reg(stages[i], &dest_reg)) {
            if (dest_reg == 0 || dest_reg == 1) continue; // R0 is 0, R1 is immediate-only

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

    // Branch / Jump Handling (branch resolution in DECODE, with 1 delay-slot)
    bool taken = false;
    uint32_t target = (uint32_t)rd_val & 0x3FF;
    
    switch (inst->opcode) {
        case OP_BEQ: if (rs_val == rt_val) taken = true; break;
        case OP_BNE: if (rs_val != rt_val) taken = true; break;
        case OP_BLT: if (rs_val < rt_val) taken = true; break;
        case OP_BGT: if (rs_val > rt_val) taken = true; break;
        case OP_BLE: if (rs_val <= rt_val) taken = true; break;
        case OP_BGE: if (rs_val >= rt_val) taken = true; break;
        case OP_JAL:
            taken = true;
            break;
        default: break;
    }
    if (taken) {
        // Delay slot: the instruction currently in FETCH must still execute.
        // We therefore redirect the PC only AFTER the current FETCH happens.
        core->pc_redirect_valid = true;
        core->pc_redirect = target;
    }

    // HALT stops fetching new instructions, but we still let the pipeline drain.
    if (inst->opcode == OP_HALT) {
        core->stop_fetch = true;
        // Any already-fetched instruction after HALT should not execute.
        core->pipe.fetch.active = false;
    }
}

void fetch_stage(Core * core){
    if (core == NULL) return;
    // Once HALT was decoded, we stop fetching, but the pipeline can still drain.
    if (core->halted || core->stop_fetch) return;
    
    // If stalled, we cannot fetch new instructions
    if (core->pipe.decode.stall || core->pipe.mem.stall) return;

    uint32_t pc = core->pc & 0x3FF;
    core->pipe.fetch.inst.binary_value = core->imem[pc];
    core->pipe.fetch.pc = pc;
    core->pipe.fetch.active = true;
    
    core->pc = (pc + 1) & 0x3FF;

    // Apply branch/jump redirect after fetching the delay-slot instruction.
    if (core->pc_redirect_valid) {
        core->pc = core->pc_redirect & 0x3FF;
        core->pc_redirect_valid = false;
    }
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
                    // Miss was already counted when we first detected it.
                    success = true;
                } else {
                     // Still missed (rare, maybe evicted by snoop?), retry bus
                     send_bus_read_request(core, addr, false);
                }
            } else if (core->pipe.mem.inst.opcode == OP_SW) {
                uint32_t data = core->regs[core->pipe.mem.inst.rd];
                if (write_word_to_cache(core, addr, data)) {
                    // Miss was already counted when we first detected it.
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
            core->stats.read_misses++;
            send_bus_read_request(core, addr, false);
            core->pipe.mem.stall = true;
        }
    } else if (op == OP_SW) {
        uint32_t val = core->regs[core->pipe.mem.inst.rd];
        if (!write_word_to_cache(core, addr, val)) {
            core->stats.write_misses++;
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

    // Commit register writes on the clock edge (handled in main.c).
    if (inst->opcode == OP_JAL) {
        core->pending_reg_write = true;
        core->pending_reg_dst = 15;
        core->pending_reg_value = core->pipe.wb.result;
        return;
    }

    if (opcode_writes_rd(inst->opcode)) {
        core->pending_reg_write = true;
        core->pending_reg_dst = inst->rd;
        core->pending_reg_value = core->pipe.wb.result;
    }
}