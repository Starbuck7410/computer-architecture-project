#include "file_io.h"

void get_arguments(int argc, char* argv[], SimFiles* files) {
    // defult
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

// read imem[i] into struct
void read_imem(SimFiles* files, Core core[CORE_COUNT]) {
    FILE* file;

    for (int i = 0; i < CORE_COUNT; i++) {
        memset(core[i].imem, 0, sizeof(core[i].imem));
        file = fopen(files->imem[i], "r");
        if (file) {
            for (int addr = 0; addr < IMEM_DEPTH; addr++) {
                if (fscanf(file, "%08x", &core[i].imem[addr]) == EOF) {
                    break;
                }
            }
            fclose(file);
        }
    }
}

// read main mem (need to define main mem arr)
void read_mainmem(SimFiles* files, uint32_t* main_memory) {
    FILE* file;

    memset(main_memory, 0, MEMIN_DEPTH * sizeof(uint32_t));
    file = fopen(files->memin, "r");
    if (file) {
        int addr = 0;
        while (fscanf(file, "%08x", &main_memory[addr]) != EOF && addr < MEMIN_DEPTH) {
            addr++;
        }
        fclose(file);
    }
}

// write outputs files 
void write_outputs(SimFiles* files, Core cores[CORE_COUNT], uint32_t* main_memory) {
    FILE* file;

    int max_addr = MEMIN_DEPTH-1;
    while (max_addr >= 0 && main_memory[max_addr] == 0) {
        max_addr--;
    }
    
    // memout
    file = fopen(files->memout, "w");
    if (!file) goto file_error;
    for (int i = 0; i <= max_addr; i++) {
        fprintf(file, "%08X\n", main_memory[i]);
    }
    fclose(file);
    

    for (int i = 0; i < CORE_COUNT; i++) {

        // regout
        file = fopen(files->regout[i], "w");
        if (!file) goto file_error;
        for (int r = 2; r < REGISTER_COUNT; r++) { // R2 to R15
            fprintf(file, "%08X\n", cores[i].regs[r]);
        }
        fclose(file);

        // dsram
        file = fopen(files->dsram[i], "w");
        if (!file) goto file_error;
        for (int line = 0; line < TSRAM_DEPTH; line++) {
            for (int word = 0; word < CACHE_BLOCK_SIZE; word++) {
                fprintf(file, "%08X\n", cores[i].cache.dsram[line].word[word]);
            }
        }
        fclose(file);
        

        // tsram
        file = fopen(files->tsram[i], "w");
        if (!file) goto file_error;
        for (int line = 0; line < TSRAM_DEPTH; line++) {
            uint32_t val = (cores[i].cache.tsram[line].mesi_state << 12) | (cores[i].cache.tsram[line].tag & 0xFFF);
            fprintf(file, "%08X\n", val);
        }
        fclose(file);

        // stats
        file = fopen(files->stats[i], "w");
        if (!file) goto file_error;
        fprintf(file, "cycles %d\n", cores[i].stats.cycles);
        fprintf(file, "instructions %d\n", cores[i].stats.instructions);
        fprintf(file, "read_hit %d\n", cores[i].stats.read_hits);    
        fprintf(file, "write_hit %d\n", cores[i].stats.write_hits);  
        fprintf(file, "read_miss %d\n", cores[i].stats.read_misses);  
        fprintf(file, "write_miss %d\n", cores[i].stats.write_misses);
        fprintf(file, "decode_stall %d\n", cores[i].stats.decode_stall); 
        fprintf(file, "mem_stall %d\n", cores[i].stats.mem_stall);      
        fclose(file);
    }

    return;
    file_error:
    perror("write_output(): Error opening file!");
    // And now you can add more logic here for handling failed files!
}