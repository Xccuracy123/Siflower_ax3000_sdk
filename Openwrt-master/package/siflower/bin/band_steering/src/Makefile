SRC = band_steering.c main.c

LIBS = -ljson-c -lubus -lubox -lblobmsg_json

band_steering:$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(INCLUDES) $(LIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

