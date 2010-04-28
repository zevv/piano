
NAME	= piano

SRC	= piano.c

#############################################################################

OBJS	= $(subst .c,.o, $(SRC))
ELF	= $(NAME).elf
FHEX	= $(NAME).hex
EHEX	= $(NAME)-eeprom.hex

CFLAGS  += -mmcu=atmega644 -Wall -Werror -O3 -g -I.
CFLAGS	+= -DF_CPU=16000000 
LDFLAGS += -mmcu=atmega644 -g
ADFLAGS += -p m644 -c avrispv2 -P usb

CROSS	= avr-
CC 	= $(CROSS)gcc
LD 	= $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE	= $(CROSS)size
AD	= /opt/avrdude-5.1/bin/avrdude

all: $(ELF) $(FHEX)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) 
	$(SIZE) $(ELF)
	
$(FHEX) $(EHEX): $(ELF) 
	$(OBJCOPY) -j .text -j .data -O ihex $(ELF) $(FHEX)
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $(ELF) $(EHEX)

$(OBJS) $(ELF): Makefile

install program: $(FHEX) $(EHEX) 
	$(AD) $(ADFLAGS) -y -e -V \
		-U flash:w:$(FHEX):i \
		-U eeprom:w:$(EHEX):i 

install_fuses: $(FHEX) $(EHEX)
	$(AD) $(ADFLAGS) -U lfuse:w:0xef:m -U hfuse:w:0xd9:m

size:
	avr-size $(OBJS) | sort -n

clean:	
	rm -f $(OBJS) $(ELF) $(EHEX) $(FHEX)  dump 

.PHONY: doc
doc:
	doxygen
	if [ -d doc/latex ]; then make -C doc/latex; fi

#####################################################################

test: axis.c 
	gcc -o test axis.c misc.c -DUNIT_TEST

dump: test
	./test > dump

graph: dump
	gnuplot < unittest.gp | xv -

# End

