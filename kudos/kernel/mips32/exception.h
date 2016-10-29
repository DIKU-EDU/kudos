/*
 * Exception handling.
 */

#ifndef KUDOS_KERNEL_MIPS32_EXCEPTION_H
#define KUDOS_KERNEL_MIPS32_EXCEPTION_H

#define EXCEPTION_INTR 0
#define EXCEPTION_TLBM 1
#define EXCEPTION_TLBL 2
#define EXCEPTION_TLBS 3
#define EXCEPTION_ADDRL 4
#define EXCEPTION_ADDRS 5
#define EXCEPTION_BUSI 6
#define EXCEPTION_BUSD 7
#define EXCEPTION_SYSCALL 8
#define EXCEPTION_BREAK 9
#define EXCEPTION_RESVI 10
#define EXCEPTION_COPROC 11
#define EXCEPTION_AOFLOW 12
#define EXCEPTION_TRAP 13

void kernel_exception_handle(int exception);
void user_exception_handle(int exception);

#endif // KUDOS_KERNEL_MIPS32_EXCEPTION_H
