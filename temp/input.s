_Z3addii:
        out     sp
        str     fp, [sp, #-4]!
        out     sp
        add     fp, sp, #0
        sub     sp, sp, #12
        str     r0, [fp, #-8]
        str     r1, [fp, #-12]
        ldr     r2, [fp, #-8]
        ldr     r3, [fp, #-12]
        add     r3, r2, r3
        mov     r0, r3
        out     r0
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
main:
        push    {fp, lr}
        out     fp
        out     lr
        add     fp, sp, #4
        out     fp
        sub     sp, sp, #16
        mov     r3, #5
        str     r3, [fp, #-8]
        mov     r3, #3
        str     r3, [fp, #-12]
        ldr     r1, [fp, #-12]
        ldr     r0, [fp, #-8]
        bl      _Z3addii
        out     r0
        str     r0, [fp, #-16]
        mov     r3, #0
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr