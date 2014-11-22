CFLAGS=-g -Wall -O2
PREFIX=/usr/local

all: libdds.a python

dds.h:
	ln -s dll.h dds.h

dds.c:
	ln -s dds.cpp $@

dds.o: dds.c dds.h

libdds.a: dds.o
	ar rc $@ $^
	ranlib $@

install:
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m644 dll.h $(DESTDIR)$(PREFIX)/include/dds.h
	install libdds.a $(DESTDIR)$(PREFIX)/lib

clean:
	rm -f dds.o libdds.a dds.c dds.h
