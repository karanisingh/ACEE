    .global main
    

add:
    bl  multiply      /* Call multiply function */
    add r0, r0, r1    /* Add the arguments */
    pop {r0, r1, lr}
    bx  lr           /* Return from add */

multiply:
    mul r0, r0, r1  /* Multiply the arguments */
    bx  lr          /* Return from multiply */

main:
    mov r0, #5      /* First argument */
    mov r1, #3      /* Second argument */
    ace r0
    ace r1 
    ace_begin
    ldr r0, [sp]    /* Load r0 from stack location pointed by sp */
    bl  add         /* Call add function */
    ace_end
    pop {ip, lr}
    out r0
    bx  lr          /* Return from main */
