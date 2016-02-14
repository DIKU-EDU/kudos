/*
 * Halt the system from userland.
 */

#include "lib.h"

int main(void) {
  syscall_halt();
  return 0;
}
