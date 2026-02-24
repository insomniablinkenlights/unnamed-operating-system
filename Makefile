CFLAGS=-static -fno-pic -fplt -m64 -mcmodel=large -Wall -Wextra -Wpedantic -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -mno-red-zone -mgeneral-regs-only -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -fno-builtin -ffreestanding
MAKEFLAGS += -j2
floppya.img: build/Kernel.bin build/insertFileSystem.out build/init.bin build/sh.bin
	dd if="/dev/zero" of=floppya.img bs=1KiB count=1440
	dd if="build/Kernel.bin" of=floppya.img conv=notrunc
	chmod +x build/insertFileSystem.out
	cd build && ./insertFileSystem.out
build/Kernel.bin: build/flat.o build/string.o build/ps2.o build/terminal.o build/syscall.o build/usermode.o build/filesystem.o build/memcpy.o build/idt_bridge.o build/malloc.o build/proc.o build/page.o build/asmbridge.o build/idt.o build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o test.ld
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
build/proc.s: proc.c headers/proc.h headers/stdint.h headers/addresses.h
	gcc $(CFLAGS) proc.c -S -o build/proc.s -Wno-pointer-arith
build/idt.s: idt.c headers/stdint.h headers/idt.h
	gcc $(CFLAGS) idt.c -S -o build/idt.s 
build/filesystem.s: filesystem.c headers/string.h headers/ps2.h headers/proc.h headers/terminal.h headers/usermode.h headers/stdint.h headers/addresses.h headers/chs.h
	gcc $(CFLAGS) filesystem.c -S -o build/filesystem.s
build/string.s: string.c headers/string.h
	gcc $(CFLAGS) string.c -S -o build/string.s
build/syscall.s: syscall.c headers/addresses.h headers/filesystem.h
	gcc $(CFLAGS) syscall.c -S -o build/syscall.s
build/usermode.s: usermode.c headers/stdint.h headers/addresses.h headers/proc.h headers/filesystem.h headers/flat.h
	gcc $(CFLAGS) usermode.c -S -o build/usermode.s
build/flat.s: flat.c headers/flat.h headers/stdint.h headers/addresses.h
	gcc $(CFLAGS) flat.c -S -o build/flat.s
build/terminal.s: terminal.c headers/addresses.h headers/stdint.h 
	gcc $(CFLAGS) terminal.c -S -o build/terminal.s
build/ps2.s: ps2.c headers/ps2.h headers/stdint.h headers/addresses.h headers/filesystem.h
	gcc $(CFLAGS) ps2.c -S -o build/ps2.s
build/init.bin: build/init.o
	pushd build && ld -T ../userland/init.ld && popd
build/sh.bin: build/sh.o
	pushd build && ld -T ../userland/sh.ld && popd
build/%.o: userland/%.S
	as $< -o $@


build/insertFileSystem.out: insertFileSystem/main.c headers/filesystem_compat.h
	gcc insertFileSystem/main.c -o build/insertFileSystem.out
clean:
	rm build/*.o build/*.s build/*.out build/*.bin build/keymap -v --one-file-system
