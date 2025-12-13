#include "general_utils.h"
#include <stdlib.h>

// File management
typedef struct {
    char* imem[4];
    char* memin;
    char* memout;
    char* regout[4];
    char* trace[4];
    char* bustrace;
    char* dsram[4];
    char* tsram[4];
    char* stats[4];
} SimFiles;

// Function Declarations
void get_arguments(int argc, char* argv[], SimFiles* files);
void read_imem(SimFiles* files, Core core[4]);
void read_mainmem(SimFiles* files, uint32_t* main_memory);
void write_outputs(SimFiles* files, Core cores[4], uint32_t* main_memory);