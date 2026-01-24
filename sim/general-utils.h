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
    OPCODE_ADD = 0, OPCODE_SUB, OPCODE_AND, OPCODE_OR, OPCODE_XOR, OPCODE_MUL, OPCODE_SLL, OPCODE_SRA, OPCODE_SRL,
    OPCODE_BEQ, OPCODE_BNE, OPCODE_BLT, OPCODE_BGT, OPCODE_BLE, OPCODE_BGE, OPCODE_JAL, OPCODE_LW, OPCODE_SW,
    OPCODE_HALT = 20
} opcode_T;

// MESI encoding MUST match the project spec (TSRAM bits 13:12):
// 0: Invalid, 1: Shared, 2: Exclusive, 3: Modified
typedef enum {
    MESI_INVALID = 0,
    MESI_SHARED = 1,
    MESI_EXCLUSIVE = 2,
    MESI_MODIFIED = 3
} mesi_state_T;
typedef enum { BUS_NOCMD = 0, BUS_RD, BUS_RDX, BUS_FLUSH } bus_cmd_T;

// instruction_T & Registers
typedef struct {
    uint32_t binary_value; 
    opcode_T opcode;        
    uint8_t rd;            
    uint8_t rs;            
    uint8_t rt;            
    int32_t imm;           
} instruction_T; // We REAALLLY wanted to use bit-fields here but damn you undefined behaviour

// pipeline_T
typedef struct {
    uint32_t pc;            
    instruction_T instruction;       
    int32_t result;        
    bool active;            
    bool stall; // Helper to prevent stage from advancing
} pipeline_stage_T;

typedef struct {
    pipeline_stage_T fetch;
    pipeline_stage_T decode;
    pipeline_stage_T execute;
    pipeline_stage_T memory;
    pipeline_stage_T writeback;
} pipeline_T;

// Memory & cache_T
typedef struct {
    uint32_t tag;   
    mesi_state_T mesi_state; 
} tsram_line_T;

typedef struct {
    uint32_t word[CACHE_BLOCK_SIZE];
} dsram_line_T;

typedef struct {
    dsram_line_T dsram[TSRAM_DEPTH];     
    tsram_line_T tsram[TSRAM_DEPTH];   
} cache_T;

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
} core_stats_T;

// Bus Structures
typedef struct {
    int bus_origin_id;      
    bus_cmd_T bus_cmd;         
    uint32_t bus_addr;   
    uint32_t bus_data;   
} bus_request_T;

typedef struct {
    bool has_pending_request;
    bus_request_T request;
    bool request_done; // Flag set by bus when operation completes
} bus_interface_T;

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

    pipeline_T pipe;
    cache_T cache;
    core_stats_T stats;
    bus_interface_T bus_interface; // Private interface
    uint32_t imem[IMEM_DEPTH]; 
    bool halted;            
} core_T;

typedef struct {
    cache_T * cpu_cache[CORE_COUNT];
    bus_interface_T * bus_interface[CORE_COUNT]; // Pointers to core interfaces
    uint32_t * system_memory; 
    uint32_t max_memory_accessed;      // Max item written to memory

    // Current State of the Bus Wire
    int bus_origin_id;
    bus_cmd_T bus_cmd;
    uint32_t bus_addr;
    uint32_t bus_data;
    bool bus_shared; 

    // Internal Arbitration State
    int cooldown_timer;      
    int word_offset;         
    int last_granted_device; 
    bool busy;               

    // For BUS_FLUSH completion: what MESI state should the flusher keep?
    // - Eviction flush: INVALID
    // - Snoop flush on BUS_RD: SHARED (M->S)
    // - Snoop flush on BUS_RDX: INVALID (M->I)
    mesi_state_T flush_post_state;
    bool flush_post_state_valid;
} bus_T;