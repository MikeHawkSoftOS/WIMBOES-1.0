ENTRY(_start)
SECTIONS {
    . = 0x10000;
    .text : { *(.text.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { __bss_start = .; *(.bss*) *(COMMON) __bss_end = .; }
    /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
