CC = gcc
CFLAGS = -L/usr/X11/lib -lX11
DIR = /usr/local/bin

install:
	$(CC) $(CFLAGS) -o sewm main.c
	install sewm $(DIR)/sewm

uninstall:
	rm -rf $(DIR)/sewm sewm
