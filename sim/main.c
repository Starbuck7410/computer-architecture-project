#include "general_utils.h"
#include "file_io.h"
#include "pipeline.h"
#include "bus.h"
#include <stdlib.h>

SystemBus system_bus;

// Helper to advance pipeline stages (latching)
void update_pipeline_stages(Core * core) {
    if (core->halted) return;

    // 1. Writeback is always ready to accept from Mem (unless halted, handled in stage)
    // 2. Mem accepts from Execute ONLY if Mem is not stalled by cache miss
    if (!core->pipe.mem.stall) {
        core->pipe.wb = core->pipe.mem;
        core->pipe.mem = core->pipe.execute;
        core->pipe.execute = core->pipe.decode;
        
        // If Decode was stalled (data hazard), we don't fetch new, we keep decode
        if (core->pipe.decode.stall) {
             // If decode was stalled, we insert a bubble into execute
             core->pipe.execute.active = false;
             // We do NOT pull from fetch
             core->pipe.decode.stall = false; // Reset flag for next check
        } else {
             core->pipe.decode = core->pipe.fetch;
             // Fetch stage is refilled by fetch_stage() next cycle
        }
    } else {
        // Memory stage is stalled (waiting for Bus).
        // WB becomes a bubble (nothing new finishes)
        core->pipe.wb.active = false; 
        // MEM stays as is. EXEC stays as is. DEC stays as is. FETCH stays as is.
        core->stats.mem_stall++;
    }
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
        init_bus(cores); // Link caches to bus
    }

    read_imem(&sim_files, cores);

    int cycle = 0;
    bool active = true;

    while(active){
        bool all_halted = true;

        // 1. Run Pipeline Stages (Hardware Parallelism)
        for(int i = 0; i < CORE_COUNT; i++){
            if(!cores[i]->halted) {
                all_halted = false;
                
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

        if(all_halted) break;

        // 2. Bus Arbitration & Transaction
        bus_handler();

        // 3. Logging
        log_core_trace(&sim_files, cores, cycle);
        log_bus_trace(&sim_files, cycle);

        // 4. Advance Pipeline (Clock Edge)
        for(int i = 0; i < CORE_COUNT; i++){
            update_pipeline_stages(cores[i]);
            cores[i]->stats.cycles++;
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