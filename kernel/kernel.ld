OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  /*
   * the link base is supplied by the Makefile:
   *   --defsym=_kbase=0x80000000  (QEMU)
   *   --defsym=_kbase=0x80200000  (Duo, loaded via U-Boot `go`)
   */
  . = _kbase;
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
    PROVIDE(_bss_start = .);
    *(.bss)
    *(.sbss*)
    PROVIDE(_bss_end = .);
  }

  .stack (NOLOAD) : {
    *(.stack)
    PROVIDE(end = .);
  }
}
