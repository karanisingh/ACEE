.global main

@
@   int test(int x, int y)
@   {
@       int z = 2*y;
@       if(x == 100000)
@       {
@           if(x < z)
@               return 111;
@           return 100;
@       }
@       return 10;
@   }


@   int main()
@   {
@       int x = 7;
@       int y = 22;
@       printf("%d", test(x, y));
@       return 1;
@   }



@   Outputs
@       10 if x != 100000
@       100 if x == 100000 && x >= z
@       111 if x == 100000 && x < z
@

test:
    push {lr}
    mul r2, r1, #2  @ lsl r2, r1, #1  @
    mov r5, #10
    mov r6, #100
    mov r7, #111
    mov r3, #100000
    cmp r0, r3
    bne cond_1
    cmp r0, r2
    bge cond_2
    b cond_3
cond_1:
    mov r0, r5
    b end_f
cond_2:
    mov r0, r6
    b end_f
cond_3:
    mov r0, r7
    b end_f
end_f:
    bx lr


main:
    push    {ip, lr}
    mov r0, #7
    mov r1, #22

    @ Signify the inputs to the function under test
    ace r0              @ NEW INSTRUCTION
    ace r1              @ NEW INSTRUCTION

    @ Signify beginning of the function under test
    ace_begin           @ NEW INSTRUCTION
    
    bl test

    @ Print result, signify end of function under test
    out r0
    ace_end             @ NEW INSTRUCTION 
    
    out r0              @ NEW INSTRUCTION

    pop {ip, lr}
    bx  lr          
