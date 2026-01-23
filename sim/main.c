#include "general_utils.h"
#include "file_io.h"
#include "pipeline.h"
#include "bus.h"
#include <stdlib.h>

SystemBus system_bus;

static bool pipeline_empty(const Core* c) {
    return !c->pipe.fetch.active && !c->pipe.decode.active && !c->pipe.execute.active &&
           !c->pipe.mem.active && !c->pipe.wb.active;
}

// Commit register writes on the clock edge (end of cycle)
static void commit_register_writes(Core* c) {
    if (c == NULL) return;

    // R0 is hard-wired to 0
    c->regs[0] = 0;

    // R1: immediate register (only updated from decode)
    if (c->pending_imm_write) {
        c->regs[1] = c->pending_imm_value;
        c->pending_imm_write = false;
    }

    // General reg write (from WB)
    if (c->pending_reg_write) {
        uint8_t dst = c->pending_reg_dst;
        if (dst != 0 && dst != 1) {
            c->regs[dst] = c->pending_reg_value;
        }
        c->pending_reg_write = false;
    }

    // Keep invariants
    c->regs[0] = 0;
}

// Helper to advance pipeline stages (latching)
void update_pipeline_stages(Core * core) {
    if (core == NULL) return;

    // Pipeline register update happens on the clock edge.
    // Use temporaries so we don't accidentally reuse the same instruction twice.
    PipelineStage next_wb     = core->pipe.wb;
    PipelineStage next_mem    = core->pipe.mem;
    PipelineStage next_exec   = core->pipe.execute;
    PipelineStage next_decode = core->pipe.decode;
    PipelineStage next_fetch  = core->pipe.fetch;

    if (core->pipe.mem.stall) {
        // Whole pipeline is effectively stalled behind MEM while waiting on the bus.
        // Nothing advances this cycle (except we count the stall).
        core->stats.mem_stall++;
        return;
    }

    // WB always takes MEM, MEM takes EXEC.
    next_wb  = core->pipe.mem;
    next_mem = core->pipe.execute;

    if (core->pipe.decode.stall) {
        // Insert bubble into EXEC, keep DECODE and FETCH holding their current instructions.
        next_exec.active = false;
    } else {
        // Normal flow
        next_exec   = core->pipe.decode;
        next_decode = core->pipe.fetch;

        // Critical: once FETCH is consumed into DECODE, clear FETCH so we don't
        // "re-inject" the same instruction if fetch_stage() is blocked next cycle.
        next_fetch.active = false;
    }

    core->pipe.wb     = next_wb;
    core->pipe.mem    = next_mem;
    core->pipe.execute= next_exec;
    core->pipe.decode = next_decode;
    core->pipe.fetch  = next_fetch;
}

int main(int argc, char ** argv){
    SimFiles sim_files;
    get_arguments(argc, argv, &sim_files);

    // 1. Initialize System Memory
    system_bus.system_memory = (uint32_t*)calloc(MEMIN_DEPTH, sizeof(uint32_t));
    read_mainmem(&sim_files, system_bus.system_memory);

    // 2. Initialize Cores
    Core * cores[CORE_COUNT];
    for(int i = 0; i < CORE_COUNT; i++) {
        cores[i] = (Core*)calloc(1, sizeof(Core));
        cores[i]->id = i;
        cores[i]->pc = 0;
        system_bus.bus_interface[i] = &cores[i]->bus_interface;
    }

    // Link caches/interfaces to the shared system bus (after all cores exist)
    init_bus(cores);

    read_imem(&sim_files, cores);

    // Truncate per-cycle trace outputs (we append during the run)
    {
        FILE *fp = fopen(sim_files.bustrace, "w");
        if (fp) fclose(fp);
        for (int i = 0; i < CORE_COUNT; i++) {
            fp = fopen(sim_files.trace[i], "w");
            if (fp) fclose(fp);
        }
    }

    int cycle = 0;
    bool active = true;

    while(active){
        bool all_done = true;

        // 1. Run Pipeline Stages (Hardware Parallelism)
        for(int i = 0; i < CORE_COUNT; i++){
            // Run stages while there is still pipeline activity to drain.
            if(!cores[i]->halted || !pipeline_empty(cores[i])) {
                all_done = false;
                
                // Stages usually run "Reverse" in software sim to avoid 
                // data racing within the cycle, but here we use a latching 
                // function at the end, so order matters less, except for forwarding.
                
                writeback_stage(cores[i]);
                memory_stage(cores[i]);
                execute_stage(cores[i]);
                decode_stage(cores[i]);
                fetch_stage(cores[i]);
            }
        }

        if(all_done) break;

        // 2. Bus Arbitration & Transaction
        bus_handler();

        // 3. Logging
        log_core_trace(&sim_files, cores, cycle);
        log_bus_trace(&sim_files, cycle);

        // 4. Advance Pipeline (Clock Edge)
        for(int i = 0; i < CORE_COUNT; i++){
            // Clock edge: advance pipeline, then commit register file updates.
            update_pipeline_stages(cores[i]);
            commit_register_writes(cores[i]);
            // Count cycles until the core reaches HALT (as defined in the spec)
            if (!cores[i]->halted) {
                cores[i]->stats.cycles++;
            }
        }
        
        cycle++;
        // Safety break for infinite loops
        if(cycle > 500000) { 
            printf("Timeout reached\n"); 
            break; 
        }
    }

    write_outputs(&sim_files, cores, system_bus.system_memory);

    // Cleanup
    free(system_bus.system_memory);
    for(int i=0; i<CORE_COUNT; i++) free(cores[i]);

    return 0;
}