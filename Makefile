include makedefs

all: ${BUILDDIR}
all: ${BUILDDIR}/xbh.axf

${BUILDDIR}: 
	mkdir -p ${BUILDDIR}

# tiva.make must go first, since tiva makedefs are used for everything else
include ${TIVA_MAKE_ROOT}/tiva.make
include ${FREERTOS_MAKE_ROOT}/freertos.make
include ${LWIP_MAKE_ROOT}/lwip.make


XBH_SOURCES := $(PROJECT_ROOT)/startup_gcc.c
XBH_SOURCES += $(PROJECT_ROOT)/hal/hal.c
XBH_SOURCES += $(PROJECT_ROOT)/hal/lwip_eth.c
XBH_SOURCES += $(PROJECT_ROOT)/main.c
XBH_SOURCES += $(PROJECT_ROOT)/util.c
XBH_SOURCES += $(PROJECT_ROOT)/xbhserver.c





#XBH_SOURCES := $(wildcard $(PROJECT_ROOT)/*.c) #$(wildcard $(PROJECT_ROOT)/lwip_port/*.c)
XBH_OBJECTS := $(XBH_SOURCES:%.c=%.o) 
XBH_OBJECTS := $(subst ${PROJECT_ROOT},${BUILDDIR}/xbh,${XBH_OBJECTS})

${BUILDDIR}/xbh: 
	@mkdir -p ${BUILDDIR}/xbh

${BUILDDIR}/xbh/hal: 
	@mkdir -p ${BUILDDIR}/xbh/hal



${BUILDDIR}/xbh/%.o: ${PROJECT_ROOT}/%.c | ${BUILDDIR}/xbh ${BUILDDIR}/xbh/hal
	@if [ 'x${VERBOSE}' = x ];                            \
	 then                                                 \
	     echo "  CC    ${<}";                             \
	 else                                                 \
	     echo ${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}; \
	 fi
	@${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}
ifneq ($(findstring CYGWIN, ${os}), )
	@sed -i -r 's/ ([A-Za-z]):/ \/cygdrive\/\1/g' ${@:.o=.d}
endif


${BUILDDIR}/xbh.axf: ${XBH_OBJECTS}
${BUILDDIR}/xbh.axf: ${LIBDIR}/freertos.a
${BUILDDIR}/xbh.axf: ${LIBDIR}/libdriver.a
${BUILDDIR}/xbh.axf: ${LIBDIR}/liblwip.a
${BUILDDIR}/xbh.axf: xbh.ld
SCATTERgcc_xbh=xbh.ld
ENTRY_xbh=ResetISR
#CFLAGSgcc+=-std=gnu99 -DDEBUG -DLWIP_DEBUG -DDEBUG_STACK -g -O0
CFLAGSgcc+=-std=gnu99 -DDEBUG -DLWIP_DEBUG -g -O0

#${BUILDDIR}/usb_dev_bulk.axf: |${BUILDDIR}
#${BUILDDIR}/usb_dev_bulk.axf: ${BUILDDIR}/startup_${COMPILER}.o
#${BUILDDIR}/usb_dev_bulk.axf: ${BUILDDIR}/uartstdio.o
#${BUILDDIR}/usb_dev_bulk.axf: ${BUILDDIR}/usb_dev_bulk.o
#${BUILDDIR}/usb_dev_bulk.axf: ${BUILDDIR}/usb_bulk_structs.o
#${BUILDDIR}/usb_dev_bulk.axf: ${BUILDDIR}/ustdlib.o
#${BUILDDIR}/usb_dev_bulk.axf: ${LIBDIR}/libusb.a
#${BUILDDIR}/usb_dev_bulk.axf: ${LIBDIR}/libdriver.a
#${BUILDDIR}/usb_dev_bulk.axf: usb_dev_bulk.ld
#SCATTERgcc_usb_dev_bulk=usb_dev_bulk.ld
#ENTRY_usb_dev_bulk=ResetISR
#CFLAGSgcc=-DTARGET_IS_BLIZZARD_RB1 -DUART_BUFFERED


ifneq (${MAKECMDGOALS},clean)
XBH_DEPS := $(XBH_OBJECTS:%.o=%.d)
-include ${XBH_DEPS} __dummy__
endif


distclean: clean
	rm -rf build lib
clean:
	rm -rf build/*.o build/*.d build/*.bin build/*.axf tags
tags: 
	ctags -R . \
		${TIVA_ROOT}/driverlib \
		${TIVA_ROOT}/inc \
		${LWIP_ROOT} \
		${LWIP_PORT} \
		${FREERTOS_ROOT}/Source/include \
		${FREERTOS_ROOT}/Source/*.c \
		${FREERTOS_ROOT}/Source/portable/MemMang \
		${FREERTOS_ROOT}/Source/portable/GCC/ARM_CM4F \
		$(shell echo| `arm-stellaris-eabi-gcc -print-prog-name=cc1` -v 2>&1| awk '/#include <...>/ {flag=1;next} /End of search list/{flag=0} flag {print $$0 " "}')
		
		

KEYWORDS=TODO:|FIXME:|\?\?\?:|\!\!\!:
todo: 
	@find "${PROJECT_ROOT}" \( -name "*.h" -or -name "*.c" \) -and \( ! -path "${PROJECT_ROOT}/examples/*" -and ! -path "${PROJECT_ROOT}/deps/*" \) -print0 \
	| xargs -0 egrep --with-filename --line-number --only-matching "(${KEYWORDS}).*$"" \
	| perl -p -e 's/(${KEYWORDS})/ warning:  $$1/'

.PHONY: clean tags


