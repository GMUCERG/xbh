all:  lib/libdriver.a
all:  lib/libusb.a
clean:
	rm -rf lib build
.PHONY: clean
include makedefs


