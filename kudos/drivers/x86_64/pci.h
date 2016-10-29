/*
 * The pci interface
 */

#ifndef KUDOS_DRIVERS_X86_64_PCI_H
#define KUDOS_DRIVERS_X86_64_PCI_H

/* Includes */
#include <arch.h>
#include "lib/types.h"
#include "drivers/modules.h"

/* Defines */

/* Structures */
typedef struct pci_conf_space
{
  /* Offset 0x0 */
  uint16_t vendor_id;
  uint16_t device_id;

  /* Offset 0x4 */
  uint16_t command;
  uint16_t status;

  /* Offset 0x8 */
  uint8_t revision;
  uint8_t prog_if;
  uint8_t subclass;
  uint8_t classcode;

  /* Offset 0xC */
  uint8_t cache_size;
  uint8_t latency_timer;
  uint8_t header_type;
  uint8_t bist;

  /* Offset 0x10 */
  uint32_t bar0;

  /* Offset 0x14 */
  uint32_t bar1;

  /* Offset 0x18 */
  uint32_t bar2;

  /* Offset 0x1C */
  uint32_t bar3;

  /* Offset 0x20 */
  uint32_t bar4;

  /* Offset 0x24 */
  uint32_t bar5;

  /* Offset 0x28 */
  uint32_t cis_pointer;

  /* Offset 0x2C */
  uint16_t sub_vendor_id;
  uint16_t sub_id;

  /* Offset 0x30 */
  uint32_t rom_base_addr;

  /* Offset 0x34 */
  uint64_t Reserved;

  /* Offset 0x3C */
  uint8_t irq_line;
  uint8_t irq_pin;
  uint8_t min_grant;
  uint8_t max_latency;

} __attribute__((packed)) pci_conf_t;

#define pci_module_init(name, handler, classcode, subclass) \
    static pci_device_module_t __module_pci__##name \
    __attribute__ ((section ("real_modules_ptr"))) = \
    {classcode, subclass, handler}; \
    module_define(MODULE_TYPE_PCI, name, &__module_pci__##name)

typedef int(*pci_device_handler)(io_descriptor_t*);

typedef struct {
    uint8_t classcode;
    uint8_t subclass;
    pci_device_handler device_handler;
} pci_device_module_t;
#endif // KUDOS_DRIVERS_X86_64_PCI_H
