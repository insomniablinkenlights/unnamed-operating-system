OUTPUT_FORMAT(binary)
SECTIONS
{
	.  = 0x0;
	.text.start (0):{
		sh.o (.text)
	}
}
OUTPUT(sh.bin)
