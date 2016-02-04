/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2006 Juha Aatrokoski, Timo Lilja, Leena Salmela,
   Teemu Takanen, Aleksi Virtanen

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

   $Id: hwconsole.c,v 1.66 2010/11/24 08:54:15 tlilja Exp $

   $Log: hwconsole.c,v $
   Revision 1.66  2010/11/24 08:54:15  tlilja
   Reworked the GDB serial line interface support
    - drooped hw console support
    - made gdb.c use async_input interface
    - added documentation to info

   Revision 1.65  2006/01/18 14:43:37  tlilja
   Added support for the GDB remote debugging interface.

   Revision 1.64  2006/01/12 17:11:15  jaatroko
   printf format from 'lf' to the correct 'f'

   Revision 1.63  2005/06/05 15:00:20  jaatroko
   C99 constant and formatting macros

   Revision 1.62  2005/05/08 17:12:20  jaatroko
   ELF support

   Revision 1.61  2005/04/14 17:45:45  jaatroko
   *** empty log message ***

   Revision 1.60  2005/04/09 15:54:57  jaatroko
   Forgot one PRIu64 change

   Revision 1.59  2005/04/09 15:51:57  jaatroko
   Added performance meter to the start command; CPU cycle printf now uses PRIu64 (defined by ISO C99 inttypes.h)

   Revision 1.58  2004/01/12 08:58:18  ttakanen
   Fixed printf field lengths

   Revision 1.57  2003/05/22 11:01:13  ttakanen
   memread&memwrite fix

   Revision 1.56  2003/01/29 22:54:53  javirta2
   Disassembler half ready.

   Revision 1.55  2002/11/26 11:29:40  ttakanen
   Removed old xxxs

   Revision 1.54  2002/11/26 10:40:23  ttakanen
   Fixed software shutdown exit value in scripts

   Revision 1.53  2002/11/25 14:34:54  lsalmela
   Virtual exception name mapping

   Revision 1.52  2002/11/22 23:35:36  jaatroko
   dump & poke exception reporting

   Revision 1.51  2002/11/22 21:11:04  ttakanen
   Excluded prompt printing when readline is in use.

   Revision 1.50  2002/11/22 20:35:10  tlilja
   Added support for 255 -s arguments.

   Revision 1.49  2002/11/22 20:20:13  tlilja
   *** empty log message ***

   Revision 1.48  2002/11/21 17:15:48  ttakanen
   *** empty log message ***

   Revision 1.47  2002/11/20 23:07:58  ttakanen
   *** empty log message ***

   Revision 1.46  2002/11/20 21:13:15  ttakanen
   Added register based memory dump to hw-console

   Revision 1.45  2002/11/20 20:11:56  tlilja
   *** empty log message ***

   Revision 1.44  2002/11/20 16:22:21  jaatroko
   dump & poke through translation

   Revision 1.43  2002/11/20 13:28:48  ttakanen
   Implemented command help

   Revision 1.42  2002/11/20 13:03:31  ttakanen
   Removed old XXXs

   Revision 1.41  2002/11/20 12:44:30  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.40  2002/11/17 16:02:24  ttakanen
   Implemented boot-command and kernel arguments

   Revision 1.39  2002/11/17 15:18:02  ttakanen
   Changed poke to use mem_dwrite (byte order conversion)

   Revision 1.38  2002/11/17 13:00:41  jaatroko
   ASCII dump restricted to 32..126

   Revision 1.37  2002/11/16 17:14:40  jaatroko
   ascii codes

   Revision 1.36  2002/11/12 14:01:50  javirta2
   *** empty log message ***

   Revision 1.35  2002/11/12 13:59:03  ttakanen
   TLBDUMP command

   Revision 1.34  2002/11/11 21:28:25  javirta2
   Fixed step.

   Revision 1.33  2002/11/11 15:28:30  tlilja
   Updated copyright notices

   Revision 1.32  2002/11/08 23:16:02  jaatroko
   ASCII memory dump

   Revision 1.31  2002/11/02 11:26:49  tlilja
   Added history to readline

   Revision 1.30  2002/11/02 11:11:32  tlilja
   Removed platform specific stuff from Makefile.am
   Added some headers and readline support

   Revision 1.29  2002/11/01 20:33:35  ttakanen
   *** empty log message ***

   Revision 1.28  2002/10/31 17:10:30  tlilja
   Added -s/--script feature to yams.

   Revision 1.27  2002/10/30 22:29:48  javirta2
   Read macro for registers EntryHi, EntryLo1, EntryLo0. Read and write macros
   for tlb entries.

   Revision 1.26  2002/10/28 20:47:08  jaatroko
   mem_read ja mem_write k‰ytt‰m‰‰n cpu:ta cp0:n sijaan

   Revision 1.25  2002/10/23 06:39:05  javirta2
   *** empty log message ***

   Revision 1.24  2002/10/20 18:12:38  javirta2
   Readline compatibility.

   Revision 1.23  2002/10/17 12:19:43  ttakanen
   Implented interrupts

   Revision 1.22  2002/10/16 12:30:41  ttakanen
   Fixed address adding bug in mem_direct*

   Revision 1.21  2002/10/16 12:18:30  ttakanen
   Implemented interrupt raising from console

   Revision 1.20  2002/10/13 15:28:51  ttakanen
   *** empty log message ***

   Revision 1.19  2002/10/11 11:37:58  javirta2
   *** empty log message ***

   Revision 1.18  2002/10/10 18:44:50  ttakanen
   Implemented dump-command

   Revision 1.17  2002/10/09 18:05:32  ttakanen
   *** empty log message ***

   Revision 1.16  2002/10/09 08:57:41  tlilja
   Fixed bug in simulator.c
   Code works now in Linux and Solaris.

   Revision 1.15  2002/10/08 20:28:47  ttakanen
   Implemented HELP command for hw-console

   Revision 1.14  2002/10/08 16:47:12  ttakanen
   Corrected compiler warnings

   Revision 1.13  2002/10/07 13:43:02  ttakanen
   Added ADDUI-instruction implementation

   Revision 1.12  2002/10/06 11:37:17  ttakanen
   *** empty log message ***

   Revision 1.11  2002/10/06 11:24:01  ttakanen
   *** empty log message ***

   Revision 1.10  2002/10/04 17:12:53  ttakanen
   Added lexer reset to prevent broken input propagation.

   Revision 1.9  2002/10/04 09:53:45  ttakanen
   *** empty log message ***

   Revision 1.8  2002/10/03 20:39:57  ttakanen
   *** empty log message ***

   Revision 1.7  2002/10/03 16:35:12  ttakanen
   New hw-console commands

   Revision 1.6  2002/10/03 15:54:19  ttakanen
   Added unbreak-command

   Revision 1.5  2002/10/03 14:14:28  ttakanen
   Regdump implementation.

   Revision 1.4  2002/10/03 13:35:16  javirta2
   *** empty log message ***

   Revision 1.3  2002/10/03 13:05:32  javirta2
   *** empty log message ***

   Revision 1.2  2002/10/03 12:16:00  javirta2
   *** empty log message ***

   Revision 1.1  2002/10/03 11:25:59  ttakanen
   Hardware console implementation skeleton works.

*/

/*
  Simulator hardware console commands
*/


#include "includes.h"
#include "hwconsole.h"
#include "memory.h"
#include "hwconsole.h"
#include "cpu_defs.h"
#include "elf.h"
#include "gdb.h"
#include <assert.h>
#include <sys/time.h>

#ifdef READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* READLINE */

#define HW_CONSOLE_MAX_PROMPT 64
#define HW_CONSOLE_MAX_LINE 500

void reset_lexer();
void yyparse();

/* Input string for (f)lex. Contains the unprocessed part of the input
   string. */

char *hwc_lexer_input_ptr;
static int yams_exit_code=0;

/* Help strings for commands */

struct help_string_s {
    char *command;
    char *short_command;
    char *one_line_description;
    char *long_help;
} help_strings[] = {
    { "help [command]", "h", "Print help [for command]", 
      "Prints summary of available commands.\n\n"
      "If command name is given as argument, prints help for that command."
    },

    { "start", "s", "Start simulation", 
      "Start command starts the simulation loop. While running the\n"
      "simulation, YAMS doesn't take console commands. To return to\n"
      "console and stop the simulation, send interrupt signal to YAMS\n"
      "(usually by pressing CTRL-C).\n"
      "\n"
      "The stopped simulation can be continued with a new start command."
    },

    { "step [n]", "t", "Step one [or n] clock cycles", 
      "Step runs the simulator for one clock cycle and then drops back to\n"
      "the console. If numeric argument is given to step command, given\n"
      "number of clock cycles is simulated before dropping back to the\n"
      "console.\n"
      "\n"
      "If premature returning is needed, YAMS can be forced to drop\n"
      "back to the console by sending interrupt signal (usually by pressing\n"
      "CTRL-C)."
    },

    { "break addr", "b", "Set breakpoint at addr", 
      "Break command set hardware breakpoint at the address given as\n"
      "argument to the command. When any CPU in the system loads instruction\n"
      "from the given address, YAMS drops to the console.\n"
      "\n"
      "Only one breakpoint can be active at the same time."
    },

    { "quit [exitcode]", "q", "Quit YAMS [with exit value exitcode]", 
      "Quit command exits YAMS. By default, @code{yams} exists with\n"
      "exit code 0, but if some other code is needed (usually when running\n"
      "scripted tests), exit value in range [0,255] can be given as an\n"
      "argument to the quit-command." 
    },

    { "interrupt n [cpu]", "i", "Raise interrupt line n [on CPU cpu]", 
      "Hardware and software interrupt lines can be raised with interrupt\n"
      "command. The raising will be valid only for one clock cycle. After\n"
      "that, CPU will automatically clear the interrupt as non-pending.\n"
      "\n"
      "Interrupt command takes interrupt number as first argument.\n"
      "second argument specifies the identification number of the CPU which\n"
      "should get the interrupt request. By default interrupts go to CPU 0.\n"
      "\n"
      "The interrupt number number in closed range [0,7]."
    },

    { "regdump [n]", "r", "Print register contents [for CPU n]", 
      "Regdump command prints contents of CPU and CP0 registers. By default\n"
      "CPU 0 and it's co-processor 0 status is printed. If some print for\n"
      "some other CPU is needed, regdump takes numeric argument which\n"
      "specifies the processor number. Processors are numbered starting from\n"
      "0."
    },

    { "tlbdump [cpu]", "", "Print contents of TLB [for CPU n]", 
      "Tlbdump command prints the contents of translation lookaside buffer\n"
      "for CPU 0. If numeric argument is given to the command, it specifies\n"
      "some other CPU than CPU 0 for printing."
    },

    { "regwrite [cpu:]reg v", "w", "Write v to register reg [on CPU cpu]",
      "CPU and CP0 registers can be written with regwrite command. The first\n"
      "argument for the command is the name of the register (register names\n"
      "can be seen with regdump command). The second argument is the new\n"
      "value to store in the given register.\n"
      "\n"
      "By default CPU 0 registers are affected, but register name can be\n"
      "prefixed by CPU number and colon to store into some other CPU.\n"
      "\n"
      "Some examples:\n"
      "\n"
      "regwrite s0 0xdeadbeef\n"
      "regwrite 1:sp 0x00030000"
    },

    { "memwrite addr \"file\"", "l", "Load file into address addr", 
      "Memwrite reads a file and writes it into simulators memory. The first\n"
      "argument to memwrite command must be a valid hardware memory address\n"
      "(ie. memory address relative to 0, not a segmented address) where to\n"
      "load the file. The second argument is the name of the file to read in\n"
      "quotation marks." 
    }, 

    { "memread addr l \"file\"", "m", "Write l bytes from addr to file", 
      "Memread reads simulator part of simulator memory and writes it in a\n"
      "file. The first argument to memread command must be a valid hardware\n"
      "memory address (ie. memory address relative to 0, not a segmented\n"
      "address) where to start the read from. The second argument is the\n"
      "number of bytes to read. The third argument is the name of the file\n"
      "to be written in quotation marks."
    },

    { "unbreak", "u", "Unset breakpoint", 
      "Unbreak command clears hardware breakpoints." 
    },

    { "dump [a|[c:]reg] [w]", "d", "Dump memory [start addr] [w words]", 
      "Contents of simulator memory can be seen with the dump command. By\n"
      "default, the command prints 11 words surrounding CPU 0 program\n"
      "counter. This is useful when stepping programs.\n"
      "\n"
      "Dump takes the beginning address of the dump as an optional first\n"
      "argument. The second, also optional, argument is the number of words\n"
      "to dump. Address can be substituted with name of CPU-register. The\n"
      "specified register contents are used as an address to start dump from.\n"
      "The register name can be prefixed by processor id-number and colon,\n"
      "by default CPU 0 is used."
    },

    { "poke addr w", "p", "Write word w at address addr", 
      "One word can be written into simulator memory by poke command. Poke\n"
      "takes the memory address as the first argument and value to be stored\n"
      "as second argument. Only full words can be written."
    },

    { "boot \"file\" \"args\"", "b", "Boot kernel from file [with arguments]", 
      "Boot command can be used to boot a kernel image. Boot command takes\n"
      "the name of the kernel image file in quatation marks as it's first\n"
      "argument. The second argument is optional quoted string of kernel arguments.\n"
      "\n"
      "For example, to boot Buenos kernel from \"buenos.img\" with\n"
      "arguments:\n"
      "\n"
      "boot \"buenos.img\" \"startproc=shell\"\n"
      "\n"
      "The exact boot process is:\n"
      "\n"
      "1. Kernel image file is loaded into memory. This step is equivalent\n"
      "to command memwrite 0x00010000 \"image\" .\n"
      "\n"
      "2. Program counters in all CPUs are set to 0x80010000. This could be\n"
      "done manually by using command regwrite pc 0x80010000 for each CPU.\n"
      "\n"
      "3. Kernel argument string is copied into it's memory area. This can't\n"
      "be done without boot command.\n"
      "\n"
      "4.Simulation is started. This step could be done manually with\n"
      "command start."
    },

    { NULL, NULL, NULL, NULL }
};

/* Process one line of input from user or file. The string must end
   with a newline. 

*/

int hwconsole_handle_command(char *cmd) {
    hwc_lexer_input_ptr = cmd;
    reset_lexer();
    yyparse();
    return 0;
}

#ifndef READLINE
static void hwconsole_print_prompt(char *prompt) {
    printf("YAMS [%" PRIu64 "]> ", hardware->cycle_count);
    fflush(stdout);
}

static char *replace_with_gnu_readline(char *prompt) {
    char buf[HW_CONSOLE_MAX_LINE+1];
    char *buf1;

    printf("%s", prompt);
    fflush(stdout);
    
    if (fgets(buf, HW_CONSOLE_MAX_LINE+1, stdin) == NULL) 
	return NULL;
    
    buf1 = calloc(HW_CONSOLE_MAX_LINE, sizeof(char));
    if(buf1 != NULL) {
	strncpy(buf1,buf,strlen(buf)-1);
    }
    
    return buf1;
}
#endif /* READLINE */

static int hwconsole_read_console() {
    char prompt[HW_CONSOLE_MAX_PROMPT];
    char command[HW_CONSOLE_MAX_LINE+1];
    char *buf;

    sprintf(prompt,"YAMS [%" PRIu64 "]> ", hardware->cycle_count);

#ifdef READLINE
    buf = readline(prompt);

    if(buf == NULL)
	return 1; /* quit at EOF */
    if (strlen(buf) > 1)
        add_history(buf);
#else
    buf = replace_with_gnu_readline(prompt);

    if(buf == NULL)
	return 1; /* quit at EOF */
#endif /* HAVE_LIBREADLINE */

    strcpy(command,buf);
    strcat(command,"\n");

    free(buf);

    return hwconsole_handle_command(command);
}

int hwconsole_run() {
    while(!hwconsole_read_console()) {
	/* quit command sets running state to -1 */
	if(hardware->running == -1)
	    break;
    }

    return yams_exit_code;
}
/* Return values:
 *  * -1: run yams
 *  *  0: file not found
 *  * >0: yams exit code.
 */
int command_source(char *filename) {
    FILE *source_file;

    if((source_file = fopen(filename, "rt")) == NULL) {
	printf("Can't read file '%s'.\n", filename);
	return -2;
    }

    while(!feof(source_file) && hardware->running != -1) {
	char buf[HW_CONSOLE_MAX_LINE+1];

	if(fgets(buf, HW_CONSOLE_MAX_LINE, source_file) == NULL) {
	    break; /* quit at EOF */
	}

	hwconsole_handle_command(buf);
    }

    fclose(source_file);
    if(hardware->running == -1)
	return yams_exit_code;
    else
	return -1;
}

/* Start simulation. */

void command_start() {
    struct timeval start_time, end_time;
    uint64_t start_cycle;
    double t, hz;

    printf("YAMS running...");
    fflush(stdout);

    start_cycle = hardware->cycle_count;
    gettimeofday(&start_time, NULL);

    simulator_run(hardware->cycle_count - 1);
    printf("\n");

    gettimeofday(&end_time, NULL);

    t = (double)(end_time.tv_sec - start_time.tv_sec) 
	+ (double)(end_time.tv_usec - start_time.tv_usec) / 1000000.0;

    hz = (double)(hardware->cycle_count - start_cycle) / t;

    printf("    Running time: %.2f s\n"\
	   "    Actual performance (average): %.0f kHz\n"\
	   "    Simulated second length (average): %.2f s\n", 
	   t, hz / 1000.0, (double)hardware->clockspeed / hz);
}

/* Step <stepcount> instructions. */

void command_step(uint32_t stepcount) {
    simulator_run(hardware->cycle_count+(uint64_t)stepcount);
}

/* Set hardware breakpoint at PC <address> */

void command_breakpoint(uint32_t address) {
    printf("command_breakpoint: 0x%8x\n", address);
    hardware->breakpoint = address;
}

/* Quit simulator after statistics printing and cleanup. */

void command_quit(uint32_t exitcode) {
    if(exitcode > 255) {
	printf("Exit code out of range [0,255]\n");
    } else {
	hardware->running = -1;
	yams_exit_code = (int) exitcode;
    }
}

/* Write data from file to memory. 
   address         - address the data will be written
   filename        - data
*/
int command_memwrite(uint32_t address, char *filename) {
    FILE *f;
    char buf[SIMULATOR_PAGESIZE];
    int n;

    if((f = fopen(filename,"rb")) == NULL) {
	printf("Couldn't open file [%s].\n",filename);
	return 1;
    }

    do {
	n = fread(buf,1,SIMULATOR_PAGESIZE,f); 
	if(n < 0) {
	    printf("File read error.\n");
	    return 1;
	}

	if(mem_store_direct(hardware->memory, address, n, buf)) {
	    printf("Attempt to write outside memory.\n");
	    return 1;
	}
	address = address + n;
    } while( n > 0 );
 
    fclose(f);
    return 0;
}

/* Read data from memory to a file. 
   address     - start position
   length      - bytes to be read
   filename    - file the the data will be written into
*/
void command_memread(uint32_t address, uint32_t length, char *filename) {
    FILE *f;
    char buf[SIMULATOR_PAGESIZE];
    int n;
    uint32_t from_a, size;

    if((f = fopen(filename,"wb")) == NULL) {
	printf("Couldn't open file [%s].\n",filename);
	return;
    }

    from_a = address;
    while(from_a < (address + length)) {
	if((from_a + SIMULATOR_PAGESIZE) <= (address + length))
	    size = SIMULATOR_PAGESIZE;
	else
	    size = (address + length) % SIMULATOR_PAGESIZE;
	    
	if(mem_read_direct(hardware->memory, from_a, size, buf)) {
	    printf("Attempt to read outside memory.\n");
	    return;
	}

	n = fwrite(buf,1,size,f); 
	if(n != size) {
	    printf("File write error.\n");
	    return;
	}

	from_a = from_a + size;
    };

 
    fclose(f);
}

static void print_cpu_register(cpu_t *cpu, int regnro) {
    if(regnro >= 32) {
	printf("   %-5s = %.8" PRIx32 " ", 
	       cpu_register_names[regnro],
	       READ_REG(cpu, regnro)
	       );
    } else {
	printf("%2d %-5s = %.8" PRIx32 " ", 
	       regnro,
	       cpu_register_names[regnro],
	       READ_REG(cpu, regnro)
	       );
    } 
}

static void print_cp0_register(cpu_t *cpu, int regnro) {
    if(regnro >= 32) {
	printf("   %-6s= %.8" PRIx32 " ", 
	       cp0_register_names[regnro],
	       READ_CP0_REG(cpu->cp0, regnro)
	       );
    } else {
	printf("%2d %-6s= %.8" PRIx32 " ", 
	       regnro,
	       cp0_register_names[regnro],
	       READ_CP0_REG(cpu->cp0, regnro)
	       );
    }
}


void command_regdump(uint32_t processor_id) {
    int i;

    if(processor_id >= hardware->num_cpus) {
	printf("No such processor (CPU %" PRIu32 ")\n", processor_id);
	return;
    }

    printf("CPU %" PRIu32 " - ", processor_id);
    
    if(CP0_KERNEL_MODE(hardware->cpus[processor_id])) {
	printf("In Kernel Mode");
    } else {
	printf("In User Mode");
    }

    for(i=0; i<NumCpuRegs; i++) {
	if(i%4 == 0)    
	    printf("\n");
	print_cpu_register(hardware->cpus[processor_id], 
			   (i%4)*(NumCpuRegs/4+1)+i/4);
    }

    printf("\n\n");

    printf("CPU %" PRIu32 " Co-processor 0 registers:", processor_id);
    
    for(i=0; i<NumCP0Regs; i++) {
	if(i%4 == 0)    
	    printf("\n");
	print_cp0_register(hardware->cpus[processor_id], 
			   (i%4)*(NumCP0Regs/4)+i/4);
    }

    printf("\n");
}

void command_unbreak() {
    hardware->breakpoint = 0xffffffff;
}

void command_interrupt(uint32_t interrupt_number, uint32_t cpu_number) {
    assert(cpu_number < hardware->num_cpus);

    if(interrupt_number > 7) {
	printf("Interrupt number must be between [0,7]\n");
	return;
    }

    RAISE_INTERRUPT(cpu_number, interrupt_number);

    if(interrupt_number <= 1) {
	printf("Raised software interrupt line %" PRIu32 "\n",
	       interrupt_number);
    } else {
	printf("Raised hardware interrupt line %" PRIu32 "\n",
	       interrupt_number-2);
    }
}


void command_cpuregwrite(uint32_t processor_number, 
			 uint32_t register_number,
			 uint32_t value) {
    if(processor_number >= hardware->num_cpus) {
	printf("Invalid processor number.\n");
	return;
    }

    if(register_number >= NumCpuRegs) {
	printf("Invalid CPU register.\n");
	return;
    }

    WRITE_REG(hardware->cpus[processor_number], register_number, value);

    /* Kludge to handle delay slots correctly */
    if(register_number == PC) {
	hardware->cpus[processor_number]->next_pc = value + 4;
    }
    
}

void command_cp0regwrite(uint32_t processor_number, 
			 uint32_t register_number,
			 uint32_t value) {
    if(processor_number >= hardware->num_cpus) {
	printf("Invalid processor number.\n");
	return;
    }

    if(register_number >= NumCP0Regs) {
	printf("Invalid CP0 register.\n");
	return;
    }    

    WRITE_CP0_REG(hardware->cpus[processor_number]->cp0, 
		  register_number, 
		  value);
}

void command_help(char *str) {
    int i;

    if(str == NULL) {
	for(i=0; help_strings[i].command != NULL ; i++) {
	    printf("%-21s  (%-1s)  %s\n",
		   help_strings[i].command,
		   help_strings[i].short_command,
		   help_strings[i].one_line_description);
	}
    } else {
	for(i=0; help_strings[i].command != NULL ; i++) {
	    if(!strncmp(help_strings[i].command, str, strlen(str))) {
		printf("%s:\n\n%s\n\n", 
		       help_strings[i].command,
		       help_strings[i].long_help);
		return;
	    }
	}

	printf("No help available for '%s'\n", str);
    }
}

extern char *exception_names[]; /* in cpu.c */

static void dump_word(uint32_t address) {
    uint32_t word;
    int i;
    char buf[128];
    char ascii[5];

    exception_t exc;
    uint32_t save_status;

    buf[127] = 0;
    ascii[4] = 0;


    /* This kludge assures that mem_read thinks the CP0
     * is in kernel mode
     */
    save_status = hardware->cpus[0]->cp0->registers[Status];
    hardware->cpus[0]->cp0->registers[Status] = 6;
    exc = mem_read(hardware->memory, address, &word, 4,
		   hardware->cpus[0]);
    hardware->cpus[0]->cp0->registers[Status] = save_status;

    /* Translate virtual exceptions */
    if (exc == TLBLoadInvalid)
        exc = TLBLoad;
    else if (exc == TLBStoreInvalid)
        exc = TLBStore;

    if(exc == NoException) {
	disasm_instruction(address, word, buf, 50);

	ascii[0] = word>>24;
	ascii[1] = (word>>16) & 0xff;
	ascii[2] = (word>>8) & 0xff;
	ascii[3] = word & 0xff;
	/* characters not in range [0..31] should be printable */
	if (ascii[0] < 32 || ascii[0] == 127) ascii[0] = '.';
	if (ascii[1] < 32 || ascii[1] == 127) ascii[1] = '.';
	if (ascii[2] < 32 || ascii[2] == 127) ascii[2] = '.';
	if (ascii[3] < 32 || ascii[3] == 127) ascii[3] = '.';

	for(i=32 ; i>=0 ; i--) {
	    if(READ_REG(hardware->cpus[0], i) == address) {
		printf("%2s=> %.8" PRIx32 ": %s  %.8" PRIx32 "  %-50s\n", 
		       cpu_register_names[i],
		       address,
		       ascii,
		       word,
		       buf);
		break;
	    }
	}
	if(i<0) {
		printf("     %.8" PRIx32 ": %s  %.8" PRIx32 "  %-50s\n", 
		       address,
		       ascii,
		       word,
		       buf);
	}
    } else {
	/* XXX: No register pointers (PC=> etc.) */
	printf("     %.8" PRIx32 ": Exception %s\n", 
	       address, exception_names[exc]);
    }

}

void command_dump_regsource(uint32_t processor_number,
			     uint32_t register_number,
			     uint32_t words)  {
    if(processor_number >= hardware->num_cpus) {
	printf("Invalid CPU\n");
	return;
    }

    command_dump(READ_REG(hardware->cpus[processor_number],
			  register_number),
	         words); 
}

void command_dump(uint32_t address, uint32_t words) {
    uint32_t w;

    if(address == 0xffffffff) {
	address = READ_REG(hardware->cpus[0], PC) - 20;
	if(address >= 0xffffffea)
	    address = 0;
    }

    for(w = 0 ; w < words ; w++) {
	dump_word(address + w*4);
    }
}

void command_poke(uint32_t address, uint32_t word) {
    exception_t exc;
    uint32_t save_status;

    /* This kludge assures that mem_write thinks the CP0
     * is in kernel mode
     */
    save_status = hardware->cpus[0]->cp0->registers[Status];
    hardware->cpus[0]->cp0->registers[Status] = 6;
    exc = mem_write(hardware->memory, address, &word, 4,
		    hardware->cpus[0]);
    hardware->cpus[0]->cp0->registers[Status] = save_status;

    /* Translate virtual exceptions */
    if (exc == TLBLoadInvalid)
        exc = TLBLoad;
    else if (exc == TLBStoreInvalid)
        exc = TLBStore;

    if (exc != NoException)
	printf("mem_write() failed, exception %s\n", exception_names[exc]);
}

void command_tlbdump(uint32_t processor_id) {
    uint32_t i;
    cp0_t *cp0;

    if(processor_id >= hardware->num_cpus) {
	printf("No such processor (CPU %" PRIx32 ")\n", processor_id);
	return;
    }

    cp0 = hardware->cpus[processor_id]->cp0;

    printf("TLB of CPU %" PRIx32 ":\n\n", processor_id);

    printf("Row VADDR    VPN2  G ASID PFN0     C0 D0 V0 PFN1     C1 D1 V1\n");
    printf("=== ======== ===== = ==== ======== == == == ======== == == ==\n");
    
    for(i=0; i<NUM_TLB_ENTRIES; i++) {

	printf("%2" PRIx32 ": %.8" PRIx32 " %.5" PRIx32 " %" PRId32 " %.4" PRIx32 " %.8" PRIx32 " %.2" PRIx32 " %.1" PRIx32 "  %.1" PRIx32 "  %.8" PRIx32 " %.2" PRIx32 " %.1" PRIx32 "  %.1" PRIx32 "\n"    
	       ,i,
	       TLB_VPN2(cp0, i) << 13,
	       TLB_VPN2(cp0, i),
	       TLB_G(cp0, i),
	       TLB_ASID(cp0, i),
	       TLB_PFN0(cp0, i),
	       TLB_C0(cp0, i),
	       TLB_D0(cp0, i),
	       TLB_V0(cp0, i),
	       TLB_PFN1(cp0, i),
	       TLB_C1(cp0, i),
	       TLB_D1(cp0, i),
	       TLB_V1(cp0, i));
	       
    }
}

void command_boot(char *kernel_image, char *arguments) {
    int i;
    int l;
    uint32_t entry_point = STARTUP_PC;

    l=strlen(arguments);

    if(l >= KERNEL_PARAMAREA_LENGTH-1) {
	printf("Kernel parameter string too long\n");
	printf("Boot aborted\n");
	return;
    }

    printf("Loading image...\n");

    /* Try loading an ELF file. If it's not an ELF file, treat it as a
     * binary file
     */
    i = load_elf(kernel_image, &entry_point);
    if (i == LOADELF_NOTELF) {
	if(command_memwrite(STARTUP_REAL_ADDRESS, kernel_image)) {
	    printf("Boot aborted\n");
	    return;
	}
    } else if (i != LOADELF_SUCCESS) {
	printf("Boot aborted\n");
	return;
    }

    for(i=0; i<hardware->num_cpus; i++) {
	WRITE_REG(hardware->cpus[i], PC, entry_point);

	/* Kludge to handle delay slots correctly */
	hardware->cpus[i]->next_pc = entry_point + 4;
    }

    printf("Kernel boot arguments are: \"%s\"\n", arguments);

    /* Set kernel arguments */
    memset(hardware->memory->kernel_paramarea, 0, KERNEL_PARAMAREA_LENGTH);
    memcpy(hardware->memory->kernel_paramarea, arguments, l);

    printf("Booting kernel \"%s\" at address #%.8" PRIx32 "\n",
	   kernel_image, entry_point);

    command_start();
}
