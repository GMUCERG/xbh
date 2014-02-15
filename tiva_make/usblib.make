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
vpath=./device


#
# Where to find header files that do not live in the source directory.
#
#IPATH=..

#
# The rule to create the target directory.
#
${BUILDDIR}/usblib:
	@mkdir -p ${BUILDDIR}/usblib/device
	@mkdir -p ${BUILDDIR}/usblib/host


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
	     echo ${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}; \
	 fi
	@${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}
ifneq ($(findstring CYGWIN, ${os}), )
	@sed -i -r 's/ ([A-Za-z]):/ \/cygdrive\/\1/g' ${@:.o=.d}
endif


#
# Rules for building the USB library.
#
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbbuffer.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbdesc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbdma.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbkeyboardmap.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbmode.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbringbuf.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/usbtick.o
#
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdaudio.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdbulk.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdcdc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdcdesc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdcomp.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdconfig.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbddfu-rt.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdenum.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdhandler.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdhidkeyb.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdhidmouse.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdhid.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/device/usbdmsc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhaudio.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhhidkeyboard.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhhidmouse.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhhid.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhhub.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhmsc.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhostenum.o
${LIBDIR}/libusb.a: ${BUILDDIR}/usblib/host/usbhscsi.o


#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
