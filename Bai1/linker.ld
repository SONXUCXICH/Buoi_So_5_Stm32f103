ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Interrupt vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);

        *(.text)
        *(.text.*)

        *(.rodata)
        *(.rodata.*)

        *(.glue_7)
        *(.glue_7t)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
    } > FLASH

    /* ARM exception index */
    .ARM.extab :
    {
        *(.ARM.extab*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Constructor arrays */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /* End of Flash content */
    _sidata = LOADADDR(.data);

    /* Initialized variables */
    .data :
    {
        . = ALIGN(4);

        _sdata = .;

        *(.data)
        *(.data.*)

        . = ALIGN(4);

        _edata = .;
    } > RAM AT > FLASH

    /* Zero-initialized variables */
    .bss :
    {
        . = ALIGN(4);

        _sbss = .;

        *(.bss)
        *(.bss.*)
        *(COMMON)

        . = ALIGN(4);

        _ebss = .;
    } > RAM

    /* Heap and stack reservation symbols */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + 0x400;
        . = . + 0x800;
        . = ALIGN(8);
    } > RAM

    /DISCARD/ :
    {
        *(.note.GNU-stack)
    }
}