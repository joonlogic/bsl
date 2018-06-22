CC = gcc
BSL_SRC = common.c
BSL_SRC += bsl_socket.c bsl_msgif.c bsl_msg_handle.c bsl_api.c bsl_device.c bsl_util.c 

VERSION_BSL_API = 01.01.00
VERSION_BSL_API_STR = "01.01.00"
VERSION_BSL_API_INT = 0x00010100

BSL_OBJ = $(BSL_SRC:.c=.o)
BSL_SOBJ = $(BSL_SRC:.c=.s.o)
BSL_SIM_OBJ = $(BSL_SRC:.c=.sim.o)
BSL_VTSS_OBJ = $(BSL_SRC:.c=.vtss.o)
BSL_SIM_SOBJ = $(BSL_SRC:.c=.sim.s.o)

BSL_LIB = libbsl.a
BSL_SLIB = libbsl.so
BSL_SIM_LIB = libbsl.sim.a
BSL_SIM_SLIB = libbsl.sim.so
BSL_VTSS_LIB = libbsl.vtss.a

CFLAGS += -DLINUX -Wall -g -I../module -DVERSION_BSL_API_STR=\"01.01.00\" -DVERSION_BSL_API_INT=0x00010100
# CFLAGS += -D_TARGET_
TARGET_CFLAGS = -D_TARGET_
VTSS_CFLAGS = -D_VTSS_
CFLAGS += -D_GNU_SOURCE
#LFLAGS = -lpcap -lpthread
LFLAGS_SO += -shared $(LFLAGS) -lpcap -pthread
CFLAGS_SO += $(CFLAGS) -fPIC


CFLAGS += -DREMOVE_BCAP 
#CFLAGS += -DDO_SWAP_PLOAD

INCLUDES = $(wildcard *.h)

TARGET = $(BSL_LIB) $(BSL_SLIB) $(BSL_VTSS_LIB) $(BSL_SIM_LIB) $(BSL_SIM_SLIB) 
all: $(TARGET)

%.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS) $(TARGET_CFLAGS) $< -o $@

%.s.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS_SO) $(TARGET_CFLAGS) $< -o $@

%.sim.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS) $< -o $@

%.vtss.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS) $(TARGET_CFLAGS) $(VTSS_CFLAGS) $< -o $@

%.sim.s.o: %.c ${INCLUDES}
	$(CC) -c $(CFLAGS_SO) $< -o $@

$(BSL_LIB): $(BSL_OBJ)
	$(AR) rcs $(BSL_LIB) $(LFLAGS) $(BSL_OBJ)

$(BSL_SLIB): $(BSL_SOBJ)
	$(CC) $(LFLAGS_SO) -o $@.$(VERSION_BSL_API) $(BSL_SOBJ)

$(BSL_SIM_LIB): $(BSL_SIM_OBJ)
	$(AR) rcs $(BSL_SIM_LIB) $(BSL_SIM_OBJ)

$(BSL_VTSS_LIB): $(BSL_VTSS_OBJ)
	$(AR) rcs $(BSL_VTSS_LIB) $(BSL_VTSS_OBJ)

$(BSL_SIM_SLIB): $(BSL_SIM_SOBJ)
	$(CC) $(LFLAGS_SO) -o $@.$(VERSION_BSL_API) $(BSL_SIM_SOBJ)

# apitest: $(BSL_LIB) apitest.c
# 	$(CC) apitest.c $(BSL_LIB) -o apitest

install:
	cp -f $(BSL_LIB) $(INSTALL_PATH)/lib/

install_lib:
	cp -f $(BSL_SLIB).$(VERSION_BSL_API) /usr/lib
	rm -rf /usr/lib/$(BSL_SLIB)
	ln -s $(BSL_SLIB).$(VERSION_BSL_API) /usr/lib/$(BSL_SLIB)

install_simlib:
	cp -f $(BSL_SIM_SLIB).$(VERSION_BSL_API) /usr/lib
	ln -s $(BSL_SIM_SLIB).$(VERSION_BSL_API) /usr/lib/$(BSL_SIM_SLIB)

clean:
	rm -rf *.o
	rm -rf $(BSL_LIB)
	rm -rf $(BSL_SLIB)
	rm -rf /usr/lib/$(BSL_SLIB)
	rm -rf $(BSL_SIM_LIB)
	rm -rf $(BSL_VTSS_LIB)
	rm -rf $(BSL_SIM_SLIB)*
	rm -rf apitest
