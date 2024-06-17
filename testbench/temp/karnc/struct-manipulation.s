_Z19struct_manipulation8myStruct:
        str     fp, [sp, #-4]!
        add     fp, sp, #0
        sub     sp, sp, #28
        str     r0, [fp, #-16]
        sub     r0, fp, #28
        stm     r0, {r1, r2, r3}
        mov     r3, #0
        str     r3, [fp, #-8]
        ldrb    r3, [fp, #-24]  @ zero_extendqisi2
        cmp     r3, #64
        bls     .L2
        ldrb    r3, [fp, #-24]  @ zero_extendqisi2
        cmp     r3, #90
        bhi     .L2
        ldrb    r3, [fp, #-24]  @ zero_extendqisi2
        sub     r3, r3, #67
        ldr     r2, [fp, #-8]
        add     r3, r2, r3
        str     r3, [fp, #-8]
.L2:
        ldr     r3, [fp, #-8]
        str     r3, [fp, #-28]
        ldr     r3, [fp, #-16]
        mov     ip, r3
        sub     r3, fp, #28
        ldm     r3, {r0, r1, r2}
        stm     ip, {r0, r1, r2}
        ldr     r0, [fp, #-16]
        add     sp, fp, #0
        ldr     fp, [sp], #4
        bx      lr
main:
        push    {fp, lr}
        add     fp, sp, #4
        sub     sp, sp, #24
        sub     r0, fp, #28
        sub     r3, fp, #16
        ldm     r3, {r1, r2, r3}
        bl      _Z19struct_manipulation8myStruct
        ldr     r3, [fp, #-28]
        cmp     r3, #1
        bne     .L5
        mov     r3, #1
        str     r3, [fp, #-20]
        b       .L6
.L5:
        mvn     r3, #0
        str     r3, [fp, #-20]
.L6:
        mov     r3, #1
        mov     r0, r3
        sub     sp, fp, #4
        pop     {fp, lr}
        bx      lr