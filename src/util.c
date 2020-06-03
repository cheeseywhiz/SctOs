#include <stdint.h>
#include "opsys/x86.h"
#include "util.h"

void
break_(void)
{
}

/* dead stack -> dead stacc -> dead sacc -> dead 5acc */
uintptr_t __stack_chk_guard = 0xdead5accdead5acc;

void __stack_chk_fail(void);

void
__stack_chk_fail(void)
{
    halt();
}
