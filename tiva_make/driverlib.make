#******************************************************************************
#
# Makefile - Rules for building the driver library.
#
# Copyright (c) 2005-2013 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
# 
#   Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
#   Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the  
#   distribution.
# 
#   Neither the name of Texas Instruments Incorporated nor the names of
#   its contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# This is part of revision 1.0 of the Tiva Peripheral Driver Library.
#
#******************************************************************************

#
# The base directory for TivaWare.
# EDIT: Use TIVA_ROOT
#ROOT=..

#
# Include the common make definitions.
#
include ${TIVA_MAKE_ROOT}/tiva.makedefs

#
# Where to find header files that do not live in the source directory.
# EDIT: Use ${INCLUDE}
#INCLUDE:=${INCLUDE}:${TIVA_ROOT}

#
# The default rule, which causes the driver library to be built.
#
#${LIBDIR}/libdriver.a:

#
# The rule to clean out all the build products.
#
#clean:
#	@rm -rf ${BUILDDIR}/ ${wildcard *~}

#
# The rule to create the target directory.
#

${BUILDDIR}/driverlib: 
	@mkdir -p ${BUILDDIR}/driverlib

#
# Rules for building the driver library.
#
# The rule for building the object file from each C source file.
#
${BUILDDIR}/driverlib/%.o: ${TIVA_ROOT}/driverlib/%.c | ${BUILDDIR}/driverlib
	@if [ 'x${VERBOSE}' = x ];                            \
	 then                                                 \
	     echo "  CC    ${<}";                             \
	 else                                                 \
	     echo ${CC} ${CFLAGS} -D${BUILDDIR}/driverlib -o ${@} ${<}; \
	 fi
	@${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}
ifneq ($(findstring CYGWIN, ${os}), )
	@sed -i -r 's/ ([A-Za-z]):/ \/cygdrive\/\1/g' ${@:.o=.d}
endif


${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/adc.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/can.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/comp.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/cpu.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/eeprom.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/flash.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/fpu.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/gpio.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/hibernate.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/i2c.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/interrupt.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/mpu.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/pwm.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/qei.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/ssi.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/sysctl.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/sysexc.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/systick.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/timer.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/uart.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/udma.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/usb.o
${LIBDIR}/libdriver.a: ${BUILDDIR}/driverlib/watchdog.o


#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
