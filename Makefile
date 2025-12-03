CFLAGS=-static -fno-pic -fplt -m64 -mcmodel=large -Wall -Wextra -Wpedantic -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -mno-red-zone -mgeneral-regs-only -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -fno-builtin
floppya.img: build/Kernel.bin
	dd if="build/Kernel.bin" of=floppya.img conv=notrunc
build/Kernel.bin: build/memcpy.o build/idt_bridge.o build/malloc.o build/proc.o build/page.o build/asmbridge.o build/idt.o build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o test.ld
	cd build && ld -T ../test.ld
build/%.o: %.S
	as $< -o $@
build/%.o: build/%.s
	as $< -o $@

build/chs.s: chs.c headers/stdint.h
	gcc $(CFLAGS) chs.c -S -o build/chs.s 
build/page.s: page.c headers/stdint.h headers/addresses.h
	gcc $(CFLAGS) page.c -S -o build/page.s 
build/malloc.s: malloc.c headers/stdint.h headers/addresses.h
	gcc $(CFLAGS) malloc.c -S -o build/malloc.s 
build/proc.s: proc.c headers/stdint.h headers/addresses.h
	gcc $(CFLAGS) proc.c -S -o build/proc.s -Wno-pointer-arith
build/idt.s: idt.c headers/stdint.h headers/idt.h
	gcc $(CFLAGS) idt.c -S -o build/idt.s 
clean:
	rm build/idt_bridge.o build/malloc.o build/malloc.s build/page.o build/page.s build/chs.s build/idt.s build/asmbridge.o build/idt.o build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o build/Kernel.bin
