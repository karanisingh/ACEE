basic_switch:
    push    {r7}
    sub     sp, sp, #20
    add     r7, sp, #0
    mov     r3, r0
    strb    r3, [r7, #7]
    mov     r3, #-1
    str     r3, [r7, #12]
    ldrb    r3, [r7, #7]@ zero_extendqisi2
    subs    r3, r3, #97
    cmp     r3, #17
    bhi     .L10
    adr     r2, .L4
    ldr     pc, [r2, r3, lsl#2]
.L4:
    .word .L7+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L6+1
    .word .L10+1
    .word .L10+1
    .word .L5+1
    .word .L10+1
    .word .L10+1
    .word .L10+1
    .word .L3+1
.L6:
    movs    r3, #0
    str     r3, [r7, #12]
    b       .L8
.L7:
    movs    r3, #1
    str     r3, [r7, #12]
    b       .L8
.L3:
    movs    r3, #2
    str     r3, [r7, #12]
    b       .L8
.L5:
    movs    r3, #4
    str     r3, [r7, #12]
    b       .L8
.L10:
    nop
.L8:
    ldr     r3, [r7, #12]
    mov     r0, r3
    adds    r7, r7, #20
    mov     sp, r7
    ldr     r7, [sp], #4
    bx      lr
MAIN:
    push    {r7, lr}
    mov     r0, #8

    ace     r0
    ace_begin
    bl      basic_switch
    out     r0
    ace_end


    pop     {r7, pc}
