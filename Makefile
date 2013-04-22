VERSION = $(shell git rev-parse --short HEAD)
ROUTE ?= olsr

CC = gcc
CFLAGS = -c -fpic -fvisibility=hidden -Isrc -pthread
debug : CFLAGS += -g -O0

LL = gcc
LFLAGS = -shared -pthread

OBJDIR = obj
DISTDIR = libsahn-$(VERSION)
BINDIR = bin

OBJ = $(OBJDIR)/sahn.o $(OBJDIR)/topo.o $(OBJDIR)/udp.o $(OBJDIR)/net.o \
      $(OBJDIR)/seq.o $(OBJDIR)/util/queue.o $(OBJDIR)/util/cache.o \
      $(OBJDIR)/route/$(ROUTE).o

all: debug

dist: $(DISTDIR).tar.gz

debug: $(BINDIR) $(BINDIR)/libsahn_d.so

clean:
	@rm -rf $(OBJDIR) $(DISTDIR){,.tar.gz} $(BINDIR)

#====================

$(OBJDIR):
	@mkdir -p $(OBJDIR)/{,util,route}

$(DISTDIR):
	@mkdir -p $(DISTDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

#====================

$(DISTDIR).tar.gz: $(DISTDIR) $(DISTDIR)/sahn.h $(DISTDIR)/libsahn.so
	@tar czf $(DISTDIR).tar.gz $(DISTDIR)

$(DISTDIR)/sahn.h: src/sahn.h
	@cp src/sahn.h $(DISTDIR)

$(DISTDIR)/libsahn.so: $(OBJDIR) $(OBJ)
	$(LL) $(LFLAGS) -o $(DISTDIR)/libsahn.so $(OBJ)
	@strip --strip-unneeded $(DISTDIR)/libsahn.so

#====================

$(BINDIR)/libsahn_d.so: $(OBJDIR) $(OBJ)
	$(LL) $(LFLAGS) -o $(BINDIR)/libsahn_d.so $(OBJ)

#====================

$(OBJDIR)/sahn.o: src/sahn.h src/sahn.c src/topo.h src/udp.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/sahn.o src/sahn.c

$(OBJDIR)/topo.o: src/topo.h src/topo.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/topo.o src/topo.c

$(OBJDIR)/udp.o: src/udp.h src/udp.c src/topo.h src/util/cache.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/udp.o src/udp.c

$(OBJDIR)/net.o: src/net.h src/net.c src/topo.h src/udp.h src/seq.h src/util/queue.h
	$(CC) $(CFLAGS) -o $(OBJDIR)/net.o src/net.c

$(OBJDIR)/seq.o: src/seq.h src/seq.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/seq.o src/seq.c

$(OBJDIR)/util/queue.o: src/util/queue.h src/util/queue.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/util/queue.o src/util/queue.c

$(OBJDIR)/util/cache.o: src/util/cache.h src/util/cache.c
	$(CC) $(CFLAGS) -o $(OBJDIR)/util/cache.o src/util/cache.c

$(OBJDIR)/route/$(ROUTE).o: src/route.h src/route/$(ROUTE).c
	$(CC) $(CFLAGS) -o $(OBJDIR)/route/$(ROUTE).o src/route/$(ROUTE).c

#====================
EC = $(CC)
EFLAGS = -Wl,-rpath,$(BINDIR) -L$(BINDIR) -lsahn_d -Isrc -g -O0

test1: debug $(BINDIR)/test1
$(BINDIR)/test1: examples/test1/test1.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test1 examples/test1/test1.c

test2: debug $(BINDIR)/test2
$(BINDIR)/test2: examples/test2/test2.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test2 examples/test2/test2.c

test3: debug $(BINDIR)/test3
$(BINDIR)/test3: examples/test3/test3.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test3 examples/test3/test3.c

test4: debug $(BINDIR)/test4
$(BINDIR)/test4: examples/test4/test4.c
	$(EC) $(EFLAGS) -o $(BINDIR)/test4 examples/test4/test4.c
