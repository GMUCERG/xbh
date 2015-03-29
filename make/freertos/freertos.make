INCLUDE += ${FREERTOS_ROOT}/Source
INCLUDE += ${FREERTOS_ROOT}/Source/include
INCLUDE += ${FREERTOS_ROOT}/Source/portable/GCC/ARM_CM4F


${BUILDDIR}/freertos:
	@mkdir -p ${BUILDDIR}/freertos/Source/portable/GCC/ARM_CM4F
	@mkdir -p ${BUILDDIR}/freertos/Source/portable/MemMang

#
# Rules for building the driver library.
#
# The rule for building the object file from each C source file.
#
${BUILDDIR}/freertos/%.o: ${FREERTOS_ROOT}/%.c | ${BUILDDIR}/freertos
	@if [ 'x${VERBOSE}' = x ];                            \
	 then                                                 \
	     echo "  CC    ${<}";                             \
	 else                                                 \
	     echo ${CC} ${CFLAGS} -D${COMPILER} -o ${@} -c ${<}; \
	 fi
	@${CC} ${CFLAGS} -D${COMPILER} -o ${@} -c ${<}
ifneq ($(findstring CYGWIN, ${os}), )
	@sed -i -r 's/ ([A-Za-z]):/ \/cygdrive\/\1/g' ${@:.o=.d}
endif

FREERTOS_SOURCES := $(wildcard ${FREERTOS_ROOT}/Source/portable/GCC/ARM_CM4F/*.c) \
    $(wildcard ${FREERTOS_ROOT}/Source/portable/GCC/ARM_CM4F/*.s) \
	$(wildcard ${FREERTOS_ROOT}/Source/*.c)
FREERTOS_SOURCES += ${FREERTOS_ROOT}/Source/portable/MemMang/heap_4.c 

FREERTOS_OBJECTS := $(FREERTOS_SOURCES:%.c=%.o) 
FREERTOS_OBJECTS := $(patsubst ${FREERTOS_ROOT}%,${BUILDDIR}/freertos%,${FREERTOS_OBJECTS})

ifneq (${MAKECMDGOALS},clean)
FREERTOS_DEPS := $(FREERTOS_OBJECTS:%.o=%.d)
-include ${FREERTOS_DEPS} __dummy__
endif


${LIBDIR}/freertos.a: $(FREERTOS_OBJECTS)
