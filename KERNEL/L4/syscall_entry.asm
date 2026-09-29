
global syscall_entry
extern handle_syscall
syscall_entry:
    push rax
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    mov rdi, rax
    mov rsi, rsp
    call handle_syscall
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax
    sysretq
