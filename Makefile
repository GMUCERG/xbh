include makedefs

all: ${BUILDDIR}/hello.axf

include ${TIVA_MAKE_ROOT}/tiva.make

PART=TM4C123GH6PM
VPATH:=${TIVA_ROOT}/utils


${BUILDDIR}/hello.axf: |${BUILDDIR}
${BUILDDIR}/hello.axf: ${BUILDDIR}/hello.o
${BUILDDIR}/hello.axf: ${BUILDDIR}/startup_${COMPILER}.o
${BUILDDIR}/hello.axf: ${BUILDDIR}/uartstdio.o
${BUILDDIR}/hello.axf: ${LIBDIR}/libdriver.a
${BUILDDIR}/hello.axf: hello.ld

${BUILDDIR}: 
	mkdir -p ${BUILDDIR}
    

SCATTERgcc_hello=hello.ld
SCATTERgcc_hello=hello.ld
ENTRY_hello=ResetISR
CFLAGSgcc=-DTARGET_IS_BLIZZARD_RB1


clean:
	rm -rf lib build
.PHONY: clean
