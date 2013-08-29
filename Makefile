CC=gcc
CFLAGS=-O3 -mtune=native -march=native -fPIC -Wall `pkg-config gtk+-2.0 glib-2.0 --cflags --libs`
NAME=obrun
SRC_FILES := \
	obrun.c \
	path.c

OUT=$(NAME)
PREFIX=/usr/bin

#LIBS=`pkg-config gtk+-2.0 glib-2.0 --libs`

all:
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(OUT) $(LIBS)
	strip ./$(OUT)

install: all
	cp ./$(OUT) $(PREFIX)

uninstall:
	rm $(PREFIX)/$(OUT)

clean:
	rm -f $(OUT)
