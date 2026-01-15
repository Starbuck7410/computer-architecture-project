# computer-architecture-project
The Computer Architecture project


## TODO:

### Step 1:
- Design Structs, Enums, and overall architecture of the project
- Copy and adapt assembler, simuator

Amit - Implement `bus.c: send_bus_read_request()`, and add signature to `bus.h`\
Daniel - Implement `pipeline.c: WB_stage()` [DONE]\
Zohar - Implement `bus.c: invalidate_cache_block()`, and add signature to `bus.h` [DONE]\
Shraga - Think about the memory control flow (`pipeline.c: memory_stage()`)


## Code style conventions

### Variable, types, and constants
- Variables will be named in `snake_case`
- Types will be in `PascalCase`, when there are acronyms present they will be in caps with an underscore afterwards (For example `MESI_State`)
- Constants will be in `ALL_CAPS` 

### Functions and macros
- Functions will be in `snake_case(...)`
- Macros will be in `FULL_CAPS(...)` 

### Files and directories
- Files and directories will be in `snake_case`
- All relevant files will be lazily tossed in the same directory, `sim` for simulator, `asm` for assembler and `prog` for assembly files
- The makefile will be outside of these directories, with recipes that target the different directories

