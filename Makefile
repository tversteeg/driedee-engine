# Generic makefile for static libraries

NAME=driedee

SOURCEDIR=src/$(NAME)
LIBDIR=lib
INCDIR=include
BINDIR=bin

RM=rm -f
AR=ar rcs
LCFLAGS=-I$(INCDIR) -g -Wall -Wextra -O3
LLDLIBS=-lm -lpng
GCFLAGS=-I$(INCDIR) -g -Wall -Wextra -O3 -DCC_USE_ALL
GLDLIBS=-lGL -lGLU -lGLEW -lm -lccore -lpng

LSRCS=$(wildcard ./$(SOURCEDIR)/l_*.c)
LOBJS=$(subst .c,.o,$(SRCS))
LIBFILE=lib$(NAME).a

GSRCS=$(wildcard ./$(SOURCEDIR)/g_*.c)
GOBJS=$(subst .c,.o,$(SRCS))

all: lib

.PHONY: lib
lib: $(OBJS)
	$(AR) $(LIBDIR)/lib$(NAME).a $(LOBJS)

.PHONY: clean
clean:
	find $(SOURCEDIR) -type f -name '*.o' -delete
	$(RM) $(LIBDIR)/$(LIBFILE)

.PHONY: install
install:
	mkdir -p $(DESTDIR)/usr/include
	cp -R $(INCDIR)/* $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/lib
	cp -R $(LIBDIR)/* $(DESTDIR)/usr/lib

.PHONY: dist-clean
dist-clean: clean
	$(RM) *~ .depend

.depend: $(GSRCS) $(LSRCS)
	$(RM) ./.depend
	$(CC) $(CFLAGS) -MM $(GSRCS) $(LSRCS) >>./.depend;

include .depend
