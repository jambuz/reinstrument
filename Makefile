CC := clang
CFLAGS := --target=x86_64-windows-gnu -Wall -Wextra -O3
INCLUDES := -I include -isystem /usr/x86_64-w64-mingw32/include
LIBS := -L lib -lQBDI

.PHONY: build clean

build:
	mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o bin/main.exe src/main.c
	@cp lib/QBDI.dll bin/QBDI.dll

build-dll:
	mkdir -p bin
	$(CC) $(CFLAGS) -shared $(INCLUDES) $(LIBS) -o bin/main.dll src/dllmain.c
	@cp lib/QBDI.dll bin/QBDI.dll

clean:
	rm -rf bin

# https://bitbucket.org/evolution536/crysearch-memory-scanner/src/e4e11f4dd485f7d9e722da1ca3838a626c46497b/ModuleManager.cpp?at=master&fileviewer=file-view-default
