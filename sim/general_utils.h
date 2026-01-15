#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) ;
#endif

#define IMEM_DEPTH 1024
#define MEMIN_DEPTH (1 << 21) // 2^21 words (as defined in project spec)
#define DSRAM_DEPTH 512 
#define CACHE_BLOCK_SIZE 8
#define REGISTER_COUNT 16 
#define TSRAM_DEPTH (DSRAM_DEPTH / CACHE_BLOCK_SIZE) // 64 lines
#define CORE_COUNT 4
#define BUS_DELAY 16

typedef enum {
    OP_ADD = 0, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_MUL, OP_SLL, OP_SRA, OP_SRL,
    OP_BEQ, OP_BNE, OP_BLT, OP_BGT, OP_BLE, OP_BGE, OP_JAL, OP_LW, OP_SW,
    OP_HALT = 20
} Opcode;

// MESI encoding MUST match the project spec (TSRAM bits 13:12):
// 0: Invalid, 1: Shared, 2: Exclusive, 3: Modified
typedef enum {
    MESI_INVALID = 0,
    MESI_SHARED = 1,
    MESI_EXCLUSIVE = 2,
    MESI_MODIFIED = 3
} MESI_State;
typedef enum { BUS_NOCMD = 0, BUS_RD, BUS_RDX, BUS_FLUSH } BusCmd;

// Instruction & Registers
typedef struct {
    uint32_t binary_value; 
    Opcode opcode;        
    uint8_t rd;            
    uint8_t rs;            
    uint8_t rt;            
    int32_t imm;           
} Instruction;

// Pipeline
typedef struct {
    uint32_t pc;            
    Instruction inst;       
    int32_t result;        
    bool active;            
    bool stall; // Helper to prevent stage from advancing
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
    uint32_t tag;   
    MESI_State mesi_state; 
} TSRAM_Line;

typedef struct {
    uint32_t word[CACHE_BLOCK_SIZE];
} DSRAM_Line;

typedef struct {
    DSRAM_Line dsram[TSRAM_DEPTH];     
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
    int decode_stall; 
    int mem_stall;    
} CoreStats;

// Bus Structures
typedef struct {
    int bus_orig_id;      
    BusCmd bus_cmd;         
    uint32_t bus_addr;   
    uint32_t bus_data;   
} BusRequest;

typedef struct {
    bool has_pending_request;
    BusRequest request;
    bool request_done; // Flag set by bus when operation completes
} BusInterface;

// Main core
typedef struct {
    int id;                 
    uint32_t pc;            
    int32_t regs[REGISTER_COUNT]; 

    // Register write is committed on the clock edge (so reads in the same cycle
    // see the old value, as required by the spec).
    bool pending_reg_write;
    uint8_t pending_reg_dst;
    int32_t pending_reg_value;

    // R1 holds the sign-extended immediate of the instruction currently in DECODE.
    // We also commit it on the clock edge.
    bool pending_imm_write;
    int32_t pending_imm_value;

    // Branch/jump with a single delay-slot: set redirect here in DECODE.
    bool pc_redirect_valid;
    uint32_t pc_redirect;

    // After HALT is decoded we stop fetching new instructions, but we still
    // let the pipeline drain.
    bool stop_fetch;

    Pipeline pipe;
    Cache cache;
    CoreStats stats;
    BusInterface bus_interface; // Private interface
    uint32_t imem[IMEM_DEPTH]; 
    bool halted;            
} Core;

typedef struct {
    Cache * cpu_cache[CORE_COUNT];
    BusInterface * bus_interface[CORE_COUNT]; // Pointers to core interfaces
    uint32_t * system_memory; // Changed to uint32_t ptr

    // Current State of the Bus Wire
    int bus_orig_id;
    BusCmd bus_cmd;
    uint32_t bus_addr;
    uint32_t bus_data;
    bool bus_shared; 

    // Internal Arbitration State
    int cooldown_timer;      
    int word_offset;         
    int last_granted_device; 
    bool busy;               
} SystemBus;