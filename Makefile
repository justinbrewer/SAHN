CC = gcc
CFLAGS = -c -I.
LL = gcc
LFLAGS = 
AR = ar
AFLAGS = rcs

OBJDIR = obj
DISTDIR = dist

OBJ = $(OBJDIR)/sahn.o $(OBJDIR)/topo.o

all: init $(OBJ)

dist: all
	mkdir -p $(DISTDIR)
	cp sahn/sahn.h $(DISTDIR)
	$(AR) $(AFLAGS) $(DISTDIR)/libsahn.a $(OBJ)

init:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(DISTDIR)

$(OBJDIR)/sahn.o: sahn/sahn.h sahn/sahn.c sahn/topo.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/sahn.o sahn/sahn.c

$(OBJDIR)/topo.o: sahn/topo.h sahn/topo.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/topo.o sahn/topo.c
