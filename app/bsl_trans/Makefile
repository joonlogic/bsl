CC = g++ -g
CC_FLAGS = -c -I. 
#-DDEBUG

LD = g++
LD_FLAGS = -I. 

LIBS=-lpcap

OBJS_MQTRANS = bsl_trans.o 
OBJS_MQDUMP = bsl_dump.o 
OBJS_MQBPS = bsl_bps.o 
OBJS = $(OBJS_MQTRANS) $(OBJS_MQDUMP) $(OBJS_MQBPS) 

TARGET = bsl_trans 

all: $(TARGET)

bsl_trans.o : bsl_trans.cpp 
	$(CC) $(CC_FLAGS) bsl_trans.cpp

bsl_dump.o : bsl_dump.cpp 
	$(CC) $(CC_FLAGS) bsl_dump.cpp

bsl_bps.o : bsl_bps.cpp 
	$(CC) $(CC_FLAGS) bsl_bps.cpp

bsl_trans: $(OBJS_MQTRANS)
	$(LD) $(LD_FLAGS) -o ./bsl_trans $(OBJS_MQTRANS) $(LIBS)

bsl_dump: $(OBJS)
	$(LD) $(LD_FLAGS) -o ./bsl_dump $(OBJS_MQDUMP) 

bsl_bps: $(OBJS)
	$(LD) $(LD_FLAGS) -o ./bsl_bps $(OBJS_MQBPS) 

clean:
	rm -f $(OBJS) bsl_trans bsl_dump bsl_bps
