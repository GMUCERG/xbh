#! /bin/sh
#lm4flash makefiles/TM4C123GH6PM_BL.bin 
#lm4flash -s 0F003065 build/xbh.axf
echo load|arm-stellaris-eabi-gdb
