OUTPUT_FORMAT(binary)
SECTIONS
{
	.  = 0x0;
	.text.start (0):{
		boot.o ( .text )
	}
	.text 0x7e00 : AT(0x200){
		_KERNEL_START_ = .;
			realk.o (.text)
			a20.o (.text)
			protk.o (.text)
		_KPP_ = ALIGN(0x1000)-0x7c00;
	}
	.text 0x8000007c00+_KPP_ : AT(_KPP_){
			longk.o (.text)
			pci.o (.text)
			chs.o (.text)
			asmbridge.o (.text)
			idt.o (.text)
			page.o (.text)
			proc.o (.text)
			malloc.o (.text)
			idt_bridge.o (.text)
			memcpy.o (.text)
			filesystem.o (.text)
			usermode.o (.text)
			syscall.o (.text)
			terminal.o (.text)
			ps2.o (.text)
			string.o (.text)
			flat.o (.text)
		_KERNEL_END_ = .;
		_KERNEL_SECTORS = ABSOLUTE(.-0x8000007c00)/0x200;
		/*. = ALIGN(0x1000);
		end.o ( .text)*/
/*		_PML4T_A = .*/
	}
	/*.text:ALIGN(512):{

	}*/
}
OUTPUT(Kernel.bin)
