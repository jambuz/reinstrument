CC := clang
CFLAGS := -target x86_64-windows-gnu -Wall -Wextra -O2
INCLUDES := -I /mnt/qemu-shared/instr/include -isystem /usr/x86_64-w64-mingw32/include
LIBS := -L /mnt/qemu-shared/instr/lib -lQBDI

.PHONY: build

build:
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o bin/main.exe src/main.c
	@cp lib/QBDI.dll bin/QBDI.dll
