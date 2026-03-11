OUTPUT_FORMAT(binary)
SECTIONS
{
	.  = 0x0;
	BSS_SIZE = 0x2000;
	BSS_VADD = 0x2F000;
	.text.start (0):{
		QUAD(TEXT_START);
		QUAD(TEXT_END);
		QUAD(TEXT_VADD);
		QUAD(DATA_START);
		QUAD(DATA_END);
		QUAD(DATA_VADD);
		QUAD(BSS_SIZE);
		QUAD(BSS_VADD);
	}
	TEXT_START = .;
	.text (.):{
		crt0.o (.text)
		libc.o (.text)
		sh.o (.text)
		int.o (.text)
	}
	TEXT_END = .;
	TEXT_VADD = TEXT_START;
	DATA_START = .;
	.data (.):{
		libc.o (.*data)
		sh.o (.*data)
	}
	DATA_END = .;
	DATA_VADD = DATA_START;
	. = BSS_VADD;
	.bss (.):{ 
		libc.o (.bss) 
		sh.o (.bss)
	}
}
OUTPUT(sh.bin)
