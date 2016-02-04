#include "lib.h"

#define PROMPT "KUshell# "
#define NOTFOUNDERR ": command not found\n"
#define ELEMS(x)  (sizeof(x) / sizeof(x[0]))

typedef int (*cmd_fun_t)(int argc, char** argv);

typedef struct command {
  char* cmd_name;
  cmd_fun_t cmd_fun;
} command_t;

int cmd_echo(int argc, char** argv)
{
  if (argc == 2) {
    puts(argv[1]);
    putc('\n');
    return 0;
  }
  else {
    return -1;
  }
}

static command_t commands[] = {
  { "echo", cmd_echo },
  { "ls", cmd_echo },
  { "run", cmd_echo },
  { "bgrun", cmd_echo }
//  { "cd", cmd_echo }
};

char **tokenize(char* buf)
{
  int i; /* string index */
  int j;
  int n = 0; /* number of tokens */

  char** tok = NULL;

  for(i = 0; buf[i] != '\0'; i++) {

      /* replace spaces with nuls */
      for(j = i; buf[j] == ' '; j++)
        buf[j] = '\0';

      /* alloc char* for next token unless
         handling trailing whitespace */
      if ((j == 0 || j > i) && buf[j] != '\0') {
        n++;
        tok = realloc(tok, n*(sizeof(char*)));
        tok[n-1] = &buf[j];
        i = j;
      }
  }

  tok = realloc(tok, (n+1)*(sizeof(char*)));
  tok[n] = NULL;

  return tok;
}

int main()
{
  heap_init();
  char* buf;
  char** tokenized;

  buf = readline(PROMPT);
  tokenized = tokenize(buf);

  int i;
  for(i = 0; tokenized[i] != NULL; i++) {
    puts(tokenized[i]);
    puts("\n");
  }


  tokenized = tokenized;
  commands[0] = commands[0];

  syscall_halt();
  return 0;
}
