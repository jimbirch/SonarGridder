PROG = sonargridder
CC = g++
CPPFLAGS = -g
BUILDPATH = ../build
BINPATH = ../bin
PREFIX = /usr/local/bin
LD_LIBRARY_PATH = /usr/include/x86_64-linux-gnu
LDFLAGS = -ltiff
OBJS = $(BUILDPATH)/processfiles.o $(BUILDPATH)/sidescan.o 
MAINIMPL = $(BUILDPATH)/main.o
EXP = $(BUILDPATH)/acousticfunctions.o $(BUILDPATH)/mainexp.o

$(PROG) : $(OBJS) $(MAINIMPL)
	$(CC) $(OBJS) $(MAINIMPL) -o $(BINPATH)/$(PROG) $(LDFLAGS)
experimental : $(OBJS) $(EXP)
	$(CC) $(OBJS) $(EXP) -o $(BINPATH)/$(PROG) $(LDFLAGS)
$(BUILDPATH)/main.o :
	$(CC) $(CPPFLAGS) -c main.cpp -o $(BUILDPATH)/main.o
$(BUILDPATH)/processfiles.o :
	$(CC) $(CPPFLAGS) -c processfiles.cpp -o $(BUILDPATH)/processfiles.o
$(BUILDPATH)/sidescan.o :
	$(CC) $(CPPFLAGS) -c sidescan.cpp -o $(BUILDPATH)/sidescan.o
$(BUILDPATH)/acousticfunctions.o :
	$(CC) $(CPPFLAGS) -c acousticfunctions.cpp -o $(BUILDPATH)/acousticfunctions.o
$(BUILDPATH)/mainexp.o :
	$(CC) $(CPPFLAGS) -DEXPERIMENTAL=1 -c main.cpp -o $(BUILDPATH)/mainexp.o
install :
	cp $(BINPATH)/$(PROG) $(PREFIX)
uninstall :
	rm -f $(PREFIX)/$(PROG)
clean :
	rm -f core $(BINPATH)/$(PROG) $(OBJS) $(MAINIMPL) $(EXP)

