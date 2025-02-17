ALL: default 

CC           = gcc
CLINKER      = $(CC)
OPTFLAGS     = -O0


SHELL = /bin/sh

CFLAGS  =   -DREENTRANT -Wunused -Wall -g 
CCFLAGS = $(CFLAGS)
LIBS =  -lpthread

EXECS = common.o common_net.o dsmexec exemple dsmexec_lib.o

default: $(EXECS)

dsmexec: dsmexec.o common.o dsmexec_lib.o
	$(CLINKER) $(OPTFLAGS) -o dsmexec dsmexec.o  common.o common_net.o dsmexec_lib.o $(LIBS)
	mv dsmexec ./bin

exemple: exemple.o dsm.o common.o
	$(CLINKER) $(OPTFLAGS) -o exemple exemple.o dsm.o common.o common_net.o  $(LIBS)
	mv exemple ./bin

clean:
	@-/bin/rm -f *.o *~ PI* $(EXECS) *.out core 
.c:
	$(CC) $(CFLAGS) -o $* $< $(LIBS)
.c.o:
	$(CC) $(CFLAGS) -c $<
.o:
	${CLINKER} $(OPTFLAGS) -o $* $*.o $(LIBS)
