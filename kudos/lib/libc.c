/*
 * Generic library routines for Kudos
 */

#include "lib/libc.h"
#include "drivers/polltty.h"
#include "kernel/interrupt.h"
#include "vm/memory.h"

/* Declarations for assembler subroutines */
extern void _hwstop(void);
extern void _shutdown(void);


/*** @name Library routines
 *
 * This module implements various library routines.
 *
 * @{
 */

/***
 * Writes a null terminated string to the polling TTY. This routine is
 * used for kernel messages. User program prints should use the
 * interrupt driven TTY drivers.
 *
 * @param s The null-terminated string to be printed.
 */
void kwrite(char *s)
{
  while(*s) {
    int Character = (int)*s;
    polltty_putchar(Character);
    s++;
  }
}

/***
 * Reads at most \texttt{len - 1} characters from the polling TTY
 * driver to the buffer \texttt{s}. Reading is line-oriented so
 * characters are read until a newline character is encountered (or
 * \texttt{len} characters has been read). The resulting string will
 * be null-terminated.
 *
 * @param s The buffer which will contain the read line.
 *
 * @param len The length of the buffer \texttt{s}.
 *
 */
void kread(char *s,
           int len)
{
  int ch;
  int count = 0;

  while (((ch = polltty_getchar()) != '\n') &&
         (count < len - 1)){
    s[count] = ch;
    count++;
  }
  s[count] = '\0';
}

/**
 * Compares two strings. The strings must be null-terminated. If the
 * strings are equal returns 0. If the first string is greater than
 * the second one, returns a positive value. If the first string is
 * less than the second one, returns a negative value. This function
 * works like the strncpy function in the C library.
 *
 * @param str1 First string to compare.
 *
 * @param str2 The second string to compare.
 *
 * @return The difference of the first pair of bytes in str1 and str2
 * that differ. If the strings are equal returns 0.
 */
int stringcmp(const char *str1, const char *str2)
{
  while(1) {
    if (*str1 == '\0' && *str2 == '\0')
      return 0;
    if (*str1 == '\0' || *str2 == '\0' ||
        *str1 != *str2)
      return *str1-*str2;

    str1++;
    str2++;
  }

  /* Dummy return to keep gcc happy */
  return 0;
}

/**
 * Copies a string from source to target. The target buffer should be
 * at least buflen long. At most buflen-1 characters are copied. The
 * copied string will be null-terminated. If the source string is
 * shorter than buflen-1 characters, the end of the target string will
 * be padded with nulls. This function is almost like the strcpy
 * function in C library. The only difference occurs when the source
 * string is longer than the target buffer. The C library function
 * does not return a null-terminated string in this case while this
 * function does.
 *
 * @param buflen The length of the target buffer.
 *
 * @param target The target buffer of the copy operation.
 *
 * @param source The string to be copied.
 *
 */
char *stringcopy(char *target, const char *source, int buflen)
{
  int i;
  char *ret;

  ret = target;

  for(i = 0; i < buflen - 1; i++) {
    *target = *source;
    if (*source == '\0') {
      i++;
      while(i < buflen) {
        *target = '\0';
        i++;
      }
      return ret;
    }
    target++;
    source++;
  }
  *target = '\0';

  return ret;
}


/**
 * Copies memory buffer of size buflen from source to target. The
 * target buffer should be at least buflen long.
 *
 * @param buflen The number of bytes to be copied.
 *
 * @param target The target buffer of the copy operation.
 *
 * @param source The source buffer to be copied.
 *
 */
void memcopy(int buflen, void *target, const void *source)
{
  int i;
  char *t;
  const char *s;
  uint32_t *tgt;
  const uint32_t *src;

  tgt = (uint32_t *) target;
  src = (uint32_t *) source;

  if(((uintptr_t)tgt % 4) != 0 || ((uintptr_t)src % 4) != 0 )
    {
      t = (char *)tgt;
      s = (const char *)src;

      for(i = 0; i < buflen; i++) {
        t[i] = s[i];
      }

      return;
    }

  for(i = 0; i < (buflen/4); i++) {
    *tgt = *src;
    tgt++;
    src++;
  }


  t = (char *)tgt;
  s = (const char *)src;

  for(i = 0; i < (buflen%4); i++) {
    t[i] = s[i];
  }

  return;
}


/**
 * Sets size bytes in target to value.
 *
 * @param target The target buffer of the set operation.
 *
 * @param value What the bytes should be set to.
 *
 * @param size How many bytes to set.
 *
 */
void memoryset(void *target, char value, int size)
{
  int i;
  char *tgt;

  tgt = (char *)target;
  for(i = 0; i < size; i++)
    tgt[i] = value;
}

/** Converts the initial portion of a string to an integer
 * (e.g. "-23av34" converts to -23 and "a123" to 0). Works just like
 * atoi() in the C library. Errors are ignored, so too long numbers
 * (e.g. 12345678901234567890) will give "weird" results because of an
 * overflow occurs.
 *
 * @param s The string to convert
 * @return The converted numeric value
 */
int atoi(const char *s)
{
  int sign = 1;
  int i = 0;
  int value = 0;

  /* skip leading whitespace*/
  while(s[i] != 0 &&
        (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
    i++;

  /* check possible sign */
  switch(s[i]) {
  case 0:
    return 0; /* only whitespace in s */
  case '-':
    sign = -1;
    /* and fall through */
  case '+':
    i++;
  }

  /* read chars until a nondigit (or end of string) is met */
  while(s[i] >= '0' && s[i] <= '9') {
    value = 10*value + sign*(int)(s[i]-'0');
    i++;
  }

  return value;
}

int strlen(const char *str)
{
  int l=0;

  while(*(str++) != 0) l++;

  return l;
}

uint16_t swap16(uint16_t in)
{
  return ((in & 0xFF) << 8) | ((in & 0xFF00) >> 8);
}

uint32_t swap32(uint32_t in)
{
  uint32_t out;
  char *p_in = (char *) &in;
  char *p_out = (char *) &out;
  p_out[0] = p_in[3];
  p_out[1] = p_in[2];
  p_out[2] = p_in[1];
  p_out[3] = p_in[0];

  return out;
}

/* Byte order swap */
#ifdef SMALL_ENDIAN
uint16_t from_big_endian16(uint16_t in) {
  return swap16(in);
}
uint32_t from_big_endian32(uint32_t in) {
  return swap32(in);
}
uint16_t to_big_endian16(uint16_t in) {
  return swap32(in);
}
uint32_t to_big_endian32(uint32_t in) {
  return swap32(in);
}
#else
uint16_t from_big_endian16(uint16_t in) {
  return in;
}
uint32_t from_big_endian32(uint32_t in) {
  return in;
}
uint16_t to_big_endian16(uint16_t in) {
  return in;
}
uint32_t to_big_endian32(uint32_t in) {
  return in;
}
#endif

uint32_t wordpad(uint32_t in)
{
  if (in % 4 != 0) {
    return in + (4 - in % 4);
  } else {
    return in;
  }
}

/*** @} */
