global SysNtCreateFile

section .text
SysNtCreateFile:
    mov     r10, rcx
    mov     eax, 0x55     ; NtCreateFile syscall number
    syscall
    ret
