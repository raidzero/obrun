CC=gcc
CFLAGS=-O3 -mtune=native -march=native -fPIC -Wall `pkg-config gtk+-2.0 glib-2.0 --cflags --libs`
NAME=obrun
SRC_FILES := \
	obrun.c \
	path.c

OUT=$(NAME)
PREFIX=/usr/bin

all:
	$(CC) $(CFLAGS) -DDEBUG=0 $(SRC_FILES) -o $(OUT) $(LIBS)
	strip ./$(OUT)

debug:
	$(CC) $(CFLAGS) -DDEBUG=1 $(SRC_FILES) -o $(OUT) $(LIBS)
	strip ./$(OUT)

install: all
	cp ./$(OUT) $(PREFIX)

uninstall:
	rm $(PREFIX)/$(OUT)

clean:
	rm -f $(OUT)
