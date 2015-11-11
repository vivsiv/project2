CC=gcc
CFLAGS=-I.
DEPS = # header file 
OBJ = server.o 
OBJ2 = client.o

# %.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)

#BUILD SERVER
all: client server

client: client.o
	$(CC) -o $@ $^ $(CFLAGS)

server: server.o
	$(CC) -o $@ $^ $(CFLAGS)

server.o: server.c
	$(CC) -c -o $@ $< $(CFLAGS)

client.o: client.c
	$(CC) -c -o $@ $< $(CFLAGS)

# server: $(OBJ)
# 	$(CC) -o $@ $^ $(CFLAGS)

# client: $(OBJ2)
# 	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	rm -f *.o server client   
