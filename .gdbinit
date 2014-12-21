target remote :3333
file build/xbh.axf

define hook-step
#call dbg_maskisr()
call vTaskSuspendAll()
call IntMasterDisable()
end
define hookpost-step
call IntMasterEnable()
call xTaskResumeAll()
end


