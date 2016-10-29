/*
 * The task switch segment descriptor.
 */

#ifndef KUDOS_KERNEL_X86_64_TSS_H
#define KUDOS_KERNEL_X86_64_TSS_H

/* Includes */
#include "lib/types.h"

/* Defines */
typedef struct tss
{
  /* Link to previous TSS */
  uint32_t prev_tss;

  /* Stacks and stack segments */
  uint64_t rsp_ring0;
  uint64_t rsp_ring1;
  uint64_t rsp_ring2;

  /* Reserved */
  uint64_t reserved0;

  /* IST */
  uint64_t ist[7];

  /* Reserved */
  uint64_t reserved1;
  uint16_t reserved2;

  /* IO Map */
  uint16_t io_map;

} tss_t;

/* Prototypes */
void tss_init(void);
void tss_install(int num_cpu, uint64_t cpu_stack);

/* Update stacks */
void tss_setstack(uint32_t cpu, uint64_t stack);
void tss_setuserstack(uint32_t cpu, uint64_t stack);


#endif // KUDOS_KERNEL_X86_64_TSS_H
