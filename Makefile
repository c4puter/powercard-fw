PROJECT = powercard
OBJECTS = main.o hardware.o
CHIP = atxmega128a1u

CC = avr-gcc
OBJDUMP = avr-objdump
OBJCOPY = avr-objcopy
SIZE = avr-size

CFLAGS = -mmcu=${CHIP} -DF_CPU=32000000uLL -std=c11 -Wall -Wextra -Werror \
		 -O2 -g

.PHONY:	all clean program

all:	${PROJECT}.elf ${PROJECT}.disasm

%.disasm: %.elf
	${OBJDUMP} -S $< > $@

%.hex: %.elf
	${OBJCOPY} -O ihex $< $@

${PROJECT}.elf:	${OBJECTS}
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} $^
	${SIZE} $@

program: ${PROJECT}.hex
	dfu-programmer ${CHIP} flash --force $<

clean:
	rm -f ${PROJECT}.elf ${PROJECT}.disasm ${OBJECTS}

