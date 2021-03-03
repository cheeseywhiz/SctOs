break exit_status
break break_
    commands
        silent
        finish
    end
break halt
    commands
        silent
        where
    end
#hbreak kernel_main
#    commands
#        silent
#        delete
#        source kernel.gdb
#    end
