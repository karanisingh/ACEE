_Z14simple_pointerPiS_:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #12
        str     r0, [fp, #-8]
        str     r1, [fp, #-12]
        ldr     r3, [fp, #-8]
        ldr     r2, [r3]
        ldr     r3, [fp, #-12]
        ldr     r3, [r3]
        cmp     r2, r3
        bge     .L2
        ldr     r3, [fp, #-8]
        ldr     r3, [r3]
        b       .L3
.L2:
        ldr     r3, [fp, #-12]
        ldr     r2, [r3]
        ldr     r3, [fp, #-8]
        ldr     r3, [r3]
        cmp     r2, r3
        bge     .L4
        ldr     r3, [fp, #-12]
        ldr     r3, [r3]
        b       .L3
.L4:
        ldr     r3, [fp, #-8]
        ldr     r3, [r3]
.L3:
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #8
        sub     r2, fp, #12
        sub     r3, fp, #8
        mov     r1, r2
        mov     r0, r3
        bl      _Z14simple_pointerPiS_
        mov     r3, r0
        nop
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr