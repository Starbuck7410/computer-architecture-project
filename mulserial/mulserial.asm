// --- Register Mapping ---
// $r2  = i (Row index for A and C) 6 3 4.5
// $r3  = j (Column index for B and C)
// $r4  = k (Inner dot product iterator)
// $r5  = Base Address Matrix A (0x000)
// $r6  = Base Address Matrix B (0x100)
// $r7  = Base Address Matrix C (0x200)
// $r8  = Sum (Accumulator for dot product)
// $r9  = Temporary Address Calculator
// $r10 = Value loaded from A
// $r11 = Value loaded from B
// $r12 = Multiplication Result / Temporary
// $r13 = Constant 16 (Matrix dimension)

// --- Initialization ---

// Set Base A = 0
add $r5, $r0, $r0, 0       

// Set Base B = 0x100 (Using $r1 which holds the immediate 0x100)
add $r6, $r0, $r1, 0x100   

// Set Base C = 0x200
add $r7, $r0, $r1, 0x200   

// Set Loop Limit = 16
add $r13, $r0, $r1, 16     

// Initialize i = 0
add $r2, $r0, $r0, 0       

LOOP_I:
    // Check if i == 16. If yes, jump to END.
    // Logic: if ($r2 == $r13) PC = $r1 (Value of END label)
    beq $r1, $r2, $r13, END 

    // Initialize j = 0
    add $r3, $r0, $r0, 0   

LOOP_J:
    // Check if j == 16. If yes, jump to NEXT_I.
    beq $r1, $r3, $r13, NEXT_I

    // Initialize k = 0
    add $r4, $r0, $r0, 0   

    // Initialize Sum = 0
    add $r8, $r0, $r0, 0   

LOOP_K:
    // Check if k == 16. If yes, jump to STORE_RESULT.
    beq $r1, $r4, $r13, STORE_RESULT

    // --- Calculate Address of A[i][k] ---
    // Formula: BaseA + (i * 16) + k
    // Operation: sll $rd, $rs, $rt, imm -> $rd = $rs << $rt
    // We use imm=4 to put 4 into $r1, then shift $r2(i) by $r1(4).
    sll $r9, $r2, $r1, 4    // $r9 = i << 4 (which is i * 16)
    add $r9, $r9, $r4, 0    // $r9 = (i * 16) + k
    add $r9, $r9, $r5, 0    // $r9 = BaseA + offset (Absolute Address)
    
    // Load A[i][k] from Memory
    // lw $rd, $rs, $rt -> $rd = MEM[$rs + $rt]
    lw $r10, $r9, $r0, 0    // $r10 = MEM[$r9 + 0]

    // --- Calculate Address of B[k][j] ---
    // Formula: BaseB + (k * 16) + j
    sll $r9, $r4, $r1, 4    // $r9 = k << 4 (which is k * 16)
    add $r9, $r9, $r3, 0    // $r9 = (k * 16) + j
    add $r9, $r9, $r6, 0    // $r9 = BaseB + offset (Absolute Address)

    // Load B[k][j] from Memory
    lw $r11, $r9, $r0, 0    // $r11 = MEM[$r9 + 0]

    // --- Calculation ---
    // Multiply A[i][k] * B[k][j]
    mul $r12, $r10, $r11, 0 // $r12 = $r10 * $r11
    
    // Accumulate into Sum
    add $r8, $r8, $r12, 0   // Sum = Sum + Product

    // --- Increment k ---
    add $r4, $r4, $r1, 1    // k++
    
    // Unconditional Jump back to LOOP_K
    // Logic: if ($r0 == $r0) PC = $r1 (LOOP_K address)
    beq $r1, $r0, $r0, LOOP_K

STORE_RESULT:
    // --- Store Sum into C[i][j] ---
    // Formula: BaseC + (i * 16) + j
    sll $r9, $r2, $r1, 4    // $r9 = i * 16
    add $r9, $r9, $r3, 0    // $r9 = (i * 16) + j
    add $r9, $r9, $r7, 0    // $r9 = BaseC + offset

    // Store Instruction: sw $rd, $rs, $rt -> MEM[$rs + $rt] = $rd  (Note: Check your specific ISA implementation, standard MIPS SW is different, but based on the PDF table, SW is Opcode 17. Usually SW is MEM[rs+rt] = rd).
    // Let's assume standard behavior described in table: MEM[R[rs]+R[rt]] = R[rd].
    sw $r8, $r9, $r0, 0     // MEM[$r9 + 0] = Sum

    // --- Increment j ---
    add $r3, $r3, $r1, 1    // j++
    beq $r1, $r0, $r0, LOOP_J

NEXT_I:
    // --- Increment i ---
    add $r2, $r2, $r1, 1    // i++
    beq $r1, $r0, $r0, LOOP_I

END:
    // Halt Core 0
    halt $r0, $r0, $r0, 0