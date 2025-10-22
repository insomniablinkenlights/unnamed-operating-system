floppya.img: build/Kernel.bin
	dd if="build/Kernel.bin" of=floppya.img conv=notrunc
build/Kernel.bin: build/page.o build/asmbridge.o build/idt.o build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o test.ld
	cd build && ld -T ../test.ld
build/realk.o: realk.S
	as realk.S -o build/realk.o
build/protk.o: protk.S
	as protk.S -o build/protk.o
build/longk.o: longk.S
	as longk.S -o build/longk.o
build/boot.o: boot.S
	as boot.S -o build/boot.o
build/a20.o: a20.S
	as a20.S -o build/a20.o
build/end.o: end.S
	as end.S -o build/end.o
build/pci.o: pci.S
	as pci.S -o build/pci.o
build/asmbridge.o: asmbridge.S
	as asmbridge.S -o build/asmbridge.o
build/chs.o: build/chs.s
	as build/chs.s -o build/chs.o
build/idt.o: build/idt.s
	as build/idt.s -o build/idt.o
build/chs.s: chs.c headers/stdint.h
	gcc chs.c -S -o build/chs.s -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm
build/page.s: page.c headers/stdint.h
	gcc page.c -S -o build/page.s -Wall -Wextra -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm
build/page.o: build/page.s
	as build/page.s -o build/page.o
build/idt.s: idt.c headers/stdint.h
	gcc idt.c -S -o build/idt.s -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm
clean:
	rm build/page.o build/page.s build/chs.s build/idt.s build/asmbridge.o build/idt.o build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o build/Kernel.bin
