_Z14exponentiationii:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #20
        str     r0, [fp, #-16]
        str     r1, [fp, #-20]
        mov     r3, #1
        str     r3, [fp, #-8]
        mov     r3, #0
        str     r3, [fp, #-12]
        b       .L2
.L3:
        ldr     r3, [fp, #-8]
        ldr     r2, [fp, #-16]
        mul     r3, r2, r3
        str     r3, [fp, #-8]
        ldr     r3, [fp, #-12]
        add     r3, r3, #1
        str     r3, [fp, #-12]
.L2:
        ldr     r2, [fp, #-12]
        ldr     r3, [fp, #-20]
        cmp     r2, r3
        blt     .L3
        ldr     r3, [fp, #-8]
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #8
        mov     r3, #2
        str     r3, [fp, #-8]
        mov     r3, #10
        str     r3, [fp, #-12]
        ldr     r1, [fp, #-12]
        ldr     r0, [fp, #-8]
        bl      _Z14exponentiationii
        mov     r3, r0
        out     r3
        nop
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr