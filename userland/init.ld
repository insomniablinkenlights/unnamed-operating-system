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
		QUAD(0);
		QUAD(0);
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
