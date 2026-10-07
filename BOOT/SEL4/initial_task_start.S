.section .text.start,"ax",@progbits
.global _start
.type _start,@function
_start:
    leaq __baura_stack_top(%rip), %rsp
    andq $-16, %rsp
    xorq %rbp, %rbp
    call baura_sel4_initial_task
.Lhalt:
    pause
    jmp .Lhalt
.size _start, .-_start

.section .bss.stack,"aw",@nobits
.align 16
.global __baura_stack_top
__baura_stack_bottom:
    .skip 65536
__baura_stack_top:

.section .note.GNU-stack,"",@progbits
