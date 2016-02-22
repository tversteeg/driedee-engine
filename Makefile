# Generic makefile for static libraries

NAME:=driedee

SOURCEDIR:=src/$(NAME)
LIBDIR:=lib
INCDIR:=include
BINDIR:=bin

RM:=rm -f
AR:=ar rcs
CFLAGS:=-I$(INCDIR) -g -Wall -Wextra -O3 -DCC_USE_ALL
LDLIBS:=-lGL -lGLU -lGLEW -lm -lccore -lccfont -lccterm -lpng

LSRCS:=$(wildcard ./$(SOURCEDIR)/l_*.c)
LOBJS:=$(subst .c,.o,$(LSRCS))
LIBFILE:=$(LIBDIR)/lib$(NAME).a

GSRCS:=$(wildcard ./$(SOURCEDIR)/g_*.c)
GOBJS:=$(subst .c,.o,$(GSRCS))

all: $(NAME)

.PHONY: $(NAME)
$(NAME): $(LIBFILE) $(GOBJS) .depend
	@mkdir $(BINDIR)
	$(CC) $(LDFLAGS) -o $(NAME) $(OBJS) $(LDLIBS)

$(LIBFILE): $(LOBJS) .depend
	@mkdir -p $(LIBDIR)
	$(AR) $(LIBFILE) $(LOBJS)

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
