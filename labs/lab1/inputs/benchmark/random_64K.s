# write sequentially
# read random access using linear feedback shift register
.text
    lui     $s0, 0x1000     # $s0 = 0x10000000 array base address
    addi    $s1, $0, 0      # i = 0
    addi    $s2, $0, 42     # r = 42
    addi    $s3, $0, 3
    nor     $s3, $0, $s3    # alignment mask
    addi    $s4, $0, 16384  # $s4 = 16384
    
write:
    slt     $t0, $s1, $s4   # i < 16384 
    beq     $t0, $0,  read  # if not then done
    sll     $t0, $s1, 2     # $t0 = i * 4 (byte offset)
    add     $t0, $t0, $s0   # address of array[i]
    addi    $t1, $0, 42     # $t1 = 42
    sw      $t1, 0($t0)     # array[i] = $t1
    addi    $s1, $s1, 1     # i = i + 1
    j       write           # repeat

read:
    slt     $t0, $s5, $s4   # i < 16384 
    beq     $t0, $0,  done  # if not then done

    add     $t6, $s2, $s0   # address of a[r]
    and     $t6, $t6, $s3   # align address
    lw      $t5, 0($t6),    # $t5 = a[r]
    
    andi    $t4, $s2, 1	    # read LSB of r
    srl     $s2, $s2, 1     # right shift r by 1 bit
    addi    $s5, $s5, 1     # i = i + 1
    beq	    $t4, $0, read   # if LSB=0 go to loop
    xori    $s2, $s2, 13568 # set random bits
    j       read            # repeat

done:
    addiu   $v0, $0, 10
    syscall
