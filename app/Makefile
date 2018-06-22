
INCLUDES = $(wildcard ../module/*.h)
INSTALL_PATH=/bsl

CC = gcc -g

#TARGET = nss_diag nss_diag64_sh nss_diag64 nss_diag64_acl\
				 nss_diag64_cam nss_diag64_cam_sh\
				 nss_diag64_dpram nss_diag64_cam_hole_sh\
         nss_diag64_dpram_hole nss_diag64_cam_hole \
				 nss_diag64_mdio nvram sendlog snorby \
         nss_diag_dma_buf nss_diag_dma_buf_tx \
		 nss_dump

TARGET =  bsl_main
TARGET +=  bsl_main_sim
TARGET +=  bsl_apicall_sim
TARGET += bsl_chassis_sim
#TARGET +=  bsl_msg_client
#TARGET +=  bsl_msg_client_sim
TARGET +=  bsl_diag64
TARGET +=  bsl_diag64_mdio
TARGET += bsl_link
TARGET += bsl_chassis

ifeq ($(BSL_LIBDIR),)
	BSL_LIBDIR = ../api
endif

BSL_LIB = $(BSL_LIBDIR)/libbsl.a
WD_BASEDIR := $(wildcard $(WD_BASEDIR))

ifeq ($(BSL_LIBDIR),)
	BSL_LIBDIR = ../api
endif

BSL_LIB = $(BSL_LIBDIR)/libbsl.a
BSL_SIM_LIB = $(BSL_LIBDIR)/libbsl.sim.a
BSL_INC = -I../api -I../module

CFLAGS += -DLINUX -Wall
LFLAGS += -lpthread -lpcap
TARGET_CFLAGS = -D_TARGET_

all: $(TARGET)

bsl_main: bsl_main.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_LIB) $(TARGET_CFLAGS) $(LFLAGS) $(BSL_INC) -o $@

bsl_main_sim: bsl_main.c $(BSL_SIM_LIB)
	$(CC) -Wall $< $(BSL_SIM_LIB) $(LFLAGS) $(BSL_INC) -o $@

bsl_apicall_sim: bsl_apicall.c $(BSL_SIM_LIB)
	$(CC) -Wall $< $(BSL_SIM_LIB) $(LFLAGS) $(BSL_INC) -o $@

bsl_chassis_sim: bsl_chassis.c $(BSL_SIM_LIB)
	$(CC) -Wall $< $(BSL_SIM_LIB) $(LFLAGS) $(BSL_INC) -o $@

bsl_link: bsl_link.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_LIB) $(LFLAGS) $(TARGET_CFLAGS) $(BSL_INC) -o $@

bsl_chassis: bsl_chassis.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_LIB) $(LFLAGS) $(TARGET_CFLAGS) $(BSL_INC) -o $@

bsl_msg_client: bsl_msg_client.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_LIB) $(LFLAGS) $(TARGET_CFLAGS) -lxml2 $(BSL_INC) -I/usr/include/libxml2 -o $@

bsl_dump: bsl_dump.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_LIB) $(LFLAGS) $(TARGET_CFLAGS) $(BSL_INC) -D_GNU_SOURCE -o $@

bsl_dump2: bsl_dump2.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_LIB) $(LFLAGS) $(TARGET_CFLAGS) $(BSL_INC) -D_GNU_SOURCE -o $@

bsl_msg_client_sim: bsl_msg_client.c $(BSL_LIB)
	$(CC) -Wall $< $(BSL_SIM_LIB) $(LFLAGS) -lxml2 $(BSL_INC) -I/usr/include/libxml2 -o $@

#bsl_diag64_sh: bsl_diag64_v0.01.c
#	$(CC) $(CFLAGS) -Wall $< $(BSL_LIB) -o $@

bsl_diag64: bsl_diag64.c
	$(CC) -Wall $(TARGET_CFLAGS) $< $(BSL_LIB) -o $@

bsl_diag64_mdio: bsl_diag64_mdio_ioctl.c $(INCLUDES)
	$(CC) $(CFLAGS) $(TARGET_CFLAGS) -Wall $< $(BSL_LIB) -o $@

install:
	cp -f $(TARGET) $(INSTALL_PATH)/diag

clean:
	rm -rf $(TARGET)
