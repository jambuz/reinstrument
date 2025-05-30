CC := clang
CFLAGS := -target x86_64-windows-gnu -Wall -Wextra -O2
INCLUDES := -I include -isystem /usr/x86_64-w64-mingw32/include
LIBS := -L lib -lQBDI

.PHONY: build

build:
	mkdir bin
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o bin/main.exe src/main.c
	@cp lib/QBDI.dll bin/QBDI.dll
