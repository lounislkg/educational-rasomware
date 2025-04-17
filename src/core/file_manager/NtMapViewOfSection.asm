global SysNtMapViewOfSection

section .text
SysNtMapViewOfSection:
    mov     r10, rcx        ; NtMapViewOfSection / previously NtMapViewOfSectionEx 
    mov     eax, 28h        ; NtMapViewOfSection syscall number (28h) / NtMapViewOfSectionEx syscall number (11Eh)
    syscall
    ret
