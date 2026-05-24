/*
 * Linker script for STM32F103C8T6 (Blue Pill)
 *   Flash: 64 KiB at 0x08000000
 *   RAM:   20 KiB at 0x20000000
 *
 * Compatible with libopencm3 startup code (vector_table symbol).
 */

ENTRY(reset_handler)

MEMORY
{
    rom  (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

_stack = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
    .text : {
        *(.vectors)          /* libopencm3 vector table */
        *(.text .text.*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        _etext = .;
    } > rom

    /* .data is loaded from ROM and copied to RAM by startup code */
    .data : AT(ADDR(.text) + SIZEOF(.text)) {
        _data_start = .;
        *(.data .data.*)
        . = ALIGN(4);
        _data_end = .;
    } > ram

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > ram

    /* Discard C++ exception tables (not used in firmware) */
    /DISCARD/ : { *(.eh_frame) }
}
