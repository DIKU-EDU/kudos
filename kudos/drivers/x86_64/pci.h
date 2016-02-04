/*
 * The pci interface
 */

#ifndef __PCI_H__
#define __PCI_H__

/* Includes */
#include "lib/types.h"

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

#endif

