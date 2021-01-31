# int i;
# int array[16384];
# int x;
#
# for (i = 0; i < 16384; i = i + 4) {
#   array[i] = 42
# }
#
# for (i = 0; i < 16384; i = i + 4) {
#   x = array[i];
# }
.text
    lui     $s0, 0x1000     # $s0 = 0x10000000 array base address
    addi    $s1, $0, 0      # i = 0
    addi    $s2, $0, 0      # j = 0
    addi    $t2, $0, 16384  # $t2 = 16384

write:
    slt     $t0, $s1, $t2   # i < 16384 
    beq     $t0, $0,  read  # if not then done
    sll     $t0, $s1, 2     # $t0 = i * 4 (byte offset)
    add     $t0, $t0, $s0   # address of array[i]
    addi    $t1, $0, 42     # $t1 = 42
    sw      $t1, 0($t0)     # array[i] = $t1
    addi    $s1, $s1, 4     # i = i + 1
    j       write           # repeat

read:
    slt     $t0, $s2, $t2   # i < 16384 
    beq     $t0, $0,  done  # if not then done
    sll     $t0, $s2, 2     # $t0 = i * 4 (byte offset)
    add     $t0, $t0, $s0   # address of array[i]
    lw      $t1, 0($t0)     # array[i] = $t1
    addi    $s2, $s2, 4     # i = i + 1
    j       read           # repeat

done:
    addiu   $v0, $0, 10
    syscall
