/*
 * Tools Assembly
 */

#include <asm.h>

/* Send a long */
void _outl(uint16_t Port, uint32_t Value)
{
  asm volatile("outl %%eax, %%dx" : : "d" (Port), "a" (Value));
}

/*uint8_t _inb(uint16_t Port)
  {
  uint8_t res = 0;

  asm volatile("inb %%dx, %%al" : "=a"(res) : "d" (Port));

  return res;
  }*/

uint16_t _inw(uint16_t Port)
{
  uint16_t res = 0;

  asm volatile("inw %%dx, %%ax" : "=a"(res) : "d" (Port));

  return res;
}

uint32_t _inl(uint16_t Port)
{
  uint32_t res = 0;

  asm volatile("inl %%dx, %%eax" : "=a"(res) : "d" (Port));

  return res;
}

void _insw(uint16_t Port, uint64_t Count, uint8_t *Buffer)
{
  asm volatile("rep;insw" 
               : "=D"(Buffer), "=c"(Count)
               : "D"(Buffer), "c"(Count), "d"((uint64_t)Port)
               : "memory");
}

void _outsw(uint16_t Port, uint64_t Count, uint8_t *Buffer)
{
  asm volatile("rep;outsw" 
               : "=S"(Buffer), "=c"(Count)
               : "S"(Buffer), "c"(Count), "d"((uint64_t)Port)
               : "memory");
}
