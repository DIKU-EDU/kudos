/*
 * Shell!  For running programs!
 *
 * Written by Troels Henriksen (athas@sigkill.dk) in March, 2012, for
 * the operating systems course (OSM) at DIKU.  Use this to test
 * various bits of the kernel.  Feel free to extend it if you want to.
 * Basic filename completion is supported!  Note that some commands
 * will cause a kernel panic or otherwise fail until the necessary
 * system calls have been implemented.
 */

#include "lib.h"

#define BUFFER_SIZE 100
#define PATH_LENGTH 256

#define stdin 1

int background_proc = -1;

int does_file_exist(char* file) {
  int fd = syscall_open(file);
  syscall_close(fd);
  return fd >= 0;
}

int getc_noecho(void)
{
  char c;
  syscall_read(stdin, &c, 1);
  return c;
}

void clearline() {
  int i;
  putc('\r');
  for (i = 0; i < 50; i++) {
    putc(' ');
  }
  putc('\r');
}

int cmd_ls(int argc, char** argv) {
  char d[PATH_LENGTH];
  char* dir;
  if (argc == 2) {
    strcpy(d, argv[1]);
    dir = (char*) d;
  } else {
    dir = (char*) NULL;
  }
  int i;
  int n = syscall_filecount(dir);
  char buffer[PATH_LENGTH];

  if (n < 0) {
    printf("Error reading %s (Reason: %d)\n", dir, n);
    return n;
  }

  for (i = 0; i < n; ++i)
    {
      if (syscall_file(dir, i, buffer)) {
        return 2;
      }
      printf("%s\n", buffer);
    }
  return 0;
}

int cmd_cp(int argc, char** argv) {
  if (argc != 3 && argc != 4) {
    printf("Usage: cp <from> <to> [size]\n");
    return 1;
  }
  int fd1, fd2;
  int size = argc == 4 ? atoi(argv[3]) : 0;
  if ((fd1=syscall_open(argv[1])) < 0) {
    printf("Could not open %s.  Reason: %d\n", argv[1], fd1);
    return 1;
  }
  if ((fd2=syscall_create(argv[2], size)) < 0) {
    printf("Could not create %s with initial size %d.  Reason: %d\n", argv[2], size, fd2);
    syscall_close(fd1);
    return 1;
  }
  if ((fd2=syscall_open(argv[2])) < 0) {
    printf("Could not open newly created file %s.  Reason: %d\n", argv[2], fd2);
    syscall_close(fd1);
    return 1;
  }
  int ret, i, rd, wr;
  int totalread = 0, totalwritten = 0;
  char buffer[BUFFER_SIZE];
  while ((rd = syscall_read(fd1, buffer, BUFFER_SIZE))) {
    i = 0;
    totalread += rd;
    clearline();
    printf("Read %d bytes, wrote %d bytes.", totalread, totalwritten);
    while (i < rd) {
      if ((wr=syscall_write(fd2, buffer+i, rd-i)) <= 0) {
        printf("\nCall to syscall_write() failed.  Reason: %d.\n", wr);
        if (wr == 0) {
          printf("Did you remember to make the destination file big enough?\n");
        }
        ret=1;
        goto exit;
      }
      totalwritten += wr;
      i += wr;
      clearline();
      printf("Read %d bytes, wrote %d bytes.", totalread, totalwritten);
    }
  }
 exit:
  printf("\n");
  syscall_close(fd1);
  syscall_close(fd2);
  return ret;
}

int cmd_touch(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: touch <file> [size]\n");
    return 1;
  }
  int ret;
  int size = atoi(argv[2]);
  if ((ret=syscall_create(argv[1], size)) < 0) {
    printf("Could not create %s with initial size %d.  Reason: %d\n", argv[1], size, ret);
    return 1;
  }
  return 0;
}

int cmd_rm(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: rm <files>...\n");
    return 1;
  }
  int i, ret = 0;
  for (i = 1; i < argc; i++) {
    int res = syscall_delete(argv[i]);
    if (res != 0) {
      printf("Failed to remove %s, reason %d.\n", argv[i+1], res);
      ret = 1;
    }
  }
  return ret;
}

int cmd_wait(int argc, char** argv) {
  argv=argv;
  if (argc != 1) {
    printf("Usage: wait\n");
  } else if (background_proc == -1) {
    printf("No background process to wait for.\n");
  } else {
    int retval = syscall_join(background_proc);
    background_proc = 1;
    return retval;
  }
  return 1;
}

int cmd_cmp(int argc, char** argv) {
  /* Note that this command is absurdly slow.  Never do single-byte
     reads in the real world. */
  if (argc != 3 && argc != 4) {
    printf("Usage: cmp <file1> <file2>\n");
    return 1;
  }
  int fd1, fd2;
  if ((fd1=syscall_open(argv[1])) < 0) {
    printf("Could not open %s.  Reason: %d\n", argv[1], fd1);
    return 1;
  }
  if ((fd2=syscall_open(argv[2])) < 0) {
    printf("Could not open file %s.  Reason: %d\n", argv[2], fd2);
    syscall_close(fd1);
    return 1;
  }
  char c1, c2;
  int rd1, rd2, i=0;
  do {
    rd1=syscall_read(fd1,&c1,1);
    rd2=syscall_read(fd2,&c2,1);
    clearline();
    printf("Comparing byte %d... ", i);
    if (rd1 != rd2) {
      printf("Files differ: not the same size.\n");
      break;
    } else if (c1 != c2) {
      printf("Files differ at position %d.\n", i);
      break;
    }
    i++;
  } while (rd1 != 0);
  printf("\n");
  syscall_close(fd1);
  syscall_close(fd2);
  return 0;
}

int cmd_echo(int argc, char** argv) {
  int i;
  for (i = 1; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  puts("\n");
  return 0;
}

int cmd_show(int argc, char** argv) {
  if (argc != 2) {
    printf("Usage: show <file>\n");
    return 1;
  }
  int fd;
  if ((fd=syscall_open(argv[1])) < 0) {
    printf("Could not open %s.  Reason: %d\n", argv[1], fd);
    return 1;
  }

  int rd;
  char buffer[BUFFER_SIZE];
  while ((rd = syscall_read(fd, buffer, BUFFER_SIZE))) {
    int wr=0, thiswr;
    while (wr < rd) {
      if ((thiswr = syscall_write(1, buffer+wr, rd-wr)) <= 0) {
        printf("\nCall to syscall_write() failed.  Reason: %d.\n", wr);
        syscall_close(fd);
        return 1;
      }
      wr += thiswr;
    }
  }
  if (rd < 0) {
    printf("\nCall to syscall_read() failed.  Reason: %d.\n", rd);
    syscall_close(fd);
    return 1;
  } else {
    syscall_close(fd);
    return 0;
  }
}

int cmd_run(char* prog) {
  if (does_file_exist(prog)) {
    return syscall_join(syscall_spawn(prog, NULL));
  } else {
    printf("No such program: %s.\n", prog);
    return 1;
  }
}

int background_run(char* prog) {
  if (background_proc != -1) {
    printf("Error: Can only run one background process at a time.\n");
    printf("Use 'wait' to block until it stops.\n");
    return 1;
  } else {
    if (does_file_exist(prog)) {
      background_proc = syscall_spawn(prog, NULL);
      printf("Background process spawned with PID %d\n", background_proc);
      return 0;
    } else {
      printf("No such program: %s.\n", prog);
      return 1;
    }
  }
}

void print_prompt(int last_retval)
{
  printf("%d> ", last_retval);
}

/* Note that tokenize(cmdline,argv) modifies cmdline by inserting NUL
   characters. */
int tokenize(char* cmdline, char** argv) {
  int argc = 0;
  int inword=0;
  char *s, *p;
  for (s = cmdline, p = cmdline; *s; s++) {
    if (*s == ' ' && inword) {
      inword=0;
      argv[argc++]=p;
      *s = '\0';
    } else if (*s != ' ' && !inword) {
      inword=1;
      p=s;
    }
  }
  if (inword) {
    argv[argc++]=p;
  }
  return argc;
}

int run_command(char* cmdline) {
  char* argv[BUFFER_SIZE];
  int argc = tokenize(cmdline, argv);
  if (argc == 0) {
    return 0;
  }
  if (strcmp(argv[0], "wait") == 0) {
    return cmd_wait(argc, argv);
  } else if (strcmp(argv[0], "ls") == 0) {
    return cmd_ls(argc, argv);
  } else if (strcmp(argv[0], "touch") == 0) {
    return cmd_touch(argc, argv);
  } else if (strcmp(argv[0], "cp") == 0) {
    return cmd_cp(argc, argv);
  } else if (strcmp(argv[0], "show") == 0) {
    return cmd_show(argc, argv);
  } else if (strcmp(argv[0], "rm") == 0) {
    return cmd_rm(argc, argv);
  } else if (strcmp(argv[0], "cmp") == 0) {
    return cmd_cmp(argc, argv);
  } else if (strcmp(argv[0], "echo") == 0) {
    return cmd_echo(argc, argv);
  } else if (strcmp(argv[0], "exit") == 0) {
    syscall_exit(0); 
    return 1; // not reached
  } else {
    int k = strlen(argv[argc-1]);
    if (argv[argc-1][k-1] == '&') {
      argv[argc-1][k-1] = '\0';
      return background_run(cmdline);
    } else {
      return cmd_run(cmdline);
    }
  }
}

void help() {
  printf("Welcome to the Buenos Shell!\n");
  printf("The following commands are available:\n");
  printf("  wait: Wait until the background process quits, then print its exit value.\n");
  printf("  ls: Print the contents of the given volume.\n");
  printf("  touch: Create a file with the given size.\n");
  printf("  cp: Create a copy of a file (the destination must not already exist).\n");
  printf("      The third argument to cp gives the maximum size of the destination (necessary in TFS)\n");
  printf("  show: Print the contents of the given file to the screen.\n");
  printf("  rm: Remove a number of ordinary files.\n");
  printf("  cmp: Compare the contents of two files.\n");
  printf("  echo: Print the arguments to the screen.\n");
  printf("  exit: leaves the shell\n");
}

int main(void) {
  char cmdline[BUFFER_SIZE];
  int ret = 0;
  help();
  while (1) {
    print_prompt(ret);
    (void) readline_static(cmdline, BUFFER_SIZE);
    run_command(cmdline);
  }
  syscall_halt();
  return 0;
}

/* Alternatively, we can use the following to do a static test: */

#ifdef FALSE

#define TEST_COMMAND(s) { char cmdline[BUFFER_SIZE] = s; run_command(cmdline); }

int main(void) {
  TEST_COMMAND("echo Copying");
  TEST_COMMAND("cp [arkimedes]theraven.txt [halibut]tofile 30000");
  TEST_COMMAND("echo Comparing source to destination");
  TEST_COMMAND("cmp [arkimedes]theraven.txt [halibut]tofile");
  TEST_COMMAND("echo Deleting destination");
  TEST_COMMAND("rm [halibut]tofile");
  return 0;
}
#endif
