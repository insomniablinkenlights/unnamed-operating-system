OUTPUT_FORMAT(binary)
SECTIONS
{
	.  = 0x0;
	.text.start (0):{
		boot.o ( .text )
	}
	.text:ALIGN(512): {
		_KERNEL_START_ = .;
			realk.o (.text)
			a20.o (.text)
			protk.o (.text)
			longk.o (.text)
		_KERNEL_END = .;
		_KERNEL_SECTORS = ABSOLUTE(.)/0x200;
		. = ALIGN(0x1000);
		end.o ( .text)
/*		_PML4T_A = .*/
	}
	/*.text:ALIGN(512):{

	}*/
}
OUTPUT(Kernel.bin)
