#include "general_utils.h"
#include "file_io.h"
#include <stdlib.h>

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
    for (int i = 0; i < 4; i++) files->imem[i] = argv[idx++];
    files->memin = argv[idx++];
    files->memout = argv[idx++];
    for (int i = 0; i < 4; i++) files->regout[i] = argv[idx++];
    for (int i = 0; i < 4; i++) files->trace[i] = argv[idx++];
    files->bustrace = argv[idx++];
    for (int i = 0; i < 4; i++) files->dsram[i] = argv[idx++];
    for (int i = 0; i < 4; i++) files->tsram[i] = argv[idx++];
    for (int i = 0; i < 4; i++) files->stats[i] = argv[idx++];
}

// read imem[i] into struct
void read_imem(SimFiles* files, Core core[4]) {
    FILE* fp;
    for (int i = 0;i < 4;i++) {
        fp = fopen(files->imem[i], "r");
        if (fp) {
            for (int addr = 0; addr < IMEM_DEPTH; addr++) {
                if (fscanf(fp, "%08x", &core[i].imem[addr]) == EOF) {
                    break;
                }
            }
            fclose(fp);
        }
    }
}

// read main mem (need to define main mem arr)
void read_mainmem(SimFiles* files, uint32_t* main_memory) {
    FILE* fp;

    fp = fopen(files->memin, "r");
    if (fp) {
        int addr = 0;
        while (fscanf(fp, "%08x", &main_memory[addr]) != EOF && addr < MEMIN_DEPTH) {
            addr++;
        }
        fclose(fp);
    }
}

// write outputs files (need to fill cache parts)
void write_outputs(SimFiles* files, Core cores[4], uint32_t* main_memory) {
    FILE* fp;

    // memout
    fp = fopen(files->memout, "w");
    if (fp) {
        for (int i = 0; i < MEMIN_DEPTH; i++) {
            fprintf(fp, "%08X\n", main_memory[i]);
        }
        fclose(fp);
    }

    for (int i = 0; i < 4; i++) {

        // regout
        fp = fopen(files->regout[i], "w");
        if (fp) {
            for (int r = 2; r < REGISTER_COUNT; r++) { // R2 to R15
                fprintf(fp, "%08X\n", cores[i].reg_file.regs[r]);
            }
            fclose(fp);
        }

        // dsram
        fp = fopen(files->dsram[i], "w");
        if (fp) {
            // fill in
            
            fclose(fp); 
        }

        // tsram
        fp = fopen(files->tsram[i], "w");
        if (fp) {
            // fill in
            fclose(fp);
        }

        // stats
        fp = fopen(files->stats[i], "w");
        if (fp) {
            fprintf(fp, "cycles %d\n", cores[i].stats.cycles);
            fprintf(fp, "instructions %d\n", cores[i].stats.instructions);
            fprintf(fp, "read_hit %d\n", cores[i].stats.read_hits);    
            fprintf(fp, "write_hit %d\n", cores[i].stats.write_hits);  
            fprintf(fp, "read_miss %d\n", cores[i].stats.read_misses);  
            fprintf(fp, "write_miss %d\n", cores[i].stats.write_misses);
            fprintf(fp, "decode_stall %d\n", cores[i].stats.decode_stall); 
            fprintf(fp, "mem_stall %d\n", cores[i].stats.mem_stall);      
            fclose(fp);
        }
    }
}