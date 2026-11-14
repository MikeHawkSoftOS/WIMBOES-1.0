# Wimboes build: `make` (about a second), `make run` (QEMU), `make test` (native UI screenshots)
CC      = gcc
CFLAGS  = -m32 -O2 -Wall -Wextra -ffreestanding -fno-pic -fno-pie -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx \
          -fno-asynchronous-unwind-tables -fno-tree-loop-distribute-patterns -Isrc
SRCS    = $(filter-out src/hal_host.c,$(wildcard src/*.c))
OBJS    = $(patsubst src/%.c,build/%.o,$(SRCS)) build/entry.o
all: build/wimboes.img

build/%.o: src/%.c src/os.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/entry.o: src/entry.S | build
	$(CC) -m32 -c $< -o $@
build:
	mkdir -p build
build/kernel.bin: $(OBJS) src/link.ld
	ld -m elf_i386 -T src/link.ld -z noexecstack -o build/kernel.elf $(OBJS)
	objcopy -O binary build/kernel.elf build/kernel.bin
	truncate -s %512 build/kernel.bin
build/boot.bin: src/boot.S build/kernel.bin
	as --32 --defsym KSECTORS=$$(( $$(stat -c %s build/kernel.bin) / 512 )) -o build/boot.o src/boot.S
	ld -m elf_i386 -Ttext 0x7C00 --oformat binary -o build/boot.bin build/boot.o
build/wimboes.img: build/boot.bin build/kernel.bin
	cat build/boot.bin build/kernel.bin > $@
	@echo "built $@ ($$(stat -c %s $@) bytes)"
run: build/wimboes.img
	qemu-system-i386 -drive file=build/wimboes.img,format=raw -m 128 -vga std -no-reboot
run-kvm: build/wimboes.img
	qemu-system-i386 -enable-kvm -drive file=build/wimboes.img,format=raw -m 128 -vga std
vdi: build/wimboes.img
	qemu-img convert -O vdi build/wimboes.img build/wimboes.vdi
test:
	./test/run.sh
clean:
	rm -rf build
.PHONY: all run run-kvm vdi test clean
