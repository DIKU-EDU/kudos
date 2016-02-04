/*
 * The programmable interrupt controller.
 */

#include <pic.h>
#include <asm.h>
#include "kernel/interrupt.h"
#include "lib/libc.h"

void pic_sendcommand(uint8_t Pic, uint8_t Command)
{
  /* Sanity */
  if(Pic > 1)
    return;

  /* Get register */
  uint16_t Reg = (Pic == SECONDARY_PIC) ? (uint16_t)PIC1_COMMAND_REGISTER :
    (uint16_t)PIC0_COMMAND_REGISTER;

  /* Send byte */
  _outb(Reg, Command);
}

void pic_senddata(uint8_t Pic, uint8_t Data)
{
  /* Sanity */
  if(Pic > 1)
    return;

  /* Get register */
  uint16_t Reg = (Pic == SECONDARY_PIC) ? (uint16_t)PIC1_INT_MASK_REGISTER :
    (uint16_t)PIC0_INT_MASK_REGISTER;

  /* Send byte */
  _outb(Reg, Data);
}

uint8_t pic_getdata(uint8_t Pic)
{
  /* Sanity */
  if(Pic > 1)
    return 0;

  /* Get register */
  uint16_t Reg = (Pic == SECONDARY_PIC) ? (uint16_t)PIC1_INT_MASK_REGISTER :
    (uint16_t)PIC0_INT_MASK_REGISTER;

  /* Get byte */
  return _inb(Reg);
}

void pic_init()
{
  /* Control Word */
  uint8_t icw = 0;

  /* Setup initialization control word 1 */
  icw |= PIC_ICW1_MASK_INIT | PIC_ICW1_MASK_IC4;

  /* Send to both master and slave to start them */
  pic_sendcommand(PRIMARY_PIC, icw);    //11
  pic_sendcommand(SECONDARY_PIC, icw);

  /* Remap PICS */
  pic_senddata(PRIMARY_PIC, 0x20); /* Pic 0 start at 0x20 */
  pic_senddata(SECONDARY_PIC, 0x28); /* Pic 1 start at 0x28 */

  /* Send initialization control word 3
   * this is to establish the connection between master
   * and slave controllers (we set primary as master, and secondary as slave) */
  pic_senddata(PRIMARY_PIC, 0x04);
  pic_senddata(SECONDARY_PIC, 0x02);

  /* Last step is to send initialization control word 4 */
  /* We tell controller to enable i86 mode */
  icw = PIC_ICW4_MASK_UPM;

  pic_senddata(PRIMARY_PIC, icw);
  pic_senddata(SECONDARY_PIC, icw);

  /* Activate IRQs */
  pic_senddata(PRIMARY_PIC, 0);
  pic_senddata(SECONDARY_PIC, 0);

  /* enable interrupts */
  _interrupt_enable();

  /* Kickstart things */
  pic_eoi(8);
}

void pic_maskinterrupt(uint8_t irq)
{
  uint16_t Port;
  uint8_t Value;

  /* Which Pic ? */
  if(irq < 8)
    Port = PIC0_INT_MASK_REGISTER;
  else
    {
      Port = PIC1_INT_MASK_REGISTER;
      irq -= 8;
    }

  /* Get current int status and set irq to disabled */
  Value = _inb(Port) | (1 << irq);

  /* Send new status */
  _outb(Port, Value);
}

void pic_unmaskinterrupt(uint8_t irq)
{
  uint16_t Port;
  uint8_t Value;

  /* Which Pic ? */
  if(irq < 8)
    Port = PIC0_INT_MASK_REGISTER;
  else
    {
      Port = PIC1_INT_MASK_REGISTER;
      irq -= 8;
    }

  /* Get current int status and set irq to enabled */
  Value = _inb(Port) & ~(1 << irq);

  /* Send new status */
  _outb(Port, Value);
}

void pic_eoi(uint8_t irq)
{
  /* Acknowledge IRQ */
  if(irq >= 8)
    _outb(PIC1_COMMAND_REGISTER, 0x20);

  _outb(PIC0_COMMAND_REGISTER, 0x20);
}
