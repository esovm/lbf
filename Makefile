CC=gcc
CFLAGS=-O3
OBJS=lbf.o

all: lbf

lbf: $(OBJS)
	$(CC) $(CFLAGS) -s -o $@ $^

.PREFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) lbf
