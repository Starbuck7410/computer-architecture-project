# imem2.asm  (Core 2)  C = A*B for 16x16 matrices (row-partitioned)
# Core 2 computes rows i = 8..11
# Memory map:
#   A: 0x000 - 0x0FF
#   B: 0x100 - 0x1FF
#   C: 0x200 - 0x2FF
#
# IMPORTANT:
# Cache is write-back. To make sure results reach main memory (memout),
# we force-evict the C region by reading the alias range 0x400-0x4FF
# (same cache indices as 0x200-0x2FF, +0x200 = +512 words).
#
# Registers:
#   r2=i, r3=j, r4=k
#   r5=A_base, r6=B_base, r7=C_base
#   r8=sum
#   r9,r10,r11,r12 temps
#   r13=N=16
#   r14=i_end_exclusive
#   r15 used in flush loop (addr)

        add $r5,  $r0, $r0, 0        # A base = 0x000
        add $r6,  $r0, $r1, 0x100    # B base = 0x100
        add $r7,  $r0, $r1, 0x200    # C base = 0x200
        add $r13, $r0, $r1, 16       # N = 16

        add $r2,  $r0, $r1, 8# i = 8
        add $r14, $r0, $r1, 12  # i_end = 12 (exclusive)

LOOP_I: beq $r1, $r2, $r14, FLUSH_ALL
        add $r0, $r0, $r0, 0         # delay slot

        add $r3, $r0, $r0, 0         # j = 0
LOOP_J: beq $r1, $r3, $r13, NEXT_I
        add $r0, $r0, $r0, 0         # delay slot

        add $r4, $r0, $r0, 0         # k = 0
        add $r8, $r0, $r0, 0         # sum = 0
LOOP_K: beq $r1, $r4, $r13, STORE_RESULT
        add $r0, $r0, $r0, 0         # delay slot

        # A[i][k] address: A_base + (i*16 + k)
        sll $r9,  $r2, $r1, 4        # r9 = i*16
        add $r10, $r4, $r0, 0        # r10 = k
        add $r9,  $r9, $r10, 0       # r9 = i*16 + k
        add $r9,  $r9, $r5,  0       # r9 = addrA
        lw  $r10, $r9, $r0, 0        # r10 = A[i][k]

        # B[k][j] address: B_base + (k*16 + j)
        sll $r9,  $r4, $r1, 4        # r9 = k*16
        add $r11, $r3, $r0, 0        # r11 = j
        add $r9,  $r9, $r11, 0       # r9 = k*16 + j
        add $r9,  $r9, $r6,  0       # r9 = addrB
        lw  $r11, $r9, $r0, 0        # r11 = B[k][j]

        mul $r12, $r10, $r11, 0      # r12 = A[i][k] * B[k][j]
        add $r8,  $r8,  $r12, 0      # sum += product

        add $r4, $r4, $r1, 1         # k++
        beq $r1, $r0, $r0, LOOP_K
        add $r0, $r0, $r0, 0         # delay slot

STORE_RESULT:
        # C[i][j] address: C_base + (i*16 + j)
        sll $r9,  $r2, $r1, 4        # r9 = i*16
        add $r10, $r3, $r0, 0        # r10 = j
        add $r9,  $r9, $r10, 0       # r9 = i*16 + j
        add $r9,  $r9, $r7,  0       # r9 = addrC
        sw  $r8,  $r9, $r0, 0        # C[i][j] = sum

        add $r3, $r3, $r1, 1         # j++
        beq $r1, $r0, $r0, LOOP_J
        add $r0, $r0, $r0, 0         # delay slot

NEXT_I:
        add $r2, $r2, $r1, 1         # i++
        beq $r1, $r0, $r0, LOOP_I
        add $r0, $r0, $r0, 0         # delay slot

# Force-evict C region (0x200-0x2FF) by reading alias region 0x400-0x4FF
FLUSH_ALL:
        add $r15, $r0, $r1, 0x400    # addr = 0x400
        add $r14, $r0, $r1, 0x500    # end  = 0x500 (exclusive)
FLUSH_LOOP:
        beq $r1, $r15, $r14, END
        add $r0,  $r0,  $r0, 0       # delay slot

        lw  $r12, $r15, $r0, 0       # dummy read (evicts matching C line)
        add $r15, $r15, $r1, 1       # addr++
        beq $r1,  $r0,  $r0, FLUSH_LOOP
        add $r0,  $r0,  $r0, 0       # delay slot

END:    halt $r0, $r0, $r0, 0
