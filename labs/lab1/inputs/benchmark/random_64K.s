# random access using linear feedback shift register
.text
    lui     $s0, 0x1000     # $s0 = 0x10000000 array base address
    addi    $s1, $0, 0      # i = 0
    addi    $s2, $0, 42     # r = 42
    addi    $s3, $0, 3
    nor     $s3, $0, $s3    # alignment mask
    addi    $s4, $0, 16384  # $t2 = 16384

loop:
    slt     $t0, $s1, $s4   # i < 16384 
    beq     $t0, $0,  done  # if not then done

    add     $t1, $s2, $s0   # address of a[r]
    and     $t1, $t1, $s3   # align address
    lw      $t2, 0($t1),    # $t2 = a[r]
    
    andi    $t3, $s2, 1	    # read LSB of r
    srl     $s2, $s2, 1     # right shift r by 1 bit
    addi    $s1, $s1, 1     # i = i + 1
    beq	    $t3, $0, loop   # if LSB=0 go to loop
    xori    $s2, $s2, 13568 # set random bits
    j       loop            # repeat

done:
    addiu   $v0, $0, 10
    syscall