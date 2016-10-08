PROJECT = powercard
OBJECTS = main.o hardware.o leds.o regulator.o \
		  avr1308/twi_slave_driver.o \
		  esh/esh_argparser.o esh/esh.o esh/esh_hist.o
CHIP = atxmega32e5
AVRDUDE_CHIP = x32e5

CC = avr-gcc
OBJDUMP = avr-objdump
OBJCOPY = avr-objcopy
SIZE = avr-size

CFLAGS = -mmcu=${CHIP} -DF_CPU=32000000uLL -std=gnu11 -Wall -Wextra -Werror \
		 -O2 -g -flto -I esh -iquote . -I avr1308

.PHONY:	all clean program

all:	${PROJECT}.elf ${PROJECT}.disasm
	${SIZE} ${PROJECT}.elf

%.disasm: %.elf
	${OBJDUMP} -S $< > $@

%.hex: %.elf
	${OBJCOPY} -O ihex $< $@

${PROJECT}.elf:	${OBJECTS}
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} $^

program: ${PROJECT}.hex
	#dfu-programmer ${CHIP} flash --force $<
	avrdude -p ${AVRDUDE_CHIP} -c atmelice_pdi -U flash:w:$<

clean:
	rm -f ${PROJECT}.hex ${PROJECT}.disasm ${PROJECT}.elf ${OBJECTS}

