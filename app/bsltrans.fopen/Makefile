CC = g++ -g
CC_FLAGS = -c -I. 
#-DDEBUG

LD = g++
LD_FLAGS = -I. 

LIBS=-lpcap

OBJS= bsl_trans.o

TARGET = bsl_trans

all: $(TARGET)

bsl_trans.o : bsl_trans.cpp 
	$(CC) $(CC_FLAGS) bsl_trans.cpp

bsl_trans: $(OBJS)
	$(LD) $(LD_FLAGS) -o ./bsl_trans $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) bsl_trans 
