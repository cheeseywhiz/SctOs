/* 
 * generate functions for each interrupt and a table of the functions
 *
 * pseudocode:
 *     .section .text
 *     vectorNNN:
 *         if no error code:
 *             push $0
 *         push $NNN
 *         jump interrupt_handler
 *     .section .data
 *     .global vector_table
 *     vector_table:
 *         .quad vector0
 *         ...
 */

#include <stdio.h>
#include "opsys/x86.h"

int
main(void)
{
    puts(".section .text");

    for (uint16_t i = 0; i < N_INTERRUPTS; ++i) {
        printf("vector_%d:\n", i);

        if (EXC_HAS_ERROR_CODE(i))
            puts("\t/* error code on stack */");
        else
            puts("\tpushq\t$0");

        printf("\tpushq\t$%d\n", i);
        puts("\tjmp\tinterrupt_handler_stub");
    }

    puts(".section .data");
    puts(".global vector_table");
    puts("vector_table:");
    for (uint16_t i = 0; i < N_INTERRUPTS; ++i)
        printf("\t.quad vector_%d\n", i);
}
