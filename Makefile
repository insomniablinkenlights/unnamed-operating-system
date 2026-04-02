CFLAGS=-static -fno-pic -fplt -m64 -mcmodel=large -Wall -Wextra -Wpedantic -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -mno-red-zone -mgeneral-regs-only -nostdlib -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm -fno-builtin -ffreestanding
CC=x86_64-elf-gcc
MAKEFLAGS += -j2
floppya.img: build/Kernel.bin build/insertFileSystem.out build/sysroot/sbin/init build/sysroot/sbin/sh build/sysroot/sbin/echo build/sysroot/kmod/serial
	dd if="/dev/zero" of=floppya.img bs=1KiB count=1440
	dd if="build/Kernel.bin" of=floppya.img conv=notrunc
	chmod +x build/insertFileSystem.out
	cd build && ./insertFileSystem.out
build/Kernel.bin: build/driver.o build/pf.o build/flat.o build/string.o build/ps2.o build/terminal.o build/syscall.o build/usermode.o build/filesystem.o build/memcpy.o build/idt_bridge.o build/malloc.o build/proc.o build/page.o build/asmbridge.o build/idt.o build/chs.o build/pci.o build/longk.o build/protk.o build/end.o build/realk.o build/boot.o build/a20.o test.ld
	cd build && ld -T ../test.ld
build/%.o: %.S
	as $< -o $@
build/%.o: build/%.s
	as $< -o $@

build/chs.s: chs.c headers/stdint.h headers/standard.h headers/addresses.h
	$(CC) $(CFLAGS) chs.c -S -o build/chs.s 
build/page.s: page.c headers/stdint.h headers/addresses.h headers/standard.h headers/proc.h
	$(CC) $(CFLAGS) page.c -S -o build/page.s 
build/malloc.s: malloc.c headers/stdint.h headers/addresses.h headers/standard.h
	$(CC) $(CFLAGS) malloc.c -S -o build/malloc.s 
build/proc.s: proc.c headers/proc.h headers/stdint.h headers/addresses.h headers/standard.h headers/usermode.h
	$(CC) $(CFLAGS) proc.c -S -o build/proc.s -Wno-pointer-arith
build/idt.s: idt.c headers/stdint.h headers/idt.h headers/standard.h headers/addresses.h
	$(CC) $(CFLAGS) idt.c -S -o build/idt.s 
build/filesystem.s: filesystem.c headers/string.h headers/ps2.h headers/proc.h headers/terminal.h headers/usermode.h headers/stdint.h headers/addresses.h headers/chs.h headers/standard.h headers/filesystem.h headers/device.h
	$(CC) $(CFLAGS) filesystem.c -S -o build/filesystem.s
build/string.s: string.c headers/string.h
	$(CC) $(CFLAGS) string.c -S -o build/string.s
build/syscall.s: syscall.c headers/addresses.h headers/filesystem.h headers/stdint.h headers/device.h headers/usermode.h headers/proc.h headers/brk.h headers/standard.h
	$(CC) $(CFLAGS) syscall.c -S -o build/syscall.s
build/usermode.s: usermode.c headers/stdint.h headers/addresses.h headers/proc.h headers/filesystem.h headers/flat.h headers/string.h headers/brk.h headers/standard.h
	$(CC) $(CFLAGS) usermode.c -S -o build/usermode.s
build/flat.s: flat.c headers/flat.h headers/stdint.h headers/addresses.h headers/string.h headers/proc.h headers/brk.h headers/standard.h
	$(CC) $(CFLAGS) flat.c -S -o build/flat.s
build/terminal.s: terminal.c headers/addresses.h headers/stdint.h 
	$(CC) $(CFLAGS) terminal.c -S -o build/terminal.s
build/ps2.s: ps2.c headers/ps2.h headers/stdint.h headers/addresses.h headers/filesystem.h headers/terminal.h
	$(CC) $(CFLAGS) ps2.c -S -o build/ps2.s
build/pf.s: pf.c headers/stdint.h headers/addresses.h headers/proc.h headers/brk.h headers/standard.h
	$(CC) $(CFLAGS) pf.c -S -o build/pf.s
build/driver.s: driver.c headers/device.h headers/proc.h headers/stdint.h headers/usermode.h headers/addresses.h headers/terminal.h headers/string.h headers/standard.h
	$(CC) $(CFLAGS) driver.c -S -o build/driver.s
build/sysroot/sbin/init: build/userland/init.o userland/init.ld
	pushd build/userland && ld -T ../../userland/init.ld && mv ./init.bin ../sysroot/sbin/init && popd
build/sysroot/sbin/sh: build/userland/sh.o build/userland/crt0.o build/userland/int.o build/userland/libc.o userland/stdlib.h userland/sh.ld
	pushd build/userland && ld -T ../../userland/sh.ld && mv ./sh.bin ../sysroot/sbin/sh && popd
build/sysroot/sbin/echo: build/userland/echo.o build/userland/crt0.o build/userland/int.o build/userland/libc.o userland/stdlib.h userland/echo.ld
	pushd build/userland && ld -T ../../userland/echo.ld && mv ./echo.bin ../sysroot/sbin/echo && popd
build/sysroot/kmod/serial: build/userland/serial.o build/userland/crt0.o build/userland/int.o build/userland/libc.o userland/stdlib.h userland/serial.ld
	pushd build/userland && ld -T ../../userland/serial.ld && mkdir -vp ../sysroot/kmod/ && mv ./serial.bin ../sysroot/kmod/serial && popd
build/userland/%.o: userland/%.S
	as $< -o $@
build/userland/%.o: build/userland/%.s
	as $< -o $@
build/userland/%.s: userland/%.c
	$(CC) $< -S -o $@ -ffreestanding -Wall -Wextra -Wpedantic
build/insertFileSystem.out: insertFileSystem/main.c headers/filesystem_compat.h
	gcc -Wall -Wextra -Wpedantic -Werror -O0 insertFileSystem/main.c -o build/insertFileSystem.out
clean:
	rm build/userland/*.o build/userland/*.s build/*.o build/*.s build/*.out build/*.bin build/sysroot/*/*.bin build/sysroot/etc/keymap -v --one-file-system
