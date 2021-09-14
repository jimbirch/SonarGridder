PROG = songridder
CC = g++
CPPFLAGS = -g 
BUILDPATH = ../build
BINPATH = ../bin
LD_LIBRARY_PATH = /usr/include/x86_64-linux-gnu
LDFLAGS = -ltiff
OBJS = $(BUILDPATH)/main.o $(BUILDPATH)/e1e2.o $(BUILDPATH)/processfiles.o $(BUILDPATH)/sidescan.o

$(PROG) : $(OBJS)
	$(CC) $(OBJS) -o $(BINPATH)/$(PROG) $(LDFLAGS)
$(BUILDPATH)/main.o :
	$(CC) $(CPPFLAGS) -c main.cpp -o $(BUILDPATH)/main.o
$(BUILDPATH)/e1e2.o :
	$(CC) $(CPPFLAGS) -c e1e2.cpp -o $(BUILDPATH)/e1e2.o
$(BUILDPATH)/processfiles.o :
	$(CC) $(CPPFLAGS) -c processfiles.cpp -o $(BUILDPATH)/processfiles.o
$(BUILDPATH)/sidescan.o :
	$(CC) $(CPPFLAGS) -c sidescan.cpp -o $(BUILDPATH)/sidescan.o
clean :
	rm -f core $(BINPATH)/$(PROG) $(OBJS)

