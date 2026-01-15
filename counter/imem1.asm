add $r2, $zero, $imm, 0      # Base addr = 0 
add $r3, $zero, $imm, 1      # Core ID
add $r4, $zero, $imm, 128    # Loop limit = 128
add $r10, $zero, $imm, 4     # Jump target 
lw $r5, $r2, $imm, 1        
bne $r10, $r5, $r3, 0       
lw $r6, $r2, $imm, 0        
add $r6, $r6, $imm, 1        # Increment counter
sw $r6, $r2, $imm, 0         # Store counter
add $r7, $r3, $imm, 1        # CoreID + 1
and $r7, $r7, $imm, 3        
sw $r7, $r2, $imm, 1         
sub $r4, $r4, $imm, 1        # Decrement loop counter
bne $r10, $r4, $zero, 0      
add $r9, $zero, $imm, 512    # Conflict miss
lw $zero, $r9, $imm, 0       
halt $zero, $zero, $zero, 0 