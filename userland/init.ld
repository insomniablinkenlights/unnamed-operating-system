OUTPUT_FORMAT(binary)
SECTIONS
{
	.  = 0x0;
	.text.start (0):{
		init.o (.text)
	}
}
OUTPUT(init.bin)
