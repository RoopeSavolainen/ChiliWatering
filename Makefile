CC := avr-gcc
BOARD := attiny841
OUTFILE := main.hex

PORT := /dev/arduino

.PHONY:	upload clean

$(OUTFILE):	main.c
	$(CC) -mmcu=$(BOARD) main.c -Wall -Wextra -pedantic -Os -o $(OUTFILE)

upload:	$(OUTFILE)
	avrdude -c arduino -p $(BOARD) -P $(PORT) -b 19200 -U flash:w:$(OUTFILE)

clean:	
	@rm -f $(OUTFILE)
