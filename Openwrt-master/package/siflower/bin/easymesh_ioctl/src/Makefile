EXEC = easymesh_ioctl
SRC = easymesh_ioctl.c

all: $(EXEC)

easymesh_ioctl:$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

