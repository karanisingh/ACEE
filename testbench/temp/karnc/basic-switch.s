_Z12basic_switchc:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #20
        mov     r3, r0
        strb    r3, [fp, #-13]
        mvn     r3, #0
        str     r3, [fp, #-8]
        ldrb    r3, [fp, #-13]  @ zero_extendqisi2
        sub     r3, r3, #97
        cmp     r3, #17
        ldrls   pc, [pc, r3, asl #2]
        b       .L10
        .word   .L7
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L6
        .word   .L10
        .word   .L10
        .word   .L5
        .word   .L10
        .word   .L10
        .word   .L10
        .word   .L3
.L6:
        mov     r3, #0
        str     r3, [fp, #-8]
        b       .L8
.L7:
        mov     r3, #1
        str     r3, [fp, #-8]
        b       .L8
.L3:
        mov     r3, #2
        str     r3, [fp, #-8]
        b       .L8
.L5:
        mov     r3, #4
        str     r3, [fp, #-8]
        b       .L8
.L10:
        nop
.L8:
        ldr     r3, [fp, #-8]
        mov     r0, r3
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #8
        mov     r3, #97
        strb    r3, [fp, #-5]
        ldrb    r3, [fp, #-5]   @ zero_extendqisi2
        mov     r0, r3
        bl      _Z12basic_switchc
        str     r0, [fp, #-12]
        ldr     r3, [fp, #-12]
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr