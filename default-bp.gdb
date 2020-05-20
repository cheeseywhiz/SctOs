# save breakpoints bp.gdb
hbreak efi_debug_entry
  commands
    finish
  end
disable $bpnum
hbreak main
hbreak halt
  commands
    where
  end
