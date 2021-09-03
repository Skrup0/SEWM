CC = gcc
CFLAGS = -pthread -Wno-deprecated-declarations -L/usr/X11/lib -lX11 -I/usr/include/freetype2 -lXft
DIR = /usr/local/bin

install:
	$(CC) $(CFLAGS) -o bin/sewm src/main.c
	install bin/sewm $(DIR)/sewm

uninstall:
	rm -rf $(DIR)/sewm bin/sewm

reinstall: uninstall install
