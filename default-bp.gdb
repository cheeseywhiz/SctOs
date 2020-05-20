# save breakpoints bp.gdb
hbreak debug_entry
  commands
    finish
  end
hbreak halt
  commands
    where
  end
