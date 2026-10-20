ENTRY(_start)

SECTIONS
{
    . = 0x8000;
    .text : { KEEP(*(.text.boot)) *(.text) }
    . = ALIGN(16);
    .rodata : { *(.rodata) }
    . = ALIGN(16);
    .data : { *(.data) }
    . = ALIGN(16);
    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    . = ALIGN(4096);
    . = . + 0x1000;
    __stack_top = .;
}