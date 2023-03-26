CC=gcc
INCL=$(HOME)/projects/fdmc/include
ORAINCL=$(ORACLE_HOME)/rdbms/public
CFLAGS= -c -O -I$(INCL) -I$(ORAINCL)
UTILITIES_SOURCES= \
	fdmc_exception.c\
	fdmc_logfile.c\
	fdmc_des.c\
	fdmc_scmp.c\
	fdmc_hash.c\
	fdmc_list.c\
	fdmc_option.c\
	fdmc_bitop.c\
	fdmc_msgfield.c\
	fdmc_isomsg.c\
	fdmc_bsamsg.c\
	fdmc_xmlmsg.c\
	fdmc_attrmsg.c\
	fdmc_trlmsg.c\
	fdmc_ip.c\
	fdmc_thread.c\
	fdmc_oci.c\
	fdmc_device.c\
	fdmc_timer.c\
	fdmc_queue.c\
	fdmc_thread.c\
	fdmc_hsm.c \
	fdmc_stack.c
	

UTILITIES_OBJECTS=$(UTILITIES_SOURCES:.c=.o)

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $<

fdmc.a: $(UTILITIES_OBJECTS) 

FDMCEXCEPTIONS=$(INCL)/fdmc_exception.h
FDMCLOGFILE=$(INCL)/fdmc_logfile.h
FDMCHASH=$(INCL)/fdmc_hash.h
FDMCLIST=$(INCL)/fdmc_list.h
FDMCOPTIONS=$(INCL)/fdmc_option.h
FDMCBITOPS=$(INCL)/fdmc_bitop.h
FDMCMSGFIELDS=$(INCL)/fdmc_msgfield.h
FDMCISOMSG=$(INCL)/fdmc_isomsg.h
FDMCBSAMSG=$(INCL)/fdmc_bsamsg.h
FDMCXMLMSG=$(INCL)/fdmc_xmlmsg.h
FDMCATTMSG=$(INCL)/fdmc_attrmsg.h
FDMCTRLMSG=$(INCL)/fdmc_trlmsg.h
FDMCIP=$(INCL)/fdmc_ip.h
FDMCTHREAD=$(INCL)/fdmc_thread.h
FDMCOCI=$(INCL)/fdmc_oci.h
FDMCQUEUE=$(INCL)/fdmc_queue.h
FDMCDEVICE=$(INCL)/fdmc_device.h
FDMCSTACK=$(INCL)/fdmc_stack.h
FDMCTIMER=$(INCL)/fdmc_timer.h
FDMCDES=$(INCL)/fdmc_des.h
FDMCTHREAD=$(INCL)/fdmc_thread.h

FDMCBASE=$(FDMCGLOBAL) $(FDMCTRACE) $(FDMCEXCEPTIONS) $(FDMCLOGFILE) $(FDMCHASH) $(FDMCLIST) $(FDMCIP) $(FDMCTHREAD)

#1
fdmc_exception.o: fdmc_exception.c $(FDMCBASE)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#2	
fdmc_logfile.o: fdmc_logfile.c $(FDMCBASE)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	

#3
fdmc_hash.o: fdmc_hash.c $(FDMCBASE) 
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#4
fdmc_list.o: fdmc_list.c $(FDMCBASE) 
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#5
fdmc_msgfield.o: fdmc_msgfield.c $(FDMCBASE)  $(FDMCMSGFIELDS) $(FDMCBTOPS) $(FDMCOPTIONS)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#6	
fdmc_option.o: fdmc_option.c $(FDMCBASE)  $(FDMCOPTIONS) 
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#7
fdmc_bitop.o: fdmc_bitop.c $(FDMCBASE)  $(FDMCBTOPS)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#8
fdmc_isomsg.o: fdmc_isomsg.c $(FDMC_BASE)  $(FDMC_BITOPS) $(FDMCMSGFIELDS) $(FDMCISOMSG)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#9
fdmc_bsamsg.o: fdmc_bsamsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS) $(FDMCBSAMSG) $(FDMCBITOPS)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#10
fdmc_xmlmsg.o: fdmc_xmlmsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS)  $(FDMCOPTIONS) $(FDMCXMLMSG)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#11
fdmc_attmsg.o: fdmc_attmsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS)  $(FDMCOPTIONS) $(FDMCATTMSG)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#12
fdmc_trlmsg.o: fdmc_trlmsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS)  $(FDMCOPTIONS) $(FDMCTRLMSG)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#13
fdmc_ip.o: fdmc_ip.c  $(FDMC_BASE)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#14
fdmc_thread.o: fdmc_thread.c  $(FDMCBASE) $(FDMCTHREAD)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
#15
fdmc_oci.o: fdmc_oci.c  $(FDMCBASE)  $(FDMCOCI)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#16
fdmc_queue.o: fdmc_queue.c  $(FDMCBASE)  $(FDMCQUEUE)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#18
fdmc_timer.o: fdmc_timer.c  $(FDMCBASE)  $(FDMCTIMER)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o
	
#19
fdmc_device.o: fdmc_device.c  $(FDMCBASE)  $(FDMCDEVICE)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#20
fmdc_des.o: fdmc_des.c  $(FDMCBASE)  $(FDMCDES)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

#17
fdmc_stack.o: fdmc_stack.c  $(FDMCBASE)  $(FDMCSTACK)
	$(CC) $(CFLAGS) $<
	ar r fdmc.a $*.o

