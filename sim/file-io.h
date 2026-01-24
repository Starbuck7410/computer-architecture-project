#pragma once
#include "general-utils.h"
#include <stdlib.h>

extern bus_T system_bus;
typedef FILE file_T; // Just for consistency

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
} files_T;

// Function Declarations
void get_arguments(int argc, char * argv[], files_T * files);
void read_imem(files_T * files, core_T * core[CORE_COUNT]); // Changed to core_T*[] to match main
uint32_t read_mainmem(files_T * files, uint32_t * main_memory);
void write_outputs(files_T * files, core_T * cores[CORE_COUNT], uint32_t * main_memory);

// Trace Functions (Called every cycle)
void log_bus_trace(files_T * files, int cycle);
void log_core_trace(files_T * files, core_T* cores[CORE_COUNT], int cycle);