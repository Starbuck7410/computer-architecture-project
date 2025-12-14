#pragma once
#include "general_utils.h"
#include <stdlib.h>

// File management
typedef struct {
    char* imem[CORE_COUNT];
    char* memin;
    char* memout;
    char* regout[CORE_COUNT];
    char* trace[CORE_COUNT];
    char* bustrace;
    char* dsram[CORE_COUNT];
    char* tsram[CORE_COUNT];
    char* stats[CORE_COUNT];
} SimFiles;

// Function Declarations
void get_arguments(int argc, char* argv[], SimFiles* files);
void read_imem(SimFiles* files, Core core[CORE_COUNT]);
void read_mainmem(SimFiles* files, uint32_t* main_memory);
void write_outputs(SimFiles* files, Core cores[CORE_COUNT], uint32_t* main_memory);