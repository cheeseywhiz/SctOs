define leave
    monitor quit
    disconnect
    quit
end
# need to wait until the executable loads to enable software breakpoints
thbreak efi_main
    commands
        silent
        source efi.gdb
        continue
    end
continue
