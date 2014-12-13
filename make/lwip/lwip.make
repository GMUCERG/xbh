INCLUDE += ${LWIP_ROOT}/src/include
INCLUDE += ${LWIP_ROOT}/src/include/ipv4
INCLUDE += ${LWIP_ROOT}/src/include/ipv6
INCLUDE += ${LWIP_ROOT}/apps
INCLUDE += ${LWIP_PORT}/include


${BUILDDIR}/lwip:
	@mkdir -p ${BUILDDIR}/lwip/src/core/ipv4
	@mkdir -p ${BUILDDIR}/lwip/src/core/ipv6
	@mkdir -p ${BUILDDIR}/lwip/src/api
	@mkdir -p ${BUILDDIR}/lwip/src/netif
	@mkdir -p ${BUILDDIR}/lwip/port/netif

#
# Rules for building the driver library.
#
# The rule for building the object file from each C source file.
#
${BUILDDIR}/lwip/%.o: ${LWIP_ROOT}/%.c | ${BUILDDIR}/lwip
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


${BUILDDIR}/lwip/port/%.o: ${LWIP_PORT}/%.c | ${BUILDDIR}/lwip
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



LWIP_SOURCES := $(wildcard ${LWIP_ROOT}/src/core/*.c) \
	$(wildcard ${LWIP_ROOT}/src/core/ipv4/*.c) \
	$(wildcard ${LWIP_ROOT}/src/core/ipv6/*.c) \
	$(wildcard ${LWIP_ROOT}/src/api/*.c) \
	${LWIP_ROOT}/src/netif/etharp.c \
	$(wildcard ${LWIP_PORT}/*.c) \
	$(wildcard ${LWIP_PORT}/netif/*.c)
LWIP_OBJECTS := $(LWIP_SOURCES:%.c=%.o) 
LWIP_OBJECTS := $(subst ${LWIP_PORT},${BUILDDIR}/lwip/port,${LWIP_OBJECTS})
LWIP_OBJECTS := $(subst ${LWIP_ROOT},${BUILDDIR}/lwip,${LWIP_OBJECTS})

ifneq (${MAKECMDGOALS},clean)
LWIP_DEPS := $(LWIP_OBJECTS:%.o=%.d)
-include ${LWIP_DEPS} __dummy__
endif


${LIBDIR}/liblwip.a: $(LWIP_OBJECTS)
