
BOARD:=ethernet
include ~/bin/arduino-mk/arduino.mk

m:
	tail -f $(SERIALDEV)
