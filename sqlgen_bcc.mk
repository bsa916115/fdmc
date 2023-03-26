CC=bcc32
INCL=\projects\fdmc\include
CFLAGS= /Ox -I$(INCL) -I\developer\include\oci -DBCC32
FDMCGLOBAL=$(INCL)\fdmc_global.h
FDMCEXCEPTIONS=$(INCL)\fdmc_exceptions.h
FDMCLOGFILE=$(INCL)\fdmc_logfile.h
FDMCHASH=$(INCL)\fdmc_hash.h
FDMCLIST=$(INCL)\fdmc_list.h
FDMCOPTIONS=$(INCL)\fdmc_options.h
FDMCBITOPS=$(INCL)\fdmc_bitops.h
FDMCMSGFIELDS=$(INCL)\fdmc_msgfields.h
FDMCISOMSG=$(INCL)\fdmc_isomsg.h
FDMCBSAMSG=$(INCL)\fdmc_bsamsg.h
FDMCXMLMSG=$(INCL)\fdmc_xmlmsg.h
FDMCATTMSG=$(INCL)\fdmc_attrmsg.h
FDMCTRLMSG=$(INCL)\fdmc_trlmsg.h
FDMCIP=$(INCL)\fdmc_ip.h
FDMCTHREAD=$(INCL)\fdmc_thread.h
FDMCPROCESS=$(INCL)\fdmc_process.h
FDMCOCI=$(INCL)\fdmc_oci.h

FDMCBASE=$(FDMCGLOBAL) $(FDMCTRACE) $(FDMCEXCEPTIONS) $(FDMCLOGFILE) $(FDMCHASH) $(FDMCLIST) $(FDMCIP) $(FDMCTHREAD)

.SUFFIXES: .c .obj

.c.obj:
	$(CC) $(CFLAGS) $<

sqlgen: sqlgen.c \projects\fdmc\c\fdmcbcc.lib $(FDMCBASE) $(FDMCOCI) $(FDMCOPTIONS)
	$(CC) $(CFLAGS) $<.c \projects\fdmc\c\fdmcbcc.lib \developer\lib\oci\MSVC\oci.lib
	