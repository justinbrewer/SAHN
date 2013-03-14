VERSION = 0.2

CC = gcc
CFLAGS = -c -fpic -fvisibility=hidden -Isahn -pthread
debug : CFLAGS += -g -O0

LL = gcc
LFLAGS = -shared -pthread

OBJDIR = obj
DISTDIR = libsahn-$(VERSION)
BINDIR = bin

OBJ = $(OBJDIR)/sahn.o $(OBJDIR)/topo.o $(OBJDIR)/udp.o $(OBJDIR)/net.o $(OBJDIR)/seq.o

all: debug

dist: $(DISTDIR).tar.gz

debug: $(BINDIR) $(BINDIR)/libsahn_d.so

clean:
	@rm -rf $(OBJDIR) $(DISTDIR){,.tar.gz} $(BINDIR)

#====================

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(DISTDIR):
	@mkdir -p $(DISTDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

#====================

$(DISTDIR).tar.gz: $(DISTDIR) $(DISTDIR)/sahn.h $(DISTDIR)/libsahn.so
	tar czvf $(DISTDIR).tar.gz $(DISTDIR)

$(DISTDIR)/sahn.h: sahn/sahn.h
	@cp sahn/sahn.h $(DISTDIR)

$(DISTDIR)/libsahn.so: $(OBJDIR) $(OBJ)
	$(LL) $(LFLAGS) -o $(DISTDIR)/libsahn.so $(OBJ)
	@strip --strip-unneeded $(DISTDIR)/libsahn.so

#====================

$(BINDIR)/libsahn_d.so: $(OBJDIR) $(OBJ)
	$(LL) $(LFLAGS) -o $(BINDIR)/libsahn_d.so $(OBJ)

#====================

$(OBJDIR)/sahn.o: sahn/sahn.h sahn/sahn.c sahn/topo.h sahn/udp.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/sahn.o sahn/sahn.c

$(OBJDIR)/topo.o: sahn/topo.h sahn/topo.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/topo.o sahn/topo.c

$(OBJDIR)/udp.o: sahn/udp.h sahn/udp.c sahn/topo.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/udp.o sahn/udp.c

$(OBJDIR)/net.o: sahn/net.h sahn/net.c sahn/topo.h sahn/udp.h sahn/seq.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/net.o sahn/net.c

$(OBJDIR)/seq.o: sahn/seq.h sahn/seq.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/seq.o sahn/seq.c

#====================
EC = $(CC)
EFLAGS = -Wl,-rpath,$(BINDIR) -L$(BINDIR) -lsahn_d -Isahn -g -O0

test1: debug examples/test1/test1.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test1 examples/test1/test1.c

test2: debug examples/test2/test2.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test2 examples/test2/test2.c

test3: debug examples/test3/test3.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test3 examples/test3/test3.c

test4: debug examples/test4/test4.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test4 examples/test4/test4.c
