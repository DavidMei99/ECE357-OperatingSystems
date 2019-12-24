; ECE-357 2019 Fall (Problem Set#7)
; Di Mei
; OS: MacOS 10.10.2
; Arch: Windows X86-64

SECTION .data
 
msg: db "Hello, World!", 0x0a
len: equ $-msg
 
SECTION .text

global _main
 
_main:
    mov    rax, 0x2000004  ;syscall number of sys_write
    mov    rdi, 1          ;stdout
    mov    rsi, msg        ;string "Hello World!"
    mov    rdx, len        ;length of string
    syscall
 
    mov    rdi, rax        ;save exit code
    mov    rax, 0x2000001  ;syscall number of sys_exit
    syscall
