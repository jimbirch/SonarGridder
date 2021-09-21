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
GUI = $(BUILDPATH)/guimain.o $(BUILDPATH)/songui.o
EXP = $(BUILDPATH)/acousticfunctions.o $(BUILDPATH)/mainexp.o

$(PROG) : $(OBJS) $(MAINIMPL)
	$(CC) $(OBJS) $(MAINIMPL) -o $(BINPATH)/$(PROG) $(LDFLAGS)
experimental : $(OBJS) $(EXP)
	$(CC) $(OBJS) $(EXP) -o $(BINPATH)/$(PROG) $(LDFLAGS)
gui : $(OBJS) $(GUI)
	$(CC) $(OBJS) $(GUI) -o $(BINPATH)/$(PROG) $(LDFLAGS) `pkg-config --libs gtkmm-3.0`
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
$(BUILDPATH)/guimain.o :
	$(CC) $(CPPFLAGS) `pkg-config --cflags gtkmm-3.0` -c guimain.cpp -o $(BUILDPATH)/guimain.o
$(BUILDPATH)/songui.o :
	$(CC) $(CPPFLAGS) `pkg-config --cflags gtkmm-3.0` -c songui.cpp -o $(BUILDPATH)/songui.o
install :
	cp $(BINPATH)/$(PROG) $(PREFIX)
uninstall :
	rm -f $(PREFIX)/$(PROG)
clean :
	rm -f core $(BINPATH)/$(PROG) $(OBJS) $(MAINIMPL) $(EXP) $(GUI)

