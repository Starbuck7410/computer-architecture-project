#include "pipeline.h"
#include "memory.h"
#include "bus.h"

// Helper: Does this opcode_T WRITE to register RD?
static bool opcode_writes_rd(opcode_T op) {
    switch (op) {
        case OPCODE_ADD: case OPCODE_SUB: case OPCODE_AND: case OPCODE_OR:
        case OPCODE_XOR: case OPCODE_MUL: case OPCODE_SLL: case OPCODE_SRA: 
        case OPCODE_SRL: case OPCODE_LW: 
            return true;
        default: 
            return false;
    }
}

// For hazard detection we need to know the actual destination register.
// JAL writes to R15 (link), while arithmetic/lw write to RD.
static bool stage_writes_reg(const pipeline_stage_T* st, uint8_t* out_dst) {
    if (!st || !st->active) return false;
    if (st->instruction.opcode == OPCODE_JAL) {
        *out_dst = 15;
        return true;
    }
    if (opcode_writes_rd(st->instruction.opcode)) {
        *out_dst = st->instruction.rd;
        return true;
    }
    return false;
}

// Helper: Does this opcode_T READ from register RS?
static bool opcode_reads_rs(opcode_T op) {
    // Almost all ops use RS as a source (Arithmetic, Branch, Load, Store)
    // JAL and HALT do not.
    if (op == OPCODE_JAL || op == OPCODE_HALT) return false;
    return true;
}

// Helper: Does this opcode_T READ from register RT?
static bool opcode_reads_rt(opcode_T op) {
    switch (op) {
        // R-Type Arithmetic
        case OPCODE_ADD: case OPCODE_SUB: case OPCODE_AND: case OPCODE_OR:
        case OPCODE_XOR: case OPCODE_MUL: case OPCODE_SLL: case OPCODE_SRA: case OPCODE_SRL:
        // Branches compare RS and RT
        case OPCODE_BEQ: case OPCODE_BNE: case OPCODE_BLT: case OPCODE_BGT: case OPCODE_BLE: case OPCODE_BGE:
            return true;
        // LW uses RT as Destination (Write), not Source (Read) -> False
        // SW uses RD as Data Source, RS as Addr. RT is unused -> False
        default: 
            return false;
    }
}

// Helper: Does this opcode_T READ from register RD? (Special case for SW)
static bool opcode_reads_rd(opcode_T op) {
    // SW uses RD as the value-to-store.
    // Branches/JAL use RD as the target PC register.
    switch (op) {
        case OPCODE_SW:
        case OPCODE_BEQ: case OPCODE_BNE: case OPCODE_BLT: case OPCODE_BGT: case OPCODE_BLE: case OPCODE_BGE:
        case OPCODE_JAL:
            return true;
        default:
            return false;
    }
}

void execute(core_T * core){
    if (core == NULL) return;
    if (core->pipe.execute.active == 0) return;

    instruction_T *instruction = &core->pipe.execute.instruction;
    int32_t rs_val = (instruction->rs != 1) ? core->regs[instruction->rs] : instruction->imm;
    int32_t rt_val = (instruction->rt != 1) ? core->regs[instruction->rt] : instruction->imm;
    int32_t results = 0;

    switch (instruction->opcode) {
        case OPCODE_ADD: results = rs_val + rt_val; break;
        case OPCODE_SUB: results = rs_val - rt_val; break;
        case OPCODE_AND: results = rs_val & rt_val; break;
        case OPCODE_OR:  results = rs_val | rt_val; break;
        case OPCODE_XOR: results = rs_val ^ rt_val; break;
        case OPCODE_MUL: results = rs_val * rt_val; break;
        case OPCODE_SLL: results = rs_val << rt_val; break;
        case OPCODE_SRA: results = rs_val >> rt_val; break;
        case OPCODE_SRL: results = (int32_t)((uint32_t)rs_val >> rt_val); break;
        case OPCODE_JAL:
            // Link value is the next sequential instruction address (10-bit PC)
            results = (int32_t)((core->pipe.execute.pc + 1) & 0x3FF);
            break;
        case OPCODE_LW:
        case OPCODE_SW:
             // Calc effective address: rs + rt
             results = rs_val + rt_val;
             break;
        default: break;
    }
    core->pipe.execute.result = results;
}

void decode(core_T * core){
    if (core == NULL) return;
    if (core->pipe.decode.active == 0) return;

    // Important: don't "stick" forever. Re-evaluate hazards every cycle.
    core->pipe.decode.stall = false;

    instruction_T * instruction = &core->pipe.decode.instruction;
    uint32_t binary_raw = instruction->binary_value;
    instruction->opcode = (opcode_T)((binary_raw >> 24) & 0xFF);
    instruction->rd = (uint8_t)((binary_raw >> 20) & 0xF);
    instruction->rs = (uint8_t)((binary_raw >> 16) & 0xF);
    instruction->rt = (uint8_t)((binary_raw >> 12) & 0xF);
    
    int32_t imm12 = binary_raw & 0xFFF;
    if (imm12 & 0x800) imm12 |= 0xFFFFF000;
    instruction->imm = imm12;

    // R0 is hard-wired to 0, and R1 is the sign-extended immediate of the
    // instruction in DECODE
    core->pending_imm_write = true;
    core->pending_imm_value = instruction->imm;

    int32_t rs_val = (instruction->rs != 1) ? core->regs[instruction->rs] : instruction->imm;
    int32_t rt_val = (instruction->rt != 1) ? core->regs[instruction->rt] : instruction->imm;
    int32_t rd_val = (instruction->rd != 1) ? core->regs[instruction->rd] : instruction->imm;

    // --- HAZARD DETECTION (RAW) ---
    bool hazard = false;
    // No forwarding, and register writes are only visible on the NEXT cycle.
    // Therefore, we must stall if the needed source reg is being written by
    // an instruction currently in EXEC, MEM, or WB.
    pipeline_stage_T * stages[3] = { &core->pipe.execute, &core->pipe.memory, &core->pipe.writeback };
    
    for (int i = 0; i < 3; ++i) {
        uint8_t dest_reg = 0;
        if (stage_writes_reg(stages[i], &dest_reg)) {
            if (dest_reg == 0 || dest_reg == 1) continue; // R0 is 0, R1 is immediate-only

            // Check specific source registers required by current opcode_T
            if (opcode_reads_rs(instruction->opcode) && instruction->rs == dest_reg) hazard = true;
            if (opcode_reads_rt(instruction->opcode) && instruction->rt == dest_reg) hazard = true;
            if (opcode_reads_rd(instruction->opcode) && instruction->rd == dest_reg) hazard = true;
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
    
    switch (instruction->opcode) {
        case OPCODE_BEQ: if (rs_val == rt_val) taken = true; break;
        case OPCODE_BNE: if (rs_val != rt_val) taken = true; break;
        case OPCODE_BLT: if (rs_val < rt_val) taken = true; break;
        case OPCODE_BGT: if (rs_val > rt_val) taken = true; break;
        case OPCODE_BLE: if (rs_val <= rt_val) taken = true; break;
        case OPCODE_BGE: if (rs_val >= rt_val) taken = true; break;
        case OPCODE_JAL:
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
    if (instruction->opcode == OPCODE_HALT) {
        core->stop_fetch = true;
        // Any already-fetched instruction after HALT should not execute.
        core->pipe.fetch.active = false;
    }
}

void fetch(core_T * core){
    if (core == NULL) return;
    // Once HALT was decoded, we stop fetching, but the pipeline can still drain.
    if (core->halted || core->stop_fetch) return;
    
    // If stalled, we cannot fetch new instructions
    if (core->pipe.decode.stall || core->pipe.memory.stall) return;

    uint32_t pc = core->pc & 0x3FF;
    core->pipe.fetch.instruction.binary_value = core->imem[pc];
    core->pipe.fetch.pc = pc;
    core->pipe.fetch.active = true;
    
    core->pc = (pc + 1) & 0x3FF;

    // Apply branch/jump redirect after fetching the delay-slot instruction.
    if (core->pc_redirect_valid) {
        core->pc = core->pc_redirect & 0x3FF;
        core->pc_redirect_valid = false;
    }
}

void memory(core_T * core){
    if (core == NULL) return;
    
    // Resolve Existing Stall
    if (core->pipe.memory.stall) {
        if (core->bus_interface.request_done) {
            core->bus_interface.request_done = false;
            
            // Retry the operation
            uint32_t addr = core->pipe.memory.result;
            bool success = false;
            
            if (core->pipe.memory.instruction.opcode == OPCODE_LW) {
                if (is_cache_hit(&core->cache, addr)) {
                    core->pipe.memory.result = read_word_from_cache(&core->cache, addr);
                    // Miss was already counted when we first detected it.
                    success = true;
                } else {
                     // Still missed (rare, maybe evicted by snoop?), retry bus
                     send_bus_read_request(core, addr, false);
                }
            } else if (core->pipe.memory.instruction.opcode == OPCODE_SW) {
                uint32_t data = core->regs[core->pipe.memory.instruction.rd];
                if (write_word_to_cache(core, addr, data)) {
                    // Miss was already counted when we first detected it.
                    success = true;
                }
                // If false, write_word sent a new upgrade request automatically
            }

            if (success) {
                core->pipe.memory.stall = false;
            }
        }
        return; 
    }

    // Normal Execution
    if (!core->pipe.memory.active) return;

    opcode_T op = core->pipe.memory.instruction.opcode;
    uint32_t addr = core->pipe.memory.result; 
    
    if (op == OPCODE_LW) {
        if (is_cache_hit(&core->cache, addr)) {
            core->pipe.memory.result = read_word_from_cache(&core->cache, addr);
            core->stats.read_hits++;
        } else {
            core->stats.read_misses++;
            send_bus_read_request(core, addr, false);
            core->pipe.memory.stall = true;
        }
    } else if (op == OPCODE_SW) {
        uint32_t val = core->regs[core->pipe.memory.instruction.rd];
        if (!write_word_to_cache(core, addr, val)) {
            core->stats.write_misses++;
            core->pipe.memory.stall = true; // Stall for ownership/miss
        } else {
            core->stats.write_hits++;
        }
    }
}

void writeback(core_T * core) {
    if (core == NULL || !core->pipe.writeback.active) return;

    instruction_T* instruction = &core->pipe.writeback.instruction;
    core->stats.instructions++;

    if (instruction->opcode == OPCODE_HALT) {
        core->halted = true;
        return;
    }

    // Commit register writes on the clock edge (handled in main.c).
    if (instruction->opcode == OPCODE_JAL) {
        core->pending_reg_write = true;
        core->pending_reg_dst = 15;
        core->pending_reg_value = core->pipe.writeback.result;
        return;
    }

    if (opcode_writes_rd(instruction->opcode)) {
        core->pending_reg_write = true;
        core->pending_reg_dst = instruction->rd;
        core->pending_reg_value = core->pipe.writeback.result;
    }
}