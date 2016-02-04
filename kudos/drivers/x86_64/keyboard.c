/*
 * x86 Keyboard Driver
 */
#include <arch.h>
#include <asm.h>
#include <pit.h>
#include <keyboard.h>
#include "kernel/interrupt.h"
#include "lib/libc.h"

/* Extern */
extern void keyboard_irq_handler(void);
extern void _keyboard_wait(void);

/* Variables */
volatile uint8_t kb_shift = 0, kb_alt = 0, kb_ctrl = 0;
volatile uint8_t kb_numlock = 0, kb_capslock = 0, kb_scrolllock = 0;
volatile int kb_code;

/* XT Conversion Map */
static const uint8_t kb_map[] =
  {
    '\0','\0','1' ,'2',
    '3' ,'4' ,'5' ,'6' ,
    '7' ,'8' ,'9' ,'0' ,
    '-' ,'=' ,'\b','\t',
    'q' ,'w' ,'e' ,'r' ,
    't' ,'y' ,'u' ,'i' ,
    'o' ,'p' ,'[' ,']' ,
    '\n','\0','a' ,'s' ,
    'd' ,'f' ,'g' ,'h' ,
    'j' ,'k' ,'l' ,';' ,
    '\'','`' ,'\0','\\',
    'z' ,'x' ,'c' ,'v' ,
    'b' ,'n' ,'m' ,',' ,
    '.' ,'/' ,'\0','*' ,
    '\0',' ' ,'\0','\0',
    '\0','\0','\0','\0',
    '\0','\0','\0','\0',
    '\0','\0','\0','7' ,
    '8' ,'9' ,'-' ,'4' ,
    '5' ,'6' ,'+' ,'1' ,
    '2' ,'3' ,'0' ,'.'
  };

static const uint8_t kb_map_shift[] = {
  '\0','\0','!' ,'@' ,
  '#' ,'$' ,'%' ,'^' ,
  '&' ,'*' ,'(' ,')' ,
  '_' ,'+' ,'\b','\t',
  'Q' ,'W' ,'E' ,'R' ,
  'T' ,'Y' ,'U' ,'I' ,
  'O' ,'P' ,'{' ,'}' ,
  '\n','\0','A' ,'S' ,
  'D' ,'F' ,'G' ,'H' ,
  'J' ,'K' ,'L' ,':' ,
  '\"','~' ,'\0','|' ,
  'Z' ,'X' ,'C' ,'V' ,
  'b' ,'N' ,'M' ,'<' ,
  '>' ,'?' ,'\0','*' ,
  '\0',' ' ,'\0','\0',
  '\0','\0','\0','\0',
  '\0','\0','\0','\0',
  '\0','\0','\0','7' ,
  '8' ,'9' ,'-' ,'4' ,
  '5' ,'6' ,'+' ,'1' ,
  '2' ,'3' ,'0' ,'.'
};

/* Helpers */
uint8_t keyboard_getstatus()
{
  return _inb(PS2_KEYBOARD_STATUS);
}

uint8_t keyboard_test()
{
  if(keyboard_getstatus() & PS2_KB_STATUS_OUTPUT_BUSY)
    {
      _inb(PS2_KEYBOARD_DATA);
      return 1;
    }

  return 0;
}

uint8_t keyboard_sendencoder(uint8_t command)
{
  /* We need input buffer to be ready */
  while(keyboard_getstatus() & PS2_KB_STATUS_INPUT_BUSY)
    _keyboard_wait();

  /* Ready, send command */
  _outb(PS2_KEYBOARD_DATA, command);

  /* Wait for response */
  while(!(keyboard_getstatus() & PS2_KB_STATUS_OUTPUT_BUSY))
    _keyboard_wait();

  return _inb(PS2_KEYBOARD_DATA);
}

/* Reads the actual data */
uint8_t keyboard_readdata()
{
  /* Wait for data */
  while(!(keyboard_getstatus() & PS2_KB_STATUS_OUTPUT_BUSY))
    _keyboard_wait();

  return _inb(PS2_KEYBOARD_DATA);
}

/* Set LED state on keyboard */
void keyboard_updateLED()
{
  /* Fill this */
  uint8_t led_data = 0;

  led_data = (kb_scrolllock == 1) ? (led_data | 0x1) : (led_data & 0x1);
  led_data = (kb_numlock == 1) ? (led_data | 0x2) : (led_data & 0x2);
  led_data = (kb_capslock == 1) ? (led_data | 0x4) : (led_data & 0x4);

  /* Write it */
  keyboard_sendencoder(PS2_KB_SETLEDS);
  keyboard_sendencoder(led_data);
}

/* The "C" Irq */
void keyboard_key_int(void)
{
  /* decls */
  int kcode = 0;
  uint8_t extended = 0;
  uint8_t xt = 0;

  /* Wait for output buffer to be full */
  if(keyboard_getstatus() & PS2_KB_STATUS_OUTPUT_BUSY)
    {
      /* Get the scancode */
      kcode = keyboard_readdata();

      /* Is it an extended scancode ? */
      extended = (kcode == 0xE0 || kcode == 0xE1) ? 1 : 0;

      if(extended == 0)
        {
          /* Is it pressed? */
          if(kcode & 0x80)
            {
              kcode -= 0x80;
              xt = 1;
            }
          else
            {
              /* Or released? */
              xt = 0;
            }

          /* Test if it is an original XT scan code */
          switch(kcode)
            {
              /* Right shift & left shift */
            case 0x2a:
            case 0x36:
              kb_shift = (kb_shift == 0) ? 1 : 0;
              kcode = 0;
              break;

              /* Right ctrl & left ctrl */
            case 0x1d:
              kb_ctrl = (kb_ctrl == 0) ? 1 : 0;
              kcode = 0;
              break;

              /* Caps lock */
            case 0x3a:
              kb_capslock = !!xt;
              kcode = 0;
              break;

              /* Num lock */
            case 0x45:
              kb_numlock = !!xt;
              kcode = 0;
              break;

              /* Scroll lock */
            case 0x46:
              kb_scrolllock = !!xt;
              kcode = 0;
              break;

              /* ASCIIIIII */
            default:
              {
                /* Set key on pressed, not released */
                if(!xt)
                  {
                    /* Detect modifiers */
                    if(!kb_shift)
                      kb_code = kb_map[kcode];
                    else
                      kb_code = kb_map_shift[kcode];
                  }
              }
            }
        }
    }
}

void keyboard_discardkey()
{
  kb_code = INVALID_SCANCODE;
}

/* Initializor */
void keyboard_init()
{
  /* Test if keyboard exists */
  while(keyboard_test())
    _keyboard_wait();

  /* Send a reset & disable scan command */
  if(keyboard_sendencoder(PS2_KB_RESETWAIT) != PS2_KB_ACK)
    return;

  /* Send an identify command */
  if(keyboard_sendencoder(PS2_KB_IDENTIFY) != PS2_KB_ACK)
    return;

  if(keyboard_readdata() == PS2_KB_INTERFACETEST)
    keyboard_readdata();
  else
    return;

  /* Now we know that a PS/2 Keyboard exists, enable it */

  /* Send a reset & enable scan command */
  if(keyboard_sendencoder(PS2_KB_ENABLESCAN) != PS2_KB_ACK)
    return;

  /* Setup leds */
  keyboard_updateLED();

  /* Reset flags */
  kb_code = INVALID_SCANCODE;
  kb_shift = 0;
  kb_ctrl = 0;

  /* Install IRQ */
  interrupt_register(1, (int_handler_t)keyboard_irq_handler, 0);
}

int keyboard_getlastkey()
{
  return kb_code;
}

char keyboard_getkey()
{
  int key = INVALID_SCANCODE;

  /* Get key */
  while(key == INVALID_SCANCODE)
    key = keyboard_getlastkey();

  /* Discard it, we've got it */
  keyboard_discardkey();

  return (char)key;
}
