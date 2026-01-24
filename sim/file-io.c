#include "file-io.h"

void get_arguments(int argc, char* argv[], files_T* files) {
    // defaults
    if (argc < 28) {
        files->imem[0] = "imem0.txt";
        files->imem[1] = "imem1.txt";
        files->imem[2] = "imem2.txt";
        files->imem[3] = "imem3.txt";
        files->memin = "memin.txt";
        files->memout = "memout.txt";
        files->regout[0] = "regout0.txt";
        files->regout[1] = "regout1.txt";
        files->regout[2] = "regout2.txt";
        files->regout[3] = "regout3.txt";
        files->trace[0] = "core0trace.txt";
        files->trace[1] = "core1trace.txt";
        files->trace[2] = "core2trace.txt";
        files->trace[3] = "core3trace.txt";
        files->bustrace = "bustrace.txt";
        files->dsram[0] = "dsram0.txt";
        files->dsram[1] = "dsram1.txt";
        files->dsram[2] = "dsram2.txt";
        files->dsram[3] = "dsram3.txt";
        files->tsram[0] = "tsram0.txt";
        files->tsram[1] = "tsram1.txt";
        files->tsram[2] = "tsram2.txt";
        files->tsram[3] = "tsram3.txt";
        files->stats[0] = "stats0.txt";
        files->stats[1] = "stats1.txt";
        files->stats[2] = "stats2.txt";
        files->stats[3] = "stats3.txt";
        return;
    }

    // from command line
    int idx = 1;
    for (int i = 0; i < CORE_COUNT; i++) files->imem[i] = argv[idx++];
    files->memin = argv[idx++];
    files->memout = argv[idx++];
    for (int i = 0; i < CORE_COUNT; i++) files->regout[i] = argv[idx++];
    for (int i = 0; i < CORE_COUNT; i++) files->trace[i] = argv[idx++];
    files->bustrace = argv[idx++];
    for (int i = 0; i < CORE_COUNT; i++) files->dsram[i] = argv[idx++];
    for (int i = 0; i < CORE_COUNT; i++) files->tsram[i] = argv[idx++];
    for (int i = 0; i < CORE_COUNT; i++) files->stats[i] = argv[idx++];
}

// Read imem[i] into struct
void read_imem(files_T* files, core_T* core[CORE_COUNT]) {
    file_T* file;

    for (int i = 0; i < CORE_COUNT; i++) {
        memset(core[i]->imem, 0, sizeof(core[i]->imem));
        file = fopen(files->imem[i], "r");
        if (file) {
            for (int addr = 0; addr < IMEM_DEPTH; addr++) {
                if (fscanf(file, "%08X", &core[i]->imem[addr]) == EOF) {
                    break;
                }
            }
            fclose(file);
        }else{
            perror("read_imem()");
            exit(1);
        }
    }
}

// Read main memory
uint32_t read_mainmem(files_T* files, uint32_t* main_memory) {
    file_T* file;
    int addr = 0;
    memset(main_memory, 0, MEMIN_DEPTH * sizeof(uint32_t));
    file = fopen(files->memin, "r");
    if (file) {
        
        while (fscanf(file, "%08X", &main_memory[addr]) != EOF && addr < MEMIN_DEPTH) {
            addr++;
        }
        fclose(file);
    }
    return addr;
}

// Write outputs files once at the end of main loop
void write_outputs(files_T* files, core_T* cores[CORE_COUNT], uint32_t* main_memory) {
    file_T* file;

    // memout: write the full main memory image (2^21 words)
    file = fopen(files->memout, "w");
    if (!file) goto file_error;
    for (uint32_t i = 0; i < system_bus.max_memory_accessed; i++) {
        fprintf(file, "%08X\n", main_memory[i]);
    }
    fclose(file);
    

    for (int i = 0; i < CORE_COUNT; i++) {

        // regout
        file = fopen(files->regout[i], "w");
        if (!file) goto file_error;
        for (int r = 2; r < REGISTER_COUNT; r++) { // R2 to R15
            fprintf(file, "%08X\n", cores[i]->regs[r]);
        }
        fclose(file);

        // dsram
        file = fopen(files->dsram[i], "w");
        if (!file) goto file_error;
        for (int line = 0; line < TSRAM_DEPTH; line++) {
            for (int word = 0; word < CACHE_BLOCK_SIZE; word++) {
                fprintf(file, "%08X\n", cores[i]->cache.dsram[line].word[word]);
            }
        }
        fclose(file);
        

        // tsram
        file = fopen(files->tsram[i], "w");
        if (!file) goto file_error;
        for (int line = 0; line < TSRAM_DEPTH; line++) {
            uint32_t val = (cores[i]->cache.tsram[line].mesi_state << 12) | (cores[i]->cache.tsram[line].tag & 0xFFF);
            fprintf(file, "%08X\n", val);
        }
        fclose(file);

        // stats
        file = fopen(files->stats[i], "w");
        if (!file) goto file_error;
        fprintf(file, "cycles %d\n", cores[i]->stats.cycles);
        fprintf(file, "instructions %d\n", cores[i]->stats.instructions);
        fprintf(file, "read_hit %d\n", cores[i]->stats.read_hits);    
        fprintf(file, "write_hit %d\n", cores[i]->stats.write_hits);  
        fprintf(file, "read_miss %d\n", cores[i]->stats.read_misses);  
        fprintf(file, "write_miss %d\n", cores[i]->stats.write_misses);
        fprintf(file, "decode_stall %d\n", cores[i]->stats.decode_stall); 
        fprintf(file, "mem_stall %d\n", cores[i]->stats.mem_stall);      
        fclose(file);
    }

    return;
    file_error:
    perror("write_output(): Error opening file!");
}

// Write outputs each clock cycle (main loop iteration)
void log_bus_trace(files_T* files, int cycle) {
    if (system_bus.bus_cmd == BUS_NOCMD) return;

    file_T* fp = fopen(files->bustrace, "a");
    if (fp) {
        fprintf(fp, "%d %X %X %06X %08X %X\n", 
            cycle, 
            system_bus.bus_origin_id, 
            system_bus.bus_cmd, 
            system_bus.bus_addr & 0xFFFFF, 
            system_bus.bus_data, 
            system_bus.bus_shared);
        fclose(fp);
    }
}


void log_core_trace(files_T* files, core_T* cores[CORE_COUNT], int cycle) {
    for (int i = 0; i < CORE_COUNT; i++) {
        // Print as long as at least one pipeline stage is active.
        if (cores[i]->halted &&
            !cores[i]->pipe.fetch.active && !cores[i]->pipe.decode.active &&
            !cores[i]->pipe.execute.active && !cores[i]->pipe.memory.active &&
            !cores[i]->pipe.writeback.active) {
            continue;
        }

        file_T* fp = fopen(files->trace[i], "a");
        if (fp) {
            fprintf(fp, "%d ", cycle);

            // FETCH
            if (cores[i]->pipe.fetch.active) fprintf(fp, "%03X ", cores[i]->pipe.fetch.pc);
            else fprintf(fp, "--- ");

            // DECODE
            if (cores[i]->pipe.decode.active) fprintf(fp, "%03X ", cores[i]->pipe.decode.pc);
            else fprintf(fp, "--- ");

            // EXEC
            if (cores[i]->pipe.execute.active) fprintf(fp, "%03X ", cores[i]->pipe.execute.pc);
            else fprintf(fp, "--- ");

            // MEM
            if (cores[i]->pipe.memory.active) fprintf(fp, "%03X ", cores[i]->pipe.memory.pc);
            else fprintf(fp, "--- ");

            // WB
            if (cores[i]->pipe.writeback.active) fprintf(fp, "%03X ", cores[i]->pipe.writeback.pc);
            else fprintf(fp, "--- ");

            // Registers R2-R15
            for (int r = 2; r < REGISTER_COUNT; r++) {
                fprintf(fp, "%08X", cores[i]->regs[r]);
                if (r < REGISTER_COUNT - 1) fprintf(fp, " "); 
            }

            fprintf(fp, "\n");
            fclose(fp);
        }
    }
}