include makedefs

all: ${BUILDDIR}/usb_dev_serial.axf

include ${TIVA_MAKE_ROOT}/tiva.make

PART=TM4C123GH6PM
VPATH:=${TIVA_ROOT}/utils




${BUILDDIR}/usb_dev_serial.axf: |${BUILDDIR}
${BUILDDIR}/usb_dev_serial.axf: ${BUILDDIR}/startup_${COMPILER}.o
${BUILDDIR}/usb_dev_serial.axf: ${BUILDDIR}/uartstdio.o
${BUILDDIR}/usb_dev_serial.axf: ${BUILDDIR}/usb_dev_serial.o
${BUILDDIR}/usb_dev_serial.axf: ${BUILDDIR}/usb_serial_structs.o
${BUILDDIR}/usb_dev_serial.axf: ${BUILDDIR}/ustdlib.o
${BUILDDIR}/usb_dev_serial.axf: ${LIBDIR}/libusb.a
${BUILDDIR}/usb_dev_serial.axf: ${LIBDIR}/libdriver.a
${BUILDDIR}/usb_dev_serial.axf: usb_dev_serial.ld
SCATTERgcc_usb_dev_serial=usb_dev_serial.ld
ENTRY_usb_dev_serial=ResetISR
CFLAGSgcc=-DTARGET_IS_BLIZZARD_RB1 -DUART_BUFFERED


distclean: clean
	rm -rf build lib
clean:
	rm -rf build/*.o build/*.d build/*.bin build/*.axf tags
tags: 
	ctags -R . ${TIVA_ROOT}

.PHONY: clean

