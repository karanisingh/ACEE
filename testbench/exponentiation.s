@ int exponentiation(int x, int y)
@ {
@     int result = 1;
@     for(int i = 0;i < y; ++i)
@         result *= x;

@     return result;
@ }





exponentiation:
        mov     r4, #1
        mov     r3, #0
loop:
        @ bounds checking
        cmp     r0, #0
        blt     end
        cmp     r1, #0
        blt     end

        cmp     r3, r1
        beq     end
        mul     r4, r4, r0
        add     r3, r3, #1
        b       loop
end:
        mov     r0, r4
        bx      lr

main:
        push    {fp, lr}

        ace     r0
        ace     r1 
        ace_begin
        bl      exponentiation
        out     r0
        ace_end
        bx      lr