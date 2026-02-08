ENTRY(_start)
SECTIONS {
    . = 0x100000;
    .text : { *(.text*) }
    . = ALIGN(0x1000);
    .data : { *(.data*) *(.rodata*) }
    . = ALIGN(0x1000);
    .bss : { *(.bss*) *(COMMON) }
}
