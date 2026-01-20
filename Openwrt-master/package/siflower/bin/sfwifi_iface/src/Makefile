EXEC = sfwifi_iface
SRC = sfwifi_iface.c

all: $(EXEC)

sfwifi_iface:$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

