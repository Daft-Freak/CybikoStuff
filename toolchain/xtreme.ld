ENTRY(_start)

MEMORY
{
    /* internal */
    rom        (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00010000
    ram        (rwx) : ORIGIN = 0x00FFDC00, LENGTH = 0x00002000

    /* external */
    ext_ram_lo (rwx) : ORIGIN = 0x00400000, LENGTH = 0x00080000
    ext_ram    (rwx) : ORIGIN = 0x00480000, LENGTH = 0x00180000 /*bootloader loads program here*/
    flash      (rx)  : ORIGIN = 0x00600000, LENGTH = 0x00080000
}

SECTIONS
{
    .header : { LONG(0x1234ABCD) LONG(_start) } > ext_ram

    .text :
    {
        KEEP (*(.init))
        *(.text*)
    } > ext_ram

    .rodata :
    {
        *(.rodata*)
    } > ext_ram

    .data :
    {
        _sdata = .;
        *(.data*)
        _edata = .;

        PROVIDE_HIDDEN (___preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (___preinit_array_end = .);

        PROVIDE_HIDDEN (___init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE_HIDDEN (___init_array_end = .);

        PROVIDE_HIDDEN (___fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP (*(.fini_array))
        PROVIDE_HIDDEN (___fini_array_end = .);

        _end = .;
    } > ext_ram

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > ext_ram

    /* stack in on-chip ram */
    .stack :
    {
        *(.stack)
    } > ram

    _stack = ORIGIN(ram) + LENGTH(ram);
}

