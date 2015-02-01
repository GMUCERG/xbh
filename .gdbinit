target remote | openocd
file build/xbh.axf

define hook-step
monitor reg primask 1
end
define hookpost-step
monitor reg primask 0
end


