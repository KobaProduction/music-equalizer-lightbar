/*
 * Experimental ST17H66B bring-up memory layout.
 *
 * The load/run addresses are based on public ST17H66B/PHY62xx reverse-
 * engineering evidence and are NOT hardware-validated on Music-Light-V3-221101.
 * Keep this distinction explicit until the generated image executes on target.
 */

OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

MEMORY
{
    IMAGE (rx)  : ORIGIN = 0x1FFF1838, LENGTH = 0x1F40
    RAM   (rwx) : ORIGIN = 0x1FFF8000, LENGTH = 0x2000
}

SECTIONS
{
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
    } > IMAGE

    .text :
    {
        . = ALIGN(4);
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
    } > IMAGE

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > IMAGE

    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > IMAGE

    __data_load__ = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    __stack_top__ = ORIGIN(RAM) + LENGTH(RAM);
    __image_end__ = LOADADDR(.data) + SIZEOF(.data);

    ASSERT(__image_end__ <= ORIGIN(IMAGE) + LENGTH(IMAGE),
           "ST17H66B bootstrap image overflow")
    ASSERT(__bss_end__ <= __stack_top__,
           "ST17H66B RAM overflow")
}
