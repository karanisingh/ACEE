factorial:
        cmp     r0, #1
        beq     end
        

end:
        bx      lr

main:
        push    {fp, lr}
        ace     r0
        ace_begin
        bl      _Z9factoriali
        out     r0
        ace_end
        bx      lr