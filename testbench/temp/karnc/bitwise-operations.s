_Z7bitwiseii:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #20
        str     r0, [fp, #-16]
        str     r1, [fp, #-20]
        mov     r3, #0
        str     r3, [fp, #-8]
        ldr     r2, [fp, #-16]
        ldr     r3, [fp, #-20]
        eor     r3, r3, r2
        and     r3, r3, #1
        cmp     r3, #0
        beq     .L2
        ldr     r3, [fp, #-8]
        orr     r3, r3, #1
        str     r3, [fp, #-8]
        b       .L3
.L2:
        ldr     r3, [fp, #-8]
        bic     r3, r3, #1
        str     r3, [fp, #-8]
.L3:
        ldr     r2, [fp, #-16]
        ldr     r3, [fp, #-20]
        and     r3, r3, r2
        and     r3, r3, #2
        cmp     r3, #0
        beq     .L4
        ldr     r3, [fp, #-8]
        lsl     r3, r3, #2
        str     r3, [fp, #-8]
        b       .L5
.L4:
        ldr     r3, [fp, #-8]
        asr     r3, r3, #2
        str     r3, [fp, #-8]
.L5:
        ldr     r2, [fp, #-16]
        ldr     r3, [fp, #-20]
        orr     r3, r2, r3
        and     r3, r3, #4
        cmp     r3, #0
        beq     .L6
        ldr     r3, [fp, #-8]
        eor     r3, r3, #15
        str     r3, [fp, #-8]
.L6:
        ldr     r3, [fp, #-8]
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #8
        ldr     r1, [fp, #-12]
        ldr     r0, [fp, #-8]
        bl      _Z7bitwiseii
        mov     r3, r0
        nop
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr