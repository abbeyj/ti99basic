OBJS=tibasic.o runtime_functions.o rom.o grom.o
CFLAGS=-Wall -O3
.SUFFIXES : .bin

all: tibasic

.bin.c:
	perl bin2c.pl $< $@

tibasic: $(OBJS)
	$(CC) -o tibasic $(OBJS)

clean:
	rm -f $(OBJS) tibasic

