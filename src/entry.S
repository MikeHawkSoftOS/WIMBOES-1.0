/* Kernel entry (32-bit protected mode, flat, entered from boot.S at 0x10000) */
    .section .text.entry
    .global _start
_start:
    cli
    mov $0x9F000, %esp
    lgdt gdt_desc
    ljmp $0x08, $1f
1:  mov $0x10, %ax
    mov %ax, %ds; mov %ax, %es; mov %ax, %fs; mov %ax, %gs; mov %ax, %ss
    mov %cr0, %eax
    and $0xFFFFFFF3, %eax        /* clear EM, TS */
    or  $0x22, %eax              /* MP, NE */
    mov %eax, %cr0
    fninit
    mov $__bss_start, %edi
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb
    call kmain
2:  hlt
    jmp 2b

    .text
    .global isr_timer
isr_timer:
    pusha
    call timer_tick
    mov $0x20, %al
    out %al, $0x20
    popa
    iret
    .global isr_exc
isr_exc:
    cli
    call exc_panic
3:  hlt
    jmp 3b

    .data
    .align 8
gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_desc:
    .word 23
    .long gdt
