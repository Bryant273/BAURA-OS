bits 32

section .multiboot
    align 4
    dd 0x1BADB002
    dd 0x3
    dd -(0x1BADB002 + 0x3)

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    and esp, 0xfffffff0
    sub esp, 8
    mov [esp], eax
    mov [esp + 4], ebx
    call kernel_main

.halt:
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
