#******************************************************************************
#
# Makefile - Rules for building the USB library.
#
# Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
# 
# This is part of revision 1.0 of the Tiva USB Library.
#
#******************************************************************************

#
# The base directory for TivaWare.
#
#ROOT=..

#
# Include the common make definitions.
#
include ${TIVA_MAKE_ROOT}/tiva.makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=${TIVA_ROOT}/usblib/device
VPATH+=${TIVA_ROOT}/usblib/host

#
# Where to find header files that do not live in the source directory.
#
#IPATH=..

#
# The rule to create the target directory.
#
${BUILDDIR}/usblib:
	@mkdir -p ${BUILDDIR}/usblib


#
# Rules for building the driver library.
#
# The rule for building the object file from each C source file.
#
${BUILDDIR}/usblib/%.o: ${TIVA_ROOT}/usblib/%.c | ${BUILDDIR}/usblib
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


#
# Rules for building the USB library.
#
${LIBDIR}/libusb.a: ${BUILDDIR}/usbbuffer.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdaudio.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdbulk.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdcdc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdcdesc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdcomp.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdconfig.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbddfu-rt.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdenum.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdesc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdhandler.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdhid.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdhidkeyb.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdhidmouse.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdma.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbdmsc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhaudio.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhhid.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhhidkeyboard.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhhidmouse.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhhub.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhmsc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhostenum.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbhscsi.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbkeyboardmap.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbmode.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbringbuf.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usbtick.o

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
