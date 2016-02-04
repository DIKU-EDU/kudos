/** @name Formatted output functions
 *
 * The following formatted output functions are implemented here:
 * kprintf, kvprintf, snprintf, vsnprintf. These are scaled down
 * versions of those documented in the manual page printf(3), with
 * kprintf and kvprintf printf and vprintf which use the polling TTY
 * driver for output.
 *
 * The only data types supported are integer (32-bit), char and a
 * pointer (type identifiers d, i, o, u, x, X, c, s, p, no length
 * specifiers (e.g. l) are supported.
 *
 * The formatting _cannot_ refer to the parameter by number
 * (i.e. the *-format).
 *
 * The following restrictions apply to the modifiers:
 *
 * \begin{tabular}{l|l}
 * \hline
 *
 *  # (alternate form) & Supported for x,X         \\
 *  0 (zero padding)   & Supported for o,u,x,X,p   \\
 *  - (left adjust)    & Not supported             \\
 *  space              & Supported (d,i)           \\
 *  +                  & Supported (d,i)           \\
 *  Field width        & Supported for o,u,x,X,p   \\
 *  Precision          & Supported for o,u,x,X,s,p \\
 *
 * \end{tabular}
 */


#include "lib/libc.h"
#include "drivers/polltty.h"
#include "kernel/interrupt.h"
#include "kernel/spinlock.h"

/* determine whether to output to TTY or a memory buffer */
#define FLAG_TTY     0x8000

static int vxnprintf(char*, int, const char*, va_list, int);


spinlock_t kprintf_slock = 0;

/* corresponding to vprintf(3) */
int kvprintf(const char *fmt, va_list ap) {
  int written;
  interrupt_status_t intr_state;

  intr_state = _interrupt_disable();
  spinlock_acquire(&kprintf_slock);

  written = vxnprintf((char*)0, 0x7fffffff, fmt, ap, FLAG_TTY);

  spinlock_release(&kprintf_slock);
  _interrupt_set_state(intr_state);

  return written;
}

/* corresponding to printf(3) */
int kprintf(const char *fmt, ...) {
  va_list ap;
  int written;
  interrupt_status_t intr_state;

  intr_state = _interrupt_disable();
  spinlock_acquire(&kprintf_slock);

  va_start(ap, fmt);
  written = vxnprintf((char*)0, 0x7fffffff, fmt, ap, FLAG_TTY);
  va_end(ap);

  spinlock_release(&kprintf_slock);
  _interrupt_set_state(intr_state);

  return written;
}

/* (almost) vsnprintf(3) */
int vsnprintf(char *str, int size, const char  *fmt, va_list ap) {
  return vxnprintf(str, size, fmt, ap, 0);
}

/* (almost) snprintf(3) */
int snprintf(char *str, int size, const  char  *fmt, ...) {
  va_list ap;
  int written;

  va_start(ap, fmt);
  written = vxnprintf(str, size, fmt, ap, 0);
  va_end(ap);
  return written;
}










#define FLAG_SMALLS  0x01
#define FLAG_ALT     0x02
#define FLAG_ZEROPAD 0x04
#define FLAG_LEFT    0x08
#define FLAG_SPACE   0x10
#define FLAG_SIGN    0x20


/* output the given char either to the string or to the TTY */
static void printc(char *buf, char c, int flags) {
  if (flags & FLAG_TTY) {
    /* do not output (terminating) zeros to TTY */
    if (c != '\0') polltty_putchar(c); /* synched in k(v)printf */
  } else
    *buf = c;
}


/* Output 'n' in base 'base' into buffer 'buf' or to TTY.  At least
 * 'prec' numbers are output, padding with zeros if needed, and at
 * least 'width' characters are output, padding with spaces on the
 * left if needed. 'flags' tells whether to use the buffer or TTY for
 * output and whether to use capital digits.
 */
static int print_uint(char *buf,
                      int size,
                      uintptr_t n,
                      unsigned int base,
                      int flags,
                      int prec,
                      int width)
{
  static const char digits[32] = "0123456789ABCDEF0123456789abcdef";
  char rev[22]; /* space for 64-bit int in octal */
  int i = 0, written = 0;

  if (size <= 0) return 0;

  /* produce the number string in reverse order to the temp buffer 'rev' */
  do {
    if (flags & FLAG_SMALLS)
      rev[i] = digits[16 + n % base];
    else
      rev[i] = digits[n % base];
    i++;
    n /= base;
  } while (n != 0);

  /* limit precision and field with */
  prec = MIN(prec, 22);
  width = MIN(width, 22);

  /* zero pad until at least 'prec' digits written */
  while(i < prec) {
    rev[i] = '0';
    i++;
  }

  /* pad with spaces until at least 'width' chars written */
  while(i < width) {
    rev[i] = ' ';
    i++;
  }

  /* output the produced string in reverse order */
  i--;
  while(i >= 0 && written < size) {
    printc(buf++, rev[i], flags);
    written++;
    i--;
  }

  return written;
}


/* Scan a 10-base nonnegative integer from string 's'. The scanned
 * integer is returned, and '*next' is set to point to the string
 * immediately following the scanned integer.
 */
static int scan_int(const char *s, const char **next) {
  int value = 0;

  while(*s > '0' && *s < '9') {
    value = 10*value + (int)(*s - '0');
    s++;
  }

  if (next != NULL) *next = s;
  return value;
}


/* Output a formatted string 'fmt' with variable arguments 'ap' into
 * the string 'buf' or to TTY, depending on 'flags'. At most 'size'
 * characters are written (including the trailing '\0'). Returns the
 * number of characters actually written.
 */
static int vxnprintf(char *buf,
                     int size,
                     const char *fmt,
                     va_list ap,
                     int flags)
{
  int written = 0, w, moremods;
  int width, prec;
  char ch, *s;
  uintptr_t uarg;
  intptr_t arg;
  const char *p;

  if (size <= 0) return 0;

  while(written < size) {
    ch = *fmt++;
    p = fmt;
    if (ch == '\0') break;

    /* normal character => just output it */
    if (ch != '%') {
      printc(buf++, ch, flags);
      written++;
      continue;
    }

    /* to get here, ch == '%' */
    ch = *fmt++;
    p++;
    if (ch == '\0') break;

    flags &= FLAG_TTY; /*  preserve only the TTY flag */
    width = prec = -1;
    moremods = 1;

    /* read flags and modifiers (width+precision): */
    do {
      switch(ch) {
      case '#': /* alternative output */
        flags |= FLAG_ALT;
        break;

      case '0': /* zero padding */
        flags |= FLAG_ZEROPAD;
        break;

      case ' ': /* space in place of '-' */
        flags |= FLAG_SPACE;
        break;

      case '+': /* '+' in place of '-' */
        flags |= FLAG_SIGN;
        break;

      case '-': /* left align the field */
        flags |= FLAG_LEFT;
        break;

      case '.': /* value precision */
        prec = scan_int(fmt, &fmt);
        break;

      case '1': case '2': case '3': case '4': case '5':
      case '6': case '7': case '8': case '9': /* field width */
        width = scan_int(fmt-1, &fmt);
        break;

      default: /* no more modifiers to scan */
        moremods = 0;
      }

      if (moremods) ch = *fmt++;
    } while(moremods && ch != '\0');


    if (ch == '\0') break;

    /* read the type of the argument : */
    switch(ch) {
    case 'i': /* signed integer */
    case 'd':
      arg = va_arg(ap, int);

      if (arg < 0) { /* negative value, print '-' and negate */
        printc(buf++, '-', flags);
        written++;
        arg = -arg;
      } if (flags & FLAG_SIGN) { /* '+' in place of '-' */
        printc(buf++, '+', flags);
        written++;
      } else if (flags & FLAG_SPACE) { /* ' ' in place of '-' */
        printc(buf++, ' ', flags);
        written++;
      }

      w = print_uint(buf, size-written, arg, 10, flags, 0, 0);
      buf += w;
      written += w;
      break;

    case 'o': /* octal integer */
      if (prec < width && (flags & FLAG_ZEROPAD)) prec = width;
      uarg = va_arg(ap, unsigned int);
      w = print_uint(buf, size-written, uarg, 8, flags, prec, width);
      buf += w;
      written += w;
      break;

    case 'u': /* unsigned integer */
      if (prec < width && (flags & FLAG_ZEROPAD)) prec = width;

      if(*p == 'L' || *p == 'l')
        {
          /* skip L */
          fmt++;
          uarg = va_arg(ap, uint64_t);
        }
      else
        uarg = va_arg(ap, unsigned int);

      w = print_uint(buf, size-written, uarg, 10, flags, prec, width);
      buf += w;
      written += w;
      break;
    case 'p': /* memory pointer */
      flags |= FLAG_ALT;
    case 'x': /* hexadecimal integer, noncapitals */
      flags |= FLAG_SMALLS;
    case 'X': /* hexadecimal integer, capitals */

      if (flags & FLAG_ALT) { /* alt form begins with '0x' */
        printc(buf++, '0', flags);
        written++;
        if (written < size) {
          printc(buf++, 'x', flags);
          written++;
        }
        width -= 2;
      }
      if (prec < width && (flags & FLAG_ZEROPAD)) prec = width;

      if(*p == 'L' || *p == 'l')
        {
          /* Skip L */
          fmt++;
          uarg = va_arg(ap, uint64_t);
        }
      else
        uarg = va_arg(ap, unsigned int);

      w = print_uint(buf, size-written, uarg, 16, flags, prec, width);
      buf += w;
      written += w;
      break;

    case 'c': /* character */
      arg = va_arg(ap, int);
      printc(buf++, (char)arg, flags);
      written++;
      break;

    case 's': /* string */
      s = va_arg(ap, char*);
      w = size;
      if (prec != -1 && written+prec < size) w = written+prec;
      while(written < w && *s != '\0') {
        printc(buf++, *s++, flags);
        written++;
      }
      break;

    default: /* unknown type, just output */
      printc(buf++, ch, flags);
      written++;
    }
  }
  /* the string was truncated */
  if (written == size) {
    buf--;
    written = -1;
  }
  printc(buf, '\0', flags); /* terminating zero */

  return written;
}
