/*
 * Userland library
 */

/* You probably want to add new functions to this file. To maintain
 * binary compatibility (as probably required by your assignments) DO
 * NOT CHANGE EXISTING SYSCALL FUNCTIONS!
 */

#include "../kudos/proc/syscall.h"
#include "lib.h"
#include <stdarg.h> /* For va_args stuff - uses the host (Linux)
                     * definitions. */

/* Extern */
//extern uintptr_t _syscall(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

/* Halt the system (sync disks and power off). This function will
 * never return.
 */
void syscall_halt(void)
{
  _syscall(SYSCALL_HALT, 0, 0, 0);
}


/* Load the file indicated by 'filename' as a new process and execute
 * it, passing the given argv. Returns the process ID of the created
 * process. Negative values are errors.
 */
int syscall_spawn(const char *filename, const char **argv)
{
  return (int)_syscall(SYSCALL_SPAWN, (uintptr_t)filename,
                       (uintptr_t)argv, 0);
}


/* Exit the current process with exit code 'retval'. Note that
 * 'retval' must be non-negative since syscall_join's negative return
 * values are interpreted as errors in the join call itself. This
 * function will never return.
 */
void syscall_exit(int retval)
{
  _syscall(SYSCALL_EXIT, (uintptr_t)retval, 0, 0);
}


/* Wait until the execution of the process identified by 'pid' is
 * finished. Returns the exit code of the joined process, or a
 * negative value on error.
 */
int syscall_join(int pid)
{
  return (int)_syscall(SYSCALL_JOIN, (uintptr_t)pid, 0, 0);
}


/* Create a new thread running in the same address space as the
 * caller. The thread is started at function 'func', and the thread
 * will end when 'func' returns. 'arg' is passed as an argument to
 * 'func'. Returns 0 on success or a negative value on error.
 */
int syscall_fork(void (*func)(int), int arg)
{
  return (int)_syscall(SYSCALL_FORK, (uintptr_t)func, (uintptr_t)arg, 0);
}


/* (De)allocate memory by trying to set the heap to end at the address
 * 'heap_end'. Returns the new end address of the heap, or NULL on
 * error. If 'heap_end' is NULL, the current heap end is returned.
 */
void *syscall_memlimit(void *heap_end)
{
  return (void*)(uintptr_t)_syscall(SYSCALL_MEMLIMIT, (uintptr_t)heap_end, 0, 0);
}


/* Open the file identified by 'filename' for reading and
 * writing. Returns the file handle of the opened file (positive
 * value), or a negative value on error.
 */
int syscall_open(const char *filename)
{
  return (int)_syscall(SYSCALL_OPEN, (uintptr_t)filename, 0, 0);
}


/* Close the open file identified by 'filehandle'. Zero will be returned
 * success, other values indicate errors.
 */
int syscall_close(int filehandle)
{
  return (int)_syscall(SYSCALL_CLOSE, (uintptr_t)filehandle, 0, 0);
}


/* Read 'length' bytes from the open file identified by 'filehandle'
 * into 'buffer', starting at the current file position. Returns the
 * number of bytes actually read (e.g. 0 if the file position is at
 * the end of file) or a negative value on error.
 */
int syscall_read(int filehandle, void *buffer, int length)
{
  return (int)_syscall(SYSCALL_READ, (uintptr_t)filehandle,
                       (uintptr_t)buffer, (uintptr_t)length);
}


/* Set the file position of the open file identified by 'filehandle'
 * to 'offset'. Returns 0 on success or a negative value on error.
 */
int syscall_seek(int filehandle, int offset)
{
  return (int)_syscall(SYSCALL_SEEK,
                       (uintptr_t)filehandle, (uintptr_t)offset, 0);
}


/* Write 'length' bytes from 'buffer' to the open file identified by
 * 'filehandle', starting from the current file position. Returns the
 * number of bytes actually written or a negative value on error.
 */
int syscall_write(int filehandle, const void *buffer, int length)
{
  return (int)_syscall(SYSCALL_WRITE, (uintptr_t)filehandle, (uintptr_t)buffer,
                       (uintptr_t)length);
}


/* Create a file with the name 'filename' and initial size of
 * 'size'. Returns 0 on success and a negative value on error.
 */
int syscall_create(const char *filename, int size)
{
  return (int)_syscall(SYSCALL_CREATE, (uintptr_t)filename, (uintptr_t)size, 0);
}


/* Remove the file identified by 'filename' from the file system it
 * resides on. Returns 0 on success or a negative value on error.
 */
int syscall_delete(const char *filename)
{
  return (int)_syscall(SYSCALL_DELETE, (uintptr_t)filename, 0, 0);
}

/* Count the amount of files in the given directory. */
int syscall_filecount(const char *pathname)
{
  return (int)_syscall(SYSCALL_FILECOUNT, (uintptr_t)pathname, 0, 0);
}

/* Get the name of the idx'th file in the given directory. */
int syscall_file(const char *pathname, int idx, char *buffer)
{
  return (int)_syscall(SYSCALL_FILE, (uintptr_t)pathname,
                       (uintptr_t)idx, (uintptr_t)buffer);
}

/* The following functions are not system calls, but convenient
   library functions inspired by POSIX and the C standard library. */

#ifdef PROVIDE_STRING_FUNCTIONS

/* Return the length of the string pointed to by s. */
size_t strlen(const char *s)
{
  size_t i;
  for (i=0; s[i]; i++);
  return i;
}

/* Copy all of src to after dest and return dest.  Make sure there is
   enough room before calling this function. */
char *strcpy(char *dest, const char *src)
{
  size_t i;
  for (i = 0; src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  // reached end, now src[i] == '\0'
  dest[i]='\0';
  return dest;
}

/* Copy as much of src as possible to after dest.  At most n
   characters from src will be copied. If there is no null byte among
   the first n bytes of src, the string placed in dest will not be
   null-terminated. */
char *strncpy(char *dest, const char *src, size_t n)
{
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  if (i < n) {
    dest[i] = '\0';
  }
  return dest;
}

/* Copy all of src to after dest and return dest.  Make sure there is
   enough room before calling this function. */
char *strcat(char *dest, const char *src)
{
  return strcpy(dest+strlen(dest), src);
}

char *strncat(char *dest, const char *src, size_t n)
{
  size_t dest_len = strlen(dest);
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[dest_len + i] = src[i];
  }
  dest[dest_len + i] = '\0';

  return dest;
}

int strcmp(const char *s1, const char *s2)
{
  return strncmp(s1, s2, 0x7fffffff);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  int i;
  for (i = 0; (s1[i] || s2[i]) && n > 0 ; i++, n--) {
    if (s1[i] < s2[i]) {
      return -1;
    } else if (s1[i] > s2[i]) {
      return 1;
    }
  }
  return 0;
}

int memcmp(const void* s1, const void* s2,size_t n)
{
  const unsigned char *p1 = s1, *p2 = s2;
  while(n--) {
    if( *p1 != *p2 ) {
      return *p1 - *p2;
    } else {
      p1++;
      p2++;
    }
  }
  return 0;
}

char *strstr(const char *s1, const char *s2)
{
  size_t n = strlen(s2);
  while(*s1)
    if(!memcmp(s1++,s2,n))
      return (char*)s1-1;
  return NULL;
}

void *memset(void *s, int c, size_t n) {
  byte *p = s;
  while (n-- > 0) {
    *(p++) = c;
  }
  return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
  byte *d = dest;
  const byte *s = src;
  while (n-- > 0) {
    *(d++) = *(s++);
  }
  return dest;
}

#endif

#ifdef PROVIDE_BASIC_IO

/* Write c to standard output.  Returns a non-negative integer on
   success. */
int putc(char c)
{
  return syscall_write(FILEHANDLE_STDOUT, &c, 1);
}

/* Write the string pointed to by s to standard output.  Returns a
   non-negative integer on success. */
int puts(const char* s)
{
  return syscall_write(FILEHANDLE_STDOUT, s, strlen(s));
}

/* Read character from standard input, without echoing.  Returns a
   non-negative integer on success, which can be casted to char. */
int getc_raw(void)
{
  char c;
  syscall_read(FILEHANDLE_STDIN, &c, 1);
  return c;
}

/* Read character from standard input, with echoing.  Returns a
   non-negative integer on success, which can be casted to char. */
int getc(void)
{
  char c = getc_raw();
  syscall_write(FILEHANDLE_STDOUT, &c, 1); /* Echo back at user. */
  return c;
}

/* Read up to size characters from standard input into the buffer s,
   with echoing.  Returns the number of characters read. */
ssize_t gets(char *s, size_t size)
{
  size_t count;
  for (count = 0; count+1 < size; s[count++] = getc());
  s[count+1] = '\0';
  return count;
}

/* Read up from standard input up to the first newline (\n) character,
   and at most size-1 characters, into the buffer s, with echoing and
   support for backspace.  Returns the number of characters read.  You
   can check whether a full line was read by seeing whether a newline
   precedes the terminating \0. */
ssize_t readline_static(char *s, size_t size)
{
  size_t count = 0;
  while (1) {
    int c = getc_raw();
    switch (c) {
    case '\r': /* Treat as newline */
    case '\n':
      putc('\n');
      goto stop;
      break;
    case 127:
      if (count > 0) {
        putc('\010');
        putc(' ');
        putc('\010');
        count--;
      }
      break;
    default:
      if (count<size-1) {
        putc(s[count++]=c);
      }
    }
  }
 stop:
  s[count] = '\0';
  return count;
}

/* Prompt for input, and read from standard input up to the first newline (\n)
   character, with support for backspace, into dynamically reallocated buffer
   buf of initial size BUFSIZE. Returns buf */
char *readline(char *prompt)
{
  int c;
  char *buf;
  char *newbuf;
  size_t bufsize = BUFSIZE;
  size_t count = 0;

  buf = malloc(bufsize*sizeof(char));
  puts(prompt);

  while (1) {
    c = getc_raw();
    switch (c) {
    case '\r': /* Treat as newline */
    case '\n':
      putc('\n');
      goto stop;
      break;
    case 127: /* handle DELelte */
      if (count > 0) {
        putc('\010');
        putc(' ');
        putc('\010');
        count--;
      }
      break;
    default:
      if (count >= bufsize-1) {
        bufsize *= 2;
        newbuf = realloc(buf, bufsize*sizeof(char));

        if (newbuf == NULL)
          goto stop;
        else
          buf = newbuf;
      }
      buf[count] = c;
      putc(c);
      count++;
    }
  }
 stop:
  buf[count] = '\0';

  newbuf = realloc(buf, (count+1)*sizeof(char));
  if (newbuf == NULL)
    newbuf = buf;

  return newbuf;
}



#endif

/* Formatted printing, from the lib/ directory of Buenos. */
#ifdef PROVIDE_FORMATTED_OUTPUT

#define FLAG_TTY     0x8000
#define FLAG_SMALLS  0x01
#define FLAG_ALT     0x02
#define FLAG_ZEROPAD 0x04
#define FLAG_LEFT    0x08
#define FLAG_SPACE   0x10
#define FLAG_SIGN    0x20


/* Output the given char either to the string or to the TTY. */
static void printc(char *buf, char c, int flags) {
  if (flags & FLAG_TTY) {
    /* do not output (terminating) zeros to TTY */
    if (c != '\0') putc(c);
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
          unsigned int n,
          unsigned int base,
          int flags,
          int prec,
          int width)
{
  static const char digits[32] = "0123456789ABCDEF0123456789abcdef";
  char rev[11]; /* space for 32-bit int in octal */
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
  prec = MIN(prec, 11);
  width = MIN(width, 11);

  /* zero pad until at least 'prec' digits written */
  while (i < prec) {
    rev[i] = '0';
    i++;
  }

  /* pad with spaces until at least 'width' chars written */
  while (i < width) {
    rev[i] = ' ';
    i++;
  }

  /* output the produced string in reverse order */
  i--;
  while (i >= 0 && written < size) {
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

  while (*s > '0' && *s < '9') {
    value = 10*value + (int)(*s - '0');
    s++;
  }

  if (next != NULL) *next = s;
  return value;
}

static int vxnprintf(char *buf,
         int size,
         const char *fmt,
         va_list ap,
         int flags)
{
  int written = 0, w, moremods;
  int width, prec;
  char ch, *s;
  unsigned int uarg;
  int arg;

  if (size <= 0) return 0;

  while (written < size) {
    ch = *fmt++;
    if (ch == '\0') break;

    /* normal character => just output it */
    if (ch != '%') {
      printc(buf++, ch, flags);
      written++;
      continue;
    }

    /* to get here, ch == '%' */
    ch = *fmt++;
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
    } while (moremods && ch != '\0');


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
      while (written < w && *s != '\0') {
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

int printf(const char *fmt, ...) {
  va_list ap;
  int written;

  va_start(ap, fmt);
  written = vxnprintf((char*)0, 0x7fffffff, fmt, ap, FLAG_TTY);
  va_end(ap);

  return written;
}

int snprintf(char *str, int size, const  char  *fmt, ...) {
  va_list ap;
  int written;

  va_start(ap, fmt);
  written = vxnprintf(str, size, fmt, ap, 0);
  va_end(ap);
  return written;
}

#endif

/* Heap allocation. */
#ifdef PROVIDE_HEAP_ALLOCATOR

typedef struct free_block {
  size_t size;
  struct free_block *next;
} free_block_t;

static const size_t MIN_ALLOC_SIZE = sizeof(free_block_t);

free_block_t *free_list;

byte heap[HEAP_SIZE];

/* Initialise the heap - malloc et al won't work unless this is called
   first. */
void heap_init()
{
  free_list = (free_block_t*) heap;
  free_list->size = HEAP_SIZE;
  free_list->next = NULL;
}


/* Return a block of at least size bytes, or NULL if no such block
   can be found.  */
void *malloc(size_t size) {
  free_block_t *block;
  free_block_t **prev_p; /* Previous link so we can remove an element */
  if (size == 0) {
    return NULL;
  }

  /* Ensure block is big enough for bookkeeping. */
  size=MAX(MIN_ALLOC_SIZE,size);
  /* Word-align */
  if (size % 4 != 0) {
    size &= ~3;
    size += 4;
  }

  /* Iterate through list of free blocks, using the first that is
     big enough for the request. */
  for (block = free_list, prev_p = &free_list;
       block;
       prev_p = &(block->next), block = block->next) {
    if ( (int)( block->size - size - sizeof(size_t) ) >=
         (int)( MIN_ALLOC_SIZE+sizeof(size_t) ) ) {
      /* Block is too big, but can be split. */
      block->size -= size+sizeof(size_t);
      free_block_t *new_block =
        (free_block_t*)(((byte*)block)+block->size);
      new_block->size = size+sizeof(size_t);
      return ((byte*)new_block)+sizeof(size_t);
    } else if (block->size >= size + sizeof(size_t)) {
      /* Block is big enough, but not so big that we can split
         it, so just return it */
      *prev_p = block->next;
      return ((byte*)block)+sizeof(size_t);
    }
    /* Else, check the next block. */
  }

  /* No heap space left. */
  return NULL;
}

/* Return the block pointed to by ptr to the free pool. */
void free(void *ptr)
{
  if (ptr != NULL) { /* Freeing NULL is a no-op */
    free_block_t *block = (free_block_t*)((byte*)ptr-sizeof(size_t));
    free_block_t *cur_block;
    free_block_t *prev_block;

    /* Iterate through the free list, which is sorted by
       increasing address, and insert the newly freed block at the
       proper position. */
    for (cur_block = free_list, prev_block = NULL;
         ;
         prev_block = cur_block, cur_block = cur_block->next) {
      if (cur_block > block || cur_block == NULL) {
        /* Insert block here. */
        if (prev_block == NULL) {
          free_list = block;
        } else {
          prev_block->next = block;
        }
        block->next = cur_block;

        if (prev_block != NULL &&
            (size_t)((byte*)block - (byte*)prev_block) == prev_block->size) {
          /* Merge with previous. */
          prev_block->size += block->size;
          prev_block->next = cur_block;
          block = prev_block;
        }

        if (cur_block != NULL &&
            (size_t)((byte*)cur_block - (byte*)block) == block->size) {
          /* Merge with next. */
          block->size += cur_block->size;
          block->next = cur_block->next;
        }
        return;
      }
    }
  }
}

void *calloc(size_t nmemb, size_t size)
{
  size_t i;
  byte *ptr = malloc(nmemb*size);
  if (ptr != NULL) {
    for (i = 0; i < nmemb*size; i++) {
      ptr[i] = 0;
    }
  }
  return ptr;
}

void *realloc(void *ptr, size_t size)
{
  byte *new_ptr;
  size_t i;
  if (ptr == NULL) {
    return malloc(size);
  }

  if (ptr != NULL && size == 0) {
    free(ptr);
    return NULL;
  }

  /* Simple implementation: allocate new space and copy the contents
     over.  Exercise: Improve this by searching through the free
     list and seeing whether an actual enlargement is possible. */
  new_ptr = malloc(size);
  if (new_ptr != NULL) {
    for (i = 0; i < size; i++) {
      new_ptr[i] = ((byte*)ptr)[i];
    }
    free(ptr);
  }
  return new_ptr;
}

#endif

#ifdef PROVIDE_MISC

int atoi(const char *nptr)
{
  int i;
  int retval = 0;
  int n = strlen(nptr);
  for (i = 0; i < n; i++) {
    if (nptr[i] < '0' || nptr[i] > '9') {
      break;
    }
    retval = retval * 10 + (nptr[i] - '0');
  }
  return retval;
}

#endif
