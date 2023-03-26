CC=CL
CARD5=$(HOME)/projects/card5
INCL=$(CARD5)/kernel/include
ORAINCL=$(ORACLE_HOME)\oci\include
CFLAGS= -c /Ox /I$(INCL) /I$(ORAINCL)
UTILITIES_SOURCES= \
	fdmc_exception.c\
	fdmc_logfile.c\
	fdmc_des.c\
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
	fdmc_oci.c\
	fdmc_device.c\
	fdmc_timer.c\
	fdmc_queue.c\
	fdmc_hsm.c \
	fdmc_rs232.c \
	fdmc_thread.c\
	fdmc_stack.c
	

UTILITIES_OBJECTS=$(UTILITIES_SOURCES:.c=.obj)

.SUFFIXES: .c .obj

.c.obj:
	$(CC) $(CFLAGS) $*.c

kernel.lib: $(UTILITIES_OBJECTS)
	lib /OUT:kernel.lib $(UTILITIES_OBJECTS)

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
FDMCHSM=$(INCL)/fdmc_hsm.h
FDMCTIMER=$(INCL)/fdmc_timer.h
FDMCDES=$(INCL)/fdmc_des.h
FDMCTHREAD=$(INCL)/fdmc_thread.h
FDMCRS232=$(INCL)/fdmc_rs232.h

FDMCBASE=$(FDMCGLOBAL) $(FDMCTRACE) $(FDMCEXCEPTIONS) $(FDMCLOGFILE) $(FDMCHASH) $(FDMCLIST) $(FDMCIP) $(FDMCTHREAD)

#1
fdmc_exception.obj: fdmc_exception.c $(FDMCBASE)
	$(CC) $(CFLAGS) $*.c
	
#2	
fdmc_logfile.obj: fdmc_logfile.c $(FDMCBASE)
	$(CC) $(CFLAGS) $*.c
	

#3
fdmc_hash.obj: fdmc_hash.c $(FDMCBASE) 
	$(CC) $(CFLAGS) $*.c

#4
fdmc_list.obj: fdmc_list.c $(FDMCBASE) 
	$(CC) $(CFLAGS) $*.c

#5
fdmc_msgfield.obj: fdmc_msgfield.c $(FDMCBASE)  $(FDMCMSGFIELDS) $(FDMCBTOPS) $(FDMCOPTIONS)
	$(CC) $(CFLAGS) $*.c

#6	
fdmc_option.obj: fdmc_option.c $(FDMCBASE)  $(FDMCOPTIONS) 
	$(CC) $(CFLAGS) $*.c
	
#7
fdmc_bitop.obj: fdmc_bitop.c $(FDMCBASE)  $(FDMCBTOPS)
	$(CC) $(CFLAGS) $*.c
	
#8
fdmc_isomsg.obj: fdmc_isomsg.c $(FDMC_BASE)  $(FDMC_BITOPS) $(FDMCMSGFIELDS) $(FDMCISOMSG)
	$(CC) $(CFLAGS) $*.c
	
#9
fdmc_bsamsg.obj: fdmc_bsamsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS) $(FDMCBSAMSG) $(FDMCBITOPS)
	$(CC) $(CFLAGS) $*.c
	
#10
fdmc_xmlmsg.obj: fdmc_xmlmsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS)  $(FDMCOPTIONS) $(FDMCXMLMSG)
	$(CC) $(CFLAGS) $*.c

#11
fdmc_attmsg.obj: fdmc_attmsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS)  $(FDMCOPTIONS) $(FDMCATTMSG)
	$(CC) $(CFLAGS) $*.c

#12
fdmc_trlmsg.obj: fdmc_trlmsg.c  $(FDMC_BASE)  $(FDMCMSGFIELDS)  $(FDMCOPTIONS) $(FDMCTRLMSG)
	$(CC) $(CFLAGS) $*.c

#13
fdmc_ip.obj: fdmc_ip.c  $(FDMC_BASE)
	$(CC) $(CFLAGS) $*.c
	
#14
fdmc_thread.obj: fdmc_thread.c  $(FDMCBASE) $(FDMCTHREAD)
	$(CC) $(CFLAGS) $*.c
#15
fdmc_oci.obj: fdmc_oci.c  $(FDMCBASE)  $(FDMCOCI)
	$(CC) $(CFLAGS) $*.c

#16
fdmc_queue.obj: fdmc_queue.c  $(FDMCBASE)  $(FDMCQUEUE)
	$(CC) $(CFLAGS) $*.c

#18
fdmc_timer.obj: fdmc_timer.c  $(FDMCBASE)  $(FDMCTIMER)
	$(CC) $(CFLAGS) $*.c
	
#19
fdmc_device.obj: fdmc_device.c  $(FDMCBASE)  $(FDMCDEVICE)
	$(CC) $(CFLAGS) $*.c

#20
fmdc_des.obj: fdmc_des.c  $(FDMCBASE)  $(FDMCDES)
	$(CC) $(CFLAGS) $*.c

#17
fdmc_stack.obj: fdmc_stack.c  $(FDMCBASE)  $(FDMCSTACK)
	$(CC) $(CFLAGS) $*.c

#21
fdmc_hsm.obj: fdmc_hsm.c $(FDMCBASE) $(FDMCBSAMSG) $(FDMCHSM)
	$(CC) $(CFLAGS) $*.c

#22
fdmc_rs232.obj: fdmc_rs232.c $(FDMCBASE) $(FDMCRS232)
	$(CC) $(CFLAGS) $*.c
