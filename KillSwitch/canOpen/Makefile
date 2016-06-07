# #############################################################################
#
# project   :  CAN - Controller Area Network.
#
# purpose   :  CANopen Commandline Tool (using berliOS socketCAN)
#
# copyright :  (C) 2008-2009, UV Software, Friedrichshafen
#
# compiler  :  GCC - GNU C Compiler (Linux Kernel 2.6)
#
# author    :  Uwe Vogt, UV Software, Friedrichshafen
#
# e-mail    :  uwe.vogt@uv-software.de
#
# #############################################################################

CFLAGS	= -O2 -Wall -Wno-parentheses \
	  -fno-strict-aliasing \
	  -DPF_CAN=29 \
	  -DAF_CAN=PF_CAN \
	  -D_COPAPI_EXTERN

PROGRAM	= can_open

OBJECTS = main.o can_ctrl.o cop_api.o cop_sdo.o cop_nms.o cop_lss.o cop_lmt.o cop_tcp.o base64.o

MAIN_DEPS = cop_tcp.h cop_api.h can_ctrl.h can_defs.h default.h base64.h

COP_TCP_DEPS = cop_tcp.h cop_api.h  can_defs.h default.h base64.h
COP_API_DEPS = cop_api.h can_ctrl.h can_defs.h default.h
COP_SDO_DEPS = cop_api.h can_ctrl.h can_defs.h default.h
COP_NMS_DEPS = cop_api.h can_ctrl.h can_defs.h default.h
COP_LSS_DEPS = cop_api.h can_ctrl.h can_defs.h default.h
COP_LMT_DEPS = cop_api.h can_ctrl.h can_defs.h default.h

CAN_CTRL_DEPS = can_ctrl.h can_defs.h default.h


all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o

install:
	cp -f $(PROGRAM) /usr/local/bin

distclean:
	rm -f $(PROGRAM) *.o *~


main.o: main.c $(MAIN_DEPS)

cop_tcp.o: cop_tcp.c $(COP_TCP_DEPS)
cop_api.o: cop_api.c $(COP_API_DEPS)
cop_sdo.o: cop_sdo.c $(COP_SDO_DEPS)
cop_nms.o: cop_nms.c $(COP_NMS_DEPS)
cop_lss.o: cop_lss.c $(COP_LSS_DEPS)
cop_lmt.o: cop_lmt.c $(COP_LMT_DEPS)

can_ctrl.o: can_ctrl.c $(CAN_CTRL_DEPS)


can_open: $(OBJECTS)
	$(CC) -o $(PROGRAM) $(LDFLAGS) $(OBJECTS)


# ### $Id: Makefile 30 2009-02-11 12:08:46Z saturn $ ###
