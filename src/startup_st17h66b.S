.syntax unified
.cpu cortex-m0
.thumb

.section .vectors, "a", %progbits
.align 2
.global __melb_vectors
.type __melb_vectors, %object

__melb_vectors:
    .word __stack_top__
    .word Reset_Handler
    .rept 46
    .word Default_Handler
    .endr

.size __melb_vectors, . - __melb_vectors

.section .text.Reset_Handler, "ax", %progbits
.align 2
.global Reset_Handler
.type Reset_Handler, %function
.thumb_func

Reset_Handler:
    ldr r0, =__data_load__
    ldr r1, =__data_start__
    ldr r2, =__data_end__

1:
    cmp r1, r2
    bcs 2f
    ldr r3, [r0]
    str r3, [r1]
    adds r0, #4
    adds r1, #4
    b 1b

2:
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__
    movs r3, #0

3:
    cmp r1, r2
    bcs 4f
    str r3, [r1]
    adds r1, #4
    b 3b

4:
    bl main

5:
    b 5b

.size Reset_Handler, . - Reset_Handler

.section .text.Default_Handler, "ax", %progbits
.align 2
.global Default_Handler
.type Default_Handler, %function
.thumb_func

Default_Handler:
    b Default_Handler

.size Default_Handler, . - Default_Handler
