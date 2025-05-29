OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80200000,
   * where the BL33 header expects it to be.
   */
  . = 0x80200000;
  .text :
  {
    *(.text)
    . = ALIGN(0x1000);
    *(trampsec)
  }

  . = ALIGN(0x1000);
  PROVIDE(etext = .);

  /*
   * make sure end is after data and bss.
   */
  .data : {
    *(.data)
  }
  .bss : {
    *(.bss)
    *(.sbss*)
     PROVIDE(end = .);
  }
}
