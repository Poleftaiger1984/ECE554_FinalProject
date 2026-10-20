CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-a7 -fpic -ffreestanding -O0 -Wall -Wextra
LDFLAGS = -T linker.ld -nostdlib

OBJS = start.o kernel.o render.o ringBuffer.o

all: kernel.elf

kernel.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o kernel.elf $(OBJS)

start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

ringBuffer.o: ringBuffer.c
	$(CC) $(CFLAGS) -c ringBuffer.c -o ringBuffer.o

render.o: render.c
	$(CC) $(CFLAGS) -c render.c -o render.o

clean:
	rm -f *.o *.elf