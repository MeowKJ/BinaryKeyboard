ENTRY( _start )

MEMORY
{
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 4K
	RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 26K
}

PROVIDE( __global_pointer$ = 0x20004800 );

SECTIONS
{
	.init :
	{
		_sinit = .;
		. = ALIGN(4);
		KEEP(*(SORT_NONE(.init)))
		. = ALIGN(4);
		_einit = .;
	} >FLASH AT>FLASH

	.vector :
	{
		. = ALIGN(4);
		KEEP(*(.vector))
		. = ALIGN(4);
	} >FLASH AT>FLASH

	.vector_handler :
	{
		. = ALIGN(4);
		KEEP(*(.vector_handler))
		. = ALIGN(4);
	} >FLASH AT>FLASH

	.handle_reset :
	{
		. = ALIGN(4);
		KEEP(*(SORT_NONE(.handle_reset)))
		. = ALIGN(4);
	} >FLASH AT>FLASH
}
