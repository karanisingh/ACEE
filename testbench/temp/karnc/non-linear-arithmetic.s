_Z21non_linear_arithmetici:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #16
        str     r0, [fp, #-16]
        ldr     r3, [fp, #-16]
        cmp     r3, #0
        bne     .L2
        mov     r3, #2
        str     r3, [fp, #-16]
.L2:
        ldr     r3, [fp, #-16]
        mul     r2, r3, r3
        ldr     r3, [fp, #-16]
        lsl     r3, r3, #3
        add     r3, r2, r3
        sub     r3, r3, #7
        str     r3, [fp, #-8]
        ldr     r1, [fp, #-16]
        ldr     r0, [fp, #-8]
        bl      __aeabi_idiv
        mov     r3, r0
        cmp     r3, #1024
        moveq   r3, #1
        movne   r3, #0
        and     r3, r3, #255
        mov     r2, r3
        ldr     r3, [fp, #-8]
        cmp     r3, r2
        beq     .L3
        mov     r3, #1
        b       .L4
.L3:
        mov     r3, #0
.L4:
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #8
        mov     r3, #0
        str     r3, [fp, #-8]
        ldr     r0, [fp, #-8]
        bl      _Z21non_linear_arithmetici
        mov     r3, r0
        nop
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr