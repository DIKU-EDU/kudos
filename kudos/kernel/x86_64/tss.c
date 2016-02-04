/*
 * The task switch segment descriptor.
 */

#include <gdt.h>
#include <tss.h>
#include "kernel/config.h"
#include "lib/libc.h"

tss_t tss_descriptors[CONFIG_MAX_CPUS];

/* Helpers */
void tss_flush(uint16_t tss)
{
  asm volatile("ltr %%ax" : : "a"(tss));
}

/* Init */
void tss_init(void)
{
  /* Null out entries */
  memoryset(&tss_descriptors[0], 0, sizeof(tss_t) * CONFIG_MAX_CPUS);
}

/* Sets up TSS descriptors for each cpu */
void tss_install(int num_cpu, uint64_t cpu_stack)
{
  /* Null out TSS */
  uint64_t tss_base = (uint64_t)&tss_descriptors[num_cpu];
  memoryset((uint64_t*)tss_base, 0, sizeof(tss_t));

  /* Install it */
  gdt_install_tss(tss_base, sizeof(tss_t));

  /* Set stack */
  tss_descriptors[num_cpu].rsp_ring0 = cpu_stack;
  tss_descriptors[num_cpu].io_map = 0xFFFF;
  tss_descriptors[num_cpu].ist[0] = cpu_stack;

  /* Update hardware task register */
  tss_flush((uint16_t)((5 + num_cpu) * sizeof(gdt_desc_t)));
}

/* Update stacks */
void tss_setstack(uint32_t cpu, uint64_t stack)
{
  tss_descriptors[cpu].rsp_ring0 = stack;
}

void tss_setuserstack(uint32_t cpu, uint64_t stack)
{
  tss_descriptors[cpu].rsp_ring2 = stack;
}
