#******************************************************************************
#
# Makefile - Rules for building the utils library.
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
#include ${TIVA_MAKE_ROOT}/tiva.makedefs

#
# Where to find header files that do not live in the source directory.
# EDIT: Use ${INCLUDE}
#INCLUDE:=${INCLUDE}:${TIVA_ROOT}

#
# The default rule, which causes the utils library to be built.
#
#${LIBDIR}/libutils.a:

#
# The rule to clean out all the build products.
#
#clean:
#	@rm -rf ${BUILDDIR}/ ${wildcard *~}

#
# The rule to create the target directory.
#

${BUILDDIR}/utils: 
	@mkdir -p ${BUILDDIR}/utils

#
# Rules for building the utils library.
#
# The rule for building the object file from each C source file.
#
${BUILDDIR}/utils/%.o: ${TIVA_ROOT}/utils/%.c | ${BUILDDIR}/utils
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


${LIBDIR}/libutils.a: ${BUILDDIR}/utils/cmdline.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/cpu_usage.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/flash_pb.o
#${LIBDIR}/libutils.a: ${BUILDDIR}/utils/fswrapper.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/isqrt.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/locator.o
#${LIBDIR}/libutils.a: ${BUILDDIR}/utils/lwiplib.o
#${LIBDIR}/libutils.a: ${BUILDDIR}/utils/ptpdlib.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/random.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/ringbuf.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/scheduler.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/sine.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/smbus.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/softi2c.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/softssi.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/softuart.o
#${LIBDIR}/libutils.a: ${BUILDDIR}/utils/speexlib.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/spi_flash.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/swupdate.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/tftp.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/uartstdio.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/ustdlib.o
${LIBDIR}/libutils.a: ${BUILDDIR}/utils/wavfile.o


#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif


