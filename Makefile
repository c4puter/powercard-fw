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

.PHONY:	all clean program fuses

all:	${PROJECT}.elf ${PROJECT}.disasm
	${SIZE} ${PROJECT}.elf

%.disasm: %.elf
	${OBJDUMP} -S $< > $@

%.hex: %.elf
	${OBJCOPY} -O ihex $< $@

${PROJECT}.elf:	${OBJECTS}
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} $^

program: ${PROJECT}.hex
	avrdude -p ${AVRDUDE_CHIP} -c atmelice_pdi -U flash:w:$<

# When disconnected, the "12V" rail does not fall below ~2.8V for a
# very long time. While the UVLO of the 3.3V regulator on the power
# supply board typically cuts out before then, I don't want to
# guarantee that it'll always fall under the UVLO threshold, so enable
# BOD at the highest level.
#
# fuse2 = 0xfd: Sampled BOD in power-down
# fuse5 = 0xe8: Continuous BOD in power-up, BOD level 3.0V
fuses:
	avrdude -p ${AVRDUDE_CHIP} -c atmelice_pdi -U fuse2:w:0xfd:m -U fuse5:w:0xe8:m

clean:
	rm -f ${PROJECT}.hex ${PROJECT}.disasm ${PROJECT}.elf ${OBJECTS}

