
@ handmade program
// fuck
    .global main

add:
    push {r2}   // fuck
    add r2, r0, r1
    mov r0, r2
    pop {r2}
    bx lr

main:
    mov r0, #100
    mov r1, #23
    mov r2, #2
    mov r3, #3
    push {r3}
    push {r2}
    mov r2, #6
    pop {r2}
    out r2                          @ r2 == 2
    str r2, [r1]                    @ memory[23] == 2
    bl add
    out r0                          @ r0 == 123
    pop {r7, r8}                    @ r8 == r2 == 2....      r7 == r3 == 3 ERROR HERE               
    out r7                          @ r7 == 3
    out r8                          @ r8 == 2
    ldr r10, [r1]                   
    out r10                         @ r10 == 2