ALL: default 

CC           = gcc
CLINKER      = $(CC)
OPTFLAGS     = -O0


SHELL = /bin/sh

CFLAGS  =   -DREENTRANT -Wunused -Wall -g 
CCFLAGS = $(CFLAGS)
LIBS =  -lpthread

EXECS = common.o common_net.o dsmexec dsmwrap truc dsmexec_lib.o

default: $(EXECS)

dsmexec: dsmexec.o common.o dsmexec_lib.o
	$(CLINKER) $(OPTFLAGS) -o dsmexec dsmexec.o  common.o common_net.o dsmexec_lib.o $(LIBS)
	mv dsmexec ./bin

dsmwrap: dsmwrap.o common.o
	$(CLINKER) $(OPTFLAGS) -o dsmwrap dsmwrap.o  common.o common_net.o $(LIBS)
	mv dsmwrap ./bin

truc: truc.o 
	$(CLINKER) $(OPTFLAGS) -o truc truc.o common.o $(LIBS)	
	mv truc ./bin

clean:
	@-/bin/rm -f *.o *~ PI* $(EXECS) *.out core 
.c:
	$(CC) $(CFLAGS) -o $* $< $(LIBS)
.c.o:
	$(CC) $(CFLAGS) -c $<
.o:
	${CLINKER} $(OPTFLAGS) -o $* $*.o $(LIBS)
