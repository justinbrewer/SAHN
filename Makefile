CC = gcc
CFLAGS = -c -fpic -fvisibility=hidden -I.
LL = gcc
LFLAGS = -shared

OBJDIR = obj
DISTDIR = dist

OBJ = $(OBJDIR)/sahn.o $(OBJDIR)/topo.o

all: init $(OBJ)

dist: all
	@mkdir -p $(DISTDIR)
	@cp sahn/sahn.h $(DISTDIR)
	$(LL) $(LFLAGS) -o $(DISTDIR)/libsahn.so $(OBJ)
	@strip --strip-unneeded $(DISTDIR)/libsahn.so

init:
	@mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(DISTDIR)

$(OBJDIR)/sahn.o: sahn/sahn.h sahn/sahn.c sahn/topo.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/sahn.o sahn/sahn.c

$(OBJDIR)/topo.o: sahn/topo.h sahn/topo.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/topo.o sahn/topo.c
