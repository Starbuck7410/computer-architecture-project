# computer-architecture-project
The Computer Architecture project




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
