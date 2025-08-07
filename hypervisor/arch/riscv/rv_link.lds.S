#include <asm/cache.h>
#include <asm/page.h>
#include <asm/mem.h>
#undef ENTRY
#undef ALIGN

ENTRY(_start)

PHDRS
{
    text PT_LOAD ;
}

SECTIONS
{
    . = ACRN_VIRT_START;
    .text : {
        _stext = .;
        *(.text)
        _etext = .;
    } :text = 0x9090

    .data : {
    } :text

    . = ALIGN(PAGE_SIZE);             /* Init code and data */
    .init.text : {
        *(.init.text)
    } :text

    . = ALIGN(PAGE_SIZE);
    .init.data : {
        *(.init.data)
    } :text

    . = ALIGN(STACK_SIZE);
    _end = . ;
}

ASSERT( _end_boot - _start <= PAGE_SIZE, "Boot code is larger than 4K")
