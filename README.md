# Multi-Core Cycle-Accurate Simulator (MESI & Bus Coherence)

## Project Overview
This project implements a **cycle-accurate simulator** of a four-core processor system. It models the intricate interaction between instruction execution, pipeline synchronization, and memory hierarchy. By advancing one clock cycle at a time, the simulator provides a high-fidelity representation of how hardware handles shared resources and data consistency.

### System Characteristics
* **Cores:** Four independent cores.
* **Bus Architecture:** Shared bus with round-robin arbitration.
* **Coherency Protocol:** MESI (Modified, Exclusive, Shared, Invalid).
* **Clocking:** Discrete clock cycles with a strict Two-Phase (Computation/Commit) update model.

---

## Pipeline Architecture
Each core operates using a classic five-stage RISC-style pipeline. The simulator enforces strict architectural constraints, specifically the **absence of data forwarding**, which necessitates hardware stalling for hazard resolution.



### Pipeline Stages
| Stage | Description |
| :--- | :--- |
| **Fetch (IF)** | Reads instructions from the core’s private instruction memory. |
| **Decode (ID)** | Parses operands, resolves branches (with a single delay slot), and performs **RAW hazard detection**. |
| **Execute (EX)** | Performs ALU operations and calculates effective memory addresses. |
| **Memory (MEM)** | Interfaces with the local data cache; stalls if bus access is required. |
| **Write-Back (WB)** | Schedules architectural register updates to be committed at the cycle's end. |

> **Hazard Note:** Because forwarding is not supported, the Decode stage checks for RAW hazards against instructions in EX, MEM, and WB stages. If a dependency is found, a stall is asserted and a bubble is inserted into the Execute stage.

---

## Memory Hierarchy & MESI Coherence
The memory system consists of a **private direct-mapped data cache** per core and a shared main memory. Consistency across caches is maintained via the **MESI Protocol**.



### Cache Management & Bus Transactions
* **Load/Store Logic:** Operations first access the local cache. If a miss occurs or an ownership change is required (e.g., writing to a 'Shared' line), the core initiates a bus transaction.
* **Bus Arbitration:** Only one transaction may be active per cycle. The bus uses a **Round-Robin** scheme to manage competing requests from the four cores.
* **Snooping:** The bus module snoops all caches for every `BusRd` or `BusRdX` request, enforcing transitions and triggering `Flush` operations when a core holds a 'Modified' line.

---

## Execution Model: The Two-Phase Clock
To ensure correctness and prevent race conditions within the software simulation, each clock cycle is conceptually divided into two phases:

1.  **Computation Phase:** All modules (Pipeline, Bus, Memory) execute logic using the state values present at the *start* of the cycle. No architectural state is permanently modified.
2.  **Commit Phase:** At the "clock edge," all updates are applied simultaneously. This includes pipeline register movement, cache state transitions, and register file writes. 

This model ensures that register writes become visible only in the **next cycle**, accurately reflecting real hardware behavior.

---

## Module Structure & Responsibilities

| Module | Files | Responsibility |
| :--- | :--- | :--- |
| **Global Controller** | `main.c` | Initialization, cycle-by-cycle synchronization, and termination management. |
| **Pipeline Core** | `pipeline.c/h` | Stage logic, hazard detection, and branch resolution. |
| **Memory System** | `memory.c/h` | Hit/miss detection and word-level cache access helpers. |
| **Shared Bus** | `bus.c/h` | MESI state machine, snooping, and memory latency modeling. |
| **File I/O** | `file-io.c/h` | Loading memory images and generating per-cycle trace logs. |
| **Types and Definitions** | `general-utils.h` | Data structures, types, enums, etc. |

---

## Simulation Termination
The simulation ends only when a "Full Drain" condition is met:
1. All four cores must have fetched a `HALT` instruction.
2. All instructions currently in the pipelines must have progressed through the Write-Back stage.
3. All pending bus transactions must be completed.

Upon termination, the simulator generates a final dump of the register files, main memory, and cache SRAM contents for verification against project specifications.

---

## Code Style Conventions

### Naming

#### Variables, Types, and Constants
- Variables use `snake_case`.
- Types use `snake_case_T`.
  - The `_T` suffix explicitly marks a type.
  - We avoid `_t` to prevent conflicts with GNU library types.
  - Acronyms inside type names are written in uppercase and followed by an underscore.
    - Example: `mesi_state_T`
- Constants use `ALL_CAPS`.

#### Functions and Macros
- Functions use `snake_case(...)`.
  - Acronyms in function names remain lowercase.
- Macros use `FULL_CAPS(...)`.

---

### Files and Project Layout

- Files and directories use `kebab-case`.
- All relevant source files are placed directly in a single directory:
  - `sim/` contains the simulator source files.
  - Assembly files are located in a directory named after the corresponding program.
- The `makefile` is located outside these directories.
  - The `all` target builds the simulator binary and places the result in the `test/` directory.
