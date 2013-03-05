CC = gcc
CFLAGS = -c -fpic -fvisibility=hidden -I.
LL = gcc
LFLAGS = -shared

OBJDIR = obj
DISTDIR = dist
BINDIR = bin

OBJ = $(OBJDIR)/sahn.o $(OBJDIR)/topo.o

all: init $(OBJ)

dist: all
	@mkdir -p $(DISTDIR)
	@cp sahn/sahn.h $(DISTDIR)
	$(LL) $(LFLAGS) -o $(DISTDIR)/libsahn.so $(OBJ)
	@strip --strip-unneeded $(DISTDIR)/libsahn.so

init:
	@mkdir -p $(OBJDIR)

bin_init: dist
	@mkdir -p $(BINDIR)
	@cp $(DISTDIR)/libsahn.so $(BINDIR)

clean:
	@rm -rf $(OBJDIR) $(DISTDIR) $(BINDIR)

$(OBJDIR)/sahn.o: sahn/sahn.h sahn/sahn.c sahn/topo.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/sahn.o sahn/sahn.c

$(OBJDIR)/topo.o: sahn/topo.h sahn/topo.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/topo.o sahn/topo.c

#====================
EC = $(CC)
EFLAGS = -Wl,-rpath,$(BINDIR) -L$(BINDIR) -lsahn -I.

test1: bin_init examples/test1/test1.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test1 examples/test1/test1.c
