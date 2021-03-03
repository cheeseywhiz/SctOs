define leave
    monitor quit
    disconnect
    quit
end
define mir
    monitor info registers
end
define mim
    monitor info mem
end
# need to wait until the executable loads to enable software breakpoints
thbreak efi_main
    commands
        silent
        source efi.gdb
        continue
    end
hbreak kernel_main
    commands
        silent
        source kernel.gdb
        #continue
    end
continue
