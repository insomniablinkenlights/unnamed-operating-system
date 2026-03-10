OUTPUT_FORMAT(binary)
SECTIONS
{
	. = 0x0;
	.text.start (0):{
		QUAD(L);
		QUAD(K);
		QUAD(L);
		QUAD(0);
		QUAD(0);
		QUAD(K);
		QUAD(0x2000);
		QUAD(0x2F000);
	}
	L = .;
	.text (.):{
		init.o (.text)
	}
	K = .;
	.data (.):{
		init.o (.data)
	}
	M = .;
}
OUTPUT(init.bin)
