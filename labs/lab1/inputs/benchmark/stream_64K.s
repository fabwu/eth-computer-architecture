# int i;
# int array[16384];
#
# for (i = 0; i < 16384; i = i + 1) {
#   array[i] = 42
# }
.text
    lui     $s0, 0x1000     # $s0 = 0x10000000 array base address
    addi    $s1, $0, 0      # i = 0
    addi    $t2, $0, 16384  # $t2 = 16384

loop:
    slt     $t0, $s1, $t2   # i < 16384 
    beq     $t0, $0,  done  # if not then done
    sll     $t0, $s1, 2     # $t0 = i * 4 (byte offset)
    add     $t0, $t0, $s0   # address of array[i]
    addi    $t1, $0, 42     # $t1 = 42
    sw      $t1, 0($t0)     # array[i] = $t1
    addi    $s1, $s1, 1     # i = i + 1
    j       loop            # repeat

done:
    addiu   $v0, $0, 10
    syscall
