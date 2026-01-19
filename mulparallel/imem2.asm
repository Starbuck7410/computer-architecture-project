add $r2, $zero, $imm, 0        # baseA = 0x000
add $r4, $zero, $imm, 256      # baseB = 0x100
add $r5, $zero, $imm, 512      # baseC = 0x200
add $r3, $zero, $imm, 2        # core id
sll $r6, $r3, $imm, 2          # i = coreid*4
add $r7, $r6, $imm, 4          # i_end = i + 4
add $r15, $zero, $imm, 9        # target I_LOOP (patched)
add $r13, $zero, $imm, 10        # target J_LOOP (patched)
add $r14, $zero, $imm, 12        # target K_LOOP (patched)
add $r8, $zero, $imm, 0        # j = 0
add $r10, $zero, $imm, 0       # sum = 0
add $r9, $zero, $imm, 0        # k = 0
sll $r12, $r6, $imm, 4         # row_off = i*16
add $r11, $r2, $r12, 0         # tmp = baseA + row_off
add $r11, $r11, $r9, 0         # tmp = A addr = baseA + i*16 + k
lw  $r11, $r11, $imm, 0        # a = MEM[A addr]
sll $r12, $r9, $imm, 4         # tmp2 = k*16
add $r12, $r12, $r8, 0         # tmp2 = k*16 + j
add $r12, $r12, $r4, 0         # B addr = baseB + k*16 + j
lw  $r12, $r12, $imm, 0        # b = MEM[B addr]
mul $r11, $r11, $r12, 0        # prod = a*b
add $r10, $r10, $r11, 0        # sum += prod
add $r9, $r9, $imm, 1          # k++
blt $r14, $r9, $imm, 16        # if (k < 16) goto K_LOOP
sll $r12, $r6, $imm, 4         # row_off = i*16
add $r12, $r12, $r8, 0         # row_off + j
add $r12, $r12, $r5, 0         # C addr = baseC + i*16 + j
sw  $r10, $r12, $imm, 0        # MEM[C addr] = sum
add $r8, $r8, $imm, 1          # j++
blt $r13, $r8, $imm, 16        # if (j < 16) goto J_LOOP
add $r6, $r6, $imm, 1          # i++
blt $r15, $r6, $r7, 0          # if (i < i_end) goto I_LOOP
halt $zero, $zero, $zero, 0
