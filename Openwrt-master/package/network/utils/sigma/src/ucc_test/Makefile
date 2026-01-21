ucc_OBJS = wfa_ucc.o
CFLAGS += -Wall -g 
all: wfa_ucc
%.o:%.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

ucc: $(ucc_OBJS)
	$(CC) -o $@ $(ucc_OBJS)

clean:
	rm -f rbcfg *.o
	rm -f rbcfg *.so
