floppya.img: build/Kernel.bin
	dd if="build/Kernel.bin" of=floppya.img conv=notrunc
build/Kernel.bin: build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o test.ld
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
build/chs.o: build/chs.s
	as build/chs.s -o build/chs.o
build/chs.s: chs.c
	gcc chs.c -S -o build/chs.s -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm
clean:
	rm build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o build/Kernel.bin
