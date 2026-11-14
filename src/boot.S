/* BIOS boot sector: loads the kernel (LBA 1..) to 0x10000, picks a 1024x768x32 VBE
 * linear-framebuffer mode, enters 32-bit protected mode and jumps to the kernel. */
.intel_syntax noprefix
.code16
.global _boot
_boot:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl
    mov word ptr [dap_lba], 1
    mov word ptr [dest_seg], 0x1000
    mov ax, offset KSECTORS
load:
    test ax, ax
    jz loaded
    mov cx, ax
    cmp cx, 32
    jbe 1f
    mov cx, 32
1:  mov [dap_cnt], cx
    mov bx, [dest_seg]
    mov [dap_seg], bx
    push ax
    push cx
    mov dl, [boot_drive]
    mov si, offset dap
    mov ah, 0x42
    int 0x13
    jc fail_d
    pop cx
    pop ax
    sub ax, cx
    add [dap_lba], cx
    shl cx, 5
    add [dest_seg], cx
    jmp load
loaded:
    mov di, 0x8000
    mov dword ptr [di], 0x32454256        /* "VBE2" */
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne fail_v
    mov word ptr [want_w], 1024
    mov word ptr [want_h], 768
scan:
    mov si, [0x800E]
    mov ax, [0x8010]
    mov fs, ax
next:
    mov cx, fs:[si]
    add si, 2
    cmp cx, 0xFFFF
    jne 2f
    cmp word ptr [want_w], 1024
    jne fail_m
    mov word ptr [want_w], 800
    mov word ptr [want_h], 600
    jmp scan
2:
    push si
    push cx
    mov ax, 0x4F01
    mov di, 0x9000
    int 0x10
    pop cx
    pop si
    cmp ax, 0x004F
    jne next
    test byte ptr [0x9000], 0x80
    jz next
    mov ax, [0x9012]
    cmp ax, [want_w]
    jne next
    mov ax, [0x9014]
    cmp ax, [want_h]
    jne next
    cmp byte ptr [0x9019], 32
    jne next
    mov bx, cx
    or bx, 0x4000
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne fail_s
    cli
    lgdt [gdtr]
    in al, 0x92
    or al, 2
    out 0x92, al
    mov eax, cr0
    or al, 1
    mov cr0, eax
    .byte 0x66, 0xEA
    .long pm
    .word 0x08
fail_d: mov al, 'D'
    jmp fail
fail_v: mov al, 'V'
    jmp fail
fail_m: mov al, 'M'
    jmp fail
fail_s: mov al, 'S'
fail:
    mov ah, 0x0E                          /* print one letter and halt */
    xor bx, bx
    int 0x10
    cli
1:  hlt
    jmp 1b

.code32
pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9F000
    mov eax, 0x10000
    jmp eax

.code16
.align 4
gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdtr:
    .word 23
    .long gdt
dap:
    .byte 0x10, 0
dap_cnt: .word 0
dap_off: .word 0
dap_seg: .word 0
dap_lba: .long 1
         .long 0
dest_seg: .word 0
want_w: .word 0
want_h: .word 0
boot_drive: .byte 0
.org 510
.word 0xAA55
