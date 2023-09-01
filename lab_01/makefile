FAMILY=-mpic14
MCU=-p12f683
CC=sdcc
LD=sdcc
CFLAGS=-I. -I/usr/local/share/sdcc/non-free/include
TARGET=dice

SRCS =dice.c

all:
	${CC} --use-non-free ${FAMILY} ${MCU} ${CFLAGS} -o ${TARGET} ${SRCS}

sim:
	simulide ${TARGET}.simu

clean:
	rm -f *.c~ *.h~ *.o *.elf *.hex *.asm *.cod *.lst