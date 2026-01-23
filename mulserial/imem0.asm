# r2 => i, r3 => j, r4 => k, r5 => A, r6 => B, r7 => C, r8 => sum, r9 - r12 => temp, r13 => N
                add $r5,  $r0, $r0, 0        # A base = 0x000
                add $r6,  $r0, $r1, 0x100    # B base = 0x100
                add $r7,  $r0, $r1, 0x200    # C base = 0x200
                add $r13, $r0, $r1, 16       # N = 16
                add $r2,  $r0, $r0, 0        # i = 0
LOOP_I:         beq $r1, $r2, $r13, END
                add $r0, $r0, $r0, 0         # Delay slot NOP
                add $r3, $r0, $r0, 0         # j = 0
LOOP_J:         beq $r1, $r3, $r13, NEXT_I
                add $r0, $r0, $r0, 0         # Delay slot NOP
                add $r4, $r0, $r0, 0         # k = 0
                add $r8, $r0, $r0, 0         # sum = 0
LOOP_K:         beq $r1, $r4, $r13, STORE_RESULT
                add $r0, $r0, $r0, 0         # Delay slot NOP
                sll $r9,  $r2, $r1, 4        # i * 16
                add $r10, $r4, $r0, 0        # k
                add $r9,  $r9, $r10, 0
                add $r9,  $r9, $r5,  0       # r9 = &A[16 * i + k]
                lw  $r10, $r9, $r0, 0        # r10 = A[16 * i + k]
                sll $r9,  $r4, $r1, 4        # k * 16
                add $r11, $r3, $r0, 0        # j
                add $r9,  $r9, $r11, 0
                add $r9,  $r9, $r6,  0       # r9 = &B[16 * k + j]
                lw  $r11, $r9, $r0, 0        # r11 = B[16 * k + j]
                mul $r12, $r10, $r11, 0
                add $r8,  $r8,  $r12, 0
                add $r4, $r4, $r1, 1
                beq $r1, $r0, $r0, LOOP_K
                add $r0, $r0, $r0, 0         # Delay slot NOP
STORE_RESULT:   sll $r9,  $r2, $r1, 4        # i * 16
                add $r10, $r3, $r0, 0        # j
                add $r9,  $r9, $r10, 0
                add $r9,  $r9, $r7,  0
                sw  $r8,  $r9, $r0, 0
                add $r3, $r3, $r1, 1
                beq $r1, $r0, $r0, LOOP_J
                add $r0, $r0, $r0, 0         # Delay slot NOP
NEXT_I:         add $r2, $r2, $r1, 1
                beq $r1, $r0, $r0, LOOP_I
                add $r0, $r0, $r0, 0         # Delay slot NOP
END:            lw  $r0, $r0, $r1, 0x4FF     # Flush cache to main memory
                halt $r0, $r0, $r0, 0
