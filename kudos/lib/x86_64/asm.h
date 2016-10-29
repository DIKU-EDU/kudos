/*
 * Tools Assembly
 */

#ifndef KUDOS_LIB_X86_64_ASM_H
#define KUDOS_LIB_X86_64_ASM_H

#include "lib/types.h"

/* In the X86 Architecture all communications with devices
 * is done through either ports or registers, the below functions
 * either recieves a byte/word/long from a port, or sends a byte/word/long
 * to a port */
void _outb(uint16_t Port, uint8_t Value);
void _outw(uint16_t Port, uint16_t Value);
void _outl(uint16_t Port, uint32_t Value);

uint8_t _inb(uint16_t Port);
uint16_t _inw(uint16_t Port);
uint32_t _inl(uint16_t Port);

void _insw(uint16_t Port, uint64_t Count, uint8_t *Buffer);
void _outsw(uint16_t Port, uint64_t Count, uint8_t *Buffer);

#endif // KUDOS_LIB_X86_64_ASM_H
