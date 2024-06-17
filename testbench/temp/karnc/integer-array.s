_Z13integer_arrayPi:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #20
        str     r0, [fp, #-16]
        mov     r3, #0
        str     r3, [fp, #-8]
        mov     r3, #0
        str     r3, [fp, #-12]
        b       .L2
.L3:
        ldr     r3, [fp, #-12]
        lsl     r3, r3, #2
        ldr     r2, [fp, #-16]
        add     r3, r2, r3
        ldr     r3, [r3]
        ldr     r2, [fp, #-8]
        add     r3, r2, r3
        str     r3, [fp, #-8]
        ldr     r3, [fp, #-12]
        add     r3, r3, #1
        str     r3, [fp, #-12]
.L2:
        ldr     r3, [fp, #-12]
        cmp     r3, #9
        ble     .L3
        ldr     r3, [fp, #-8]
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
.LC0:
        .word   0
        .word   1
        .word   2
        .word   3
        .word   4
        .word   5
        .word   6
        .word   7
        .word   8
        .word   9
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #48
        ldr     r3, .L9
        sub     ip, fp, #48
        mov     lr, r3
        ldmia   lr!, {r0, r1, r2, r3}
        stmia   ip!, {r0, r1, r2, r3}
        ldmia   lr!, {r0, r1, r2, r3}
        stmia   ip!, {r0, r1, r2, r3}
        ldm     lr, {r0, r1}
        stm     ip, {r0, r1}
        sub     r3, fp, #48
        mov     r0, r3
        bl      _Z13integer_arrayPi
        str     r0, [fp, #-8]
        ldr     r3, [fp, #-8]
        and     r3, r3, #1
        cmp     r3, #0
        bne     .L6
        mov     r3, #1
        b       .L8
.L6:
        mov     r3, #0
.L8:
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr
.L9:
        .word   .LC0