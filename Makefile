
CPPFLAGS+=-DCIK2=\"${CIK2}\"
BOARD:=ethernet
include ~/bin/arduino-mk/arduino.mk

m:
	tail -f $(SERIALDEV)
