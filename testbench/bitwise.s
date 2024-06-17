bitwise:
    mov         r3, #0 //result = 0
    eor         r4, r0, r1
    and         r4, r4, #1
    cmp         r4, #0
    beq         res_not
    eor         r4, r4, #1 @ result |= 1;
    b           s2
res_not:
    mvn         r4, #1
    and         r3, r3, r4 @ result &= ~1
s2:
    and         r4, r1, r0
    and         r4, r4, #2
    cmp         r4, #0
    beq         resr
    lsl         r3, r3, #2
    b           s3
resr:
    lsr         r3, r3, #2
s3:
    orr         r4, r0, r1
    and         r4, r4, #4
    cmp         r4, #0
    beq         end
    eor         r3, r3, #15
end:
    mov         r0, r3
    bx          lr

main:

    ace         r0
    ace         r1
    ace_begin

    bl          bitwise
    out         r0
    ace_end

    bx          lr
