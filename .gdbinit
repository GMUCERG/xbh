target remote | openocd
file build/xbh.axf

define hook-stop
monitor reg primask 1
end
define hook-run
monitor reg primask 0
end
define hook-continue
monitor reg primask 0
end

