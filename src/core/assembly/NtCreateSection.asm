
global SysNtCreateSection

section .text
SysNtCreateSection:
    mov     r10, rcx        ; NtCreateSection
    mov     eax, 4Ah
    syscall
    ret