#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) ;
#endif

#define IMEM_DEPTH 1024
#define DSRAM_DEPTH 512 
#define TSRAM_DEPTH 64
#define REGISTER_COUNT 16 // R0 to R15


typedef enum {
    MESI_MODIFIED,
    MESI_EXCLUSIVE,
    MESI_SHARED,
    MESI_INVALID
} MESI_State;

typedef enum {
    BUS_NOCMD = 0,
    BUS_RD,
    BUS_RDX,
    BUS_FLUSH
} BusCmd;

// Instruction & Registers
typedef struct {
    uint32_t binary_value; 

    uint8_t opcode;        // bits 31:24
    uint8_t rd;            // bits 23:20
    uint8_t rs;            // bits 19:16
    uint8_t rt;            // bits 15:12

    int32_t imm;           // bits 11:0, sign-extended
} Instruction;

typedef struct {
    int32_t regs[REGISTER_COUNT]; 
} RegisterFile;

// Pipeline
typedef struct {
    uint32_t pc;            // The PC of the instruction currently in this stage
    Instruction inst;       
    uint32_t alu_result;    // Result calculated in EXEC
    bool active;            // true = real instruction, false = bubble/empty
} PipelineStage;

typedef struct {
    PipelineStage fetch;
    PipelineStage decode;
    PipelineStage execute;
    PipelineStage mem;
    PipelineStage wb;
} Pipeline;

// Memory & Cache
typedef struct {
    uint32_t tag;   // bits 11:0 
    MESI_State mesi_state; 
} TSRAM_Line;

typedef struct {
    uint32_t dsram[DSRAM_DEPTH];     
    TSRAM_Line tsram[TSRAM_DEPTH];   
} Cache;

// Status
typedef struct {
    int cycles;
    int instructions;
    int read_hits;
    int write_hits;
    int read_misses;
    int write_misses;
    int decode_stall; // Number of cycles stalled in Decode
    int mem_stall;    // Number of cycles stalled in Mem
} CoreStats;


// Main core
typedef struct {
    int id;                 // 0, 1, 2, 3
    uint32_t pc;            // Current PC

    RegisterFile reg_file;
    Pipeline pipe;
    Cache cache;
    CoreStats stats;

    uint32_t imem[IMEM_DEPTH]; // Private Instruction Memory

    bool halted;            // halt
} Core;

//Bus
typedef struct {
    int bus_origid;      // 3 bits: 0-3 for cores, 4 for Main Memory
    BusCmd bus_cmd;         
    uint32_t bus_addr;   // 21-bit word address 
    uint32_t bus_data;   // 32-bit word data 
    int bus_shared;     // 1 bit
    int last_granted_device; // ID of the device that used the bus last cycle
    bool busy;               // Is the bus currently processing a transaction

} SystemBus;


// ========== Function Declarations ==========
