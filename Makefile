PART = t85
PORT = /dev/spidev0.0
AVRFLAGS = -mmcu=attiny85 -fshort-enums -fno-inline -g -Isrc/
AVRCFLAGS = $(AVRFLAGS) -Os -std=gnu99 -mcall-prologues -DF_CPU=1000000
AVRSFLAGS = $(AVRFLAGS) -x assembler-with-cpp
CFLAGS = -Os -std=gnu99

all: bin/firmware.dump

.PHONY : upload fuses clean

upload: bin/firmware.elf
	sudo avrdude -b 100000 -c linuxspi -p $(PART) -P $(PORT) -U flash:w:bin/firmware.elf

#fuses:
#	sudo avrdude -b 100000 -c linuxspi -p $(PART) -P $(PORT) -U lfuse:w:0xF2:m

clean:
	rm -f bin/* obj/*

bin/firmware.dump: bin/firmware.elf
	avr-objdump -xd $< > $@

bin/firmware.elf: obj/firmware.o 
	avr-gcc $(AVRCFLAGS) -o$@ $^

#obj/firmware.s: src/firmware.c
#	avr-gcc $(AVRCFLAGS) -S -o$@ $<

obj/firmware.o: src/firmware.c
	avr-gcc $(AVRCFLAGS) -c -o$@ $<
