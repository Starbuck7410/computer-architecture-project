                add $r5, $r0, $r0, 0
                add $r6, $r0, $r1, 0x100
                add $r7, $r0, $r1, 0x200
                add $r13, $r0, $r1, 16
                add $r2, $r0, $r0, 0
LOOP_I:         beq $r1, $r2, $r13, END
                add $r3, $r0, $r0, 0
LOOP_J:         beq $r1, $r3, $r13, NEXT_I
                add $r4, $r0, $r0, 0
                add $r8, $r0, $r0, 0
LOOP_K:         beq $r1, $r4, $r13, STORE_RESULT
                sll $r9, $r2, $r1, 4
                add $r9, $r9, $r4, 0
                add $r9, $r9, $r5, 0
                lw $r10, $r9, $r0, 0
                sll $r9, $r4, $r1, 4
                add $r9, $r9, $r3, 0
                add $r9, $r9, $r6, 0
                lw $r11, $r9, $r0, 0
                mul $r12, $r10, $r11, 0
                add $r8, $r8, $r12, 0
                add $r4, $r4, $r1, 1
                beq $r1, $r0, $r0, LOOP_K
STORE_RESULT:   sll $r9, $r2, $r1, 4
                add $r9, $r9, $r3, 0
                add $r9, $r9, $r7, 0
                sw $r8, $r9, $r0, 0
                add $r3, $r3, $r1, 1
                beq $r1, $r0, $r0, LOOP_J
NEXT_I:         add $r2, $r2, $r1, 1
                beq $r1, $r0, $r0, LOOP_I
END:            halt $r0, $r0, $r0, 0