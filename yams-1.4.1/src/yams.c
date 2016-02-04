/* yams.c -- Yet Another Machine Simulator
   Copyright (C) 2002-2010 Juha Aatrokoski, Timo Lilja, Leena Salmela,
     Teemu Takanen, Aleksi Virtanen

   Copyright (C) 1992, 1995, 1996, 1997-1999, 2000-2002
                  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: yams.c,v 1.32 2010/11/24 13:33:20 jaatroko Exp $
*/

#include "includes.h"
#include "simulator.h"
#include "io.h"
#include "hwconsole.h"
#include "cfg.h"
#include "memory.h"
#include "gdb.h"

struct option longopts[] =
   {{ "config", required_argument, NULL, 'c'},
    { "help", no_argument, NULL, 'h' },
    { "script", required_argument, NULL, 's'},
    { "version", no_argument, NULL, 'v' },
    { "gdb", required_argument, NULL, 'g' },
    { NULL, 0, NULL, 0 }};

static void print_help(void)
{
    printf ("\
YAMS, Yet Another Machine Simulator\n\
Usage yams [-chsvg] [--config file] [--script file] [--help] [--version]\n\
           [--gdb tcp-port] [binary-file [opt1] .. [opt n]]\n\
  -c, --config file       Read configuration from 'file' instead\n\
  -s, --script file       Read a script from 'file' before going to prompt\n\
  -g, --gdb tcp-port      Start the GDB interface instead of the HW console\n\
  -v, --version           Print the version information\n\
  -h, --help              Print a summary of the options\n\
");
}


extern char version[];

static void print_version(void)
{
    printf("YAMS - Yet Another Machine Simulator %s \n", version);
    printf("\n");
    printf("\
Copyright (C) 2002-2010 Juha Aatrokoski, Timo Lilja, Leena Salmela,\n\
  Teemu Takanen, Aleksi Virtanen\n\
\n\
Yams comes with ABSOLUTELY NO WARRANTY; This is free software,\n\
and you are welcome to redistribute it under certain conditions;\n\
see the file COPYING for details.\n\
\n");
}



/* for our attempts to release all memory */
void *yams_original_brk;


int main(int argc, char *argv[])
{
    int optc;
    int help = 0, version = 0, config = 0;
    int script = 0, gdb_debug = 0, binary = 0, lose = 0;
    int exit_code = 0, scripts = 0;
    int gdb_port = 0;
    int ret = -1;

    char config_file[MAX_FILENAME_LENGTH];
    char script_file[MAX_FILENAME_LENGTH][MAX_SCRIPTS];
    char *binary_file = NULL;
    char kernel_params[KERNEL_PARAMAREA_LENGTH];

#ifdef HAVE_BRK
    yams_original_brk = sbrk(0);
#endif

    while ((optc = getopt_long(argc, argv, "+c:hs:vg:", longopts, (int *) 0))
           != EOF)

      switch (optc) {
      case 'v':
          version = 1;
          break;
      case 'h':
          help = 1;
          break;
      case 'c':
          config = 1;
          strncpy(config_file, optarg, MAX_FILENAME_LENGTH);
          break;
      case 's':
          script = 1;
          if (scripts < MAX_SCRIPTS)
              strncpy(script_file[scripts++], optarg, MAX_FILENAME_LENGTH);
          break;
      case 'g':
          gdb_debug = 1;
          gdb_port = strtol(optarg, NULL, 10); /* XXX error checking */
          break;
      default:
          lose = 1;
          break;
      }

    if (lose || optind < argc) {
        int i, space_left = KERNEL_PARAMAREA_LENGTH;
        binary_file = argv[optind];
        binary = 1;

        for (i = optind+1; i < argc && space_left > 0; i++) {
            space_left -= strlen(argv[i]) + 1;
            strncat(kernel_params, argv[i], space_left - 1);
            if (i+1 != argc)
                strncat(kernel_params, " ", 1);
        }
    }

    if (help) {
      /* Print help info and exit.  */
        print_help();
        exit(EXIT_SUCCESS);
    }

    print_version();

    if (version)
        exit(EXIT_SUCCESS);

    if (config && !cfg_read(config_file)) {
        printf("yams: unable to read configuration from file: %s\n",
               config_file);
        exit(1);
    }
    if (!config && !cfg_read_cwd() && !cfg_read_home() && !cfg_read_etc()) {
        printf("yams: unable to read configuration file.\n");
        exit(1);
    }
    cfg_init();

#ifdef WORDS_BIGENDIAN
    printf("Starting on a big-endian host.\n");
#else
    printf("Starting on a little-endian host.\n");
#endif /* WORDS_BIGENDIAN */

    if (simulator_bigendian)
	printf("Simulating a big-endian machine.\n");
    else
	printf("Simulating a little-endian machine.\n");


    if (gdb_debug)
        gdb_interface_open(gdb_port);

    simulator_init();


    if (binary) {
        command_boot(binary_file, kernel_params);
        if(hardware->running == -1)
            exit(EXIT_SUCCESS);
    }

    if (script) {
            int i;
            for (i = 0; i < scripts; i++) {
                ret = command_source(script_file[i]);
                if (ret == -2) /* file not found */
                    exit(EXIT_FAILURE);
                else if (ret >= 0) /* stop yams */
                    break;
            }
    }
    if (ret == -1) {
        exit_code = hwconsole_run();
    } else
        exit_code = ret;

    exit(exit_code);
}


/* yams.c ends here */
