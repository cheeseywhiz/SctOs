#break init_apic
#break init_apic_impl
hbreak interrupt_handler
    commands
        silent
        print *frame
        #continue
    end
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
