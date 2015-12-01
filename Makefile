CC=gcc
CFLAGS=-I.
DEPS = rdt_packet.h
OBJ = sender.o 
OBJ2 = receiver.o

# %.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)

#BUILD SENDER
all: receiver sender

receiver: receiver.o
	$(CC) -o $@ $^ $(CFLAGS)

sender: sender.o
	$(CC) -o $@ $^ $(CFLAGS)

sender.o: sender.c
	$(CC) -c -o $@ $< $(CFLAGS)

receiver.o: receiver.c
	$(CC) -c -o $@ $< $(CFLAGS)

# sender: $(OBJ)
# 	$(CC) -o $@ $^ $(CFLAGS)

# receiver: $(OBJ2)
# 	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	rm -f *.o sender receiver   
