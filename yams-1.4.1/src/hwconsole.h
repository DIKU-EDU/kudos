/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: hwconsole.h,v 1.18 2006/01/18 14:43:37 tlilja Exp $

   $Log: hwconsole.h,v $
   Revision 1.18  2006/01/18 14:43:37  tlilja
   Added support for the GDB remote debugging interface.

   Revision 1.17  2005/04/09 14:47:50  jaatroko
   Fixed some compiler warnings

   Revision 1.16  2002/11/20 21:13:15  ttakanen
   Added register based memory dump to hw-console

   Revision 1.15  2002/11/20 12:44:31  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.14  2002/11/17 16:02:24  ttakanen
   Implemented boot-command and kernel arguments

   Revision 1.13  2002/11/12 13:59:04  ttakanen
   TLBDUMP command

   Revision 1.12  2002/11/11 15:28:30  tlilja
   Updated copyright notices

   Revision 1.11  2002/11/01 20:33:35  ttakanen
   *** empty log message ***

   Revision 1.10  2002/10/31 17:10:30  tlilja
   Added -s/--script feature to yams.

   Revision 1.9  2002/10/16 12:30:41  ttakanen
   Fixed address adding bug in mem_direct*

   Revision 1.8  2002/10/16 12:18:31  ttakanen
   Implemented interrupt raising from console

   Revision 1.7  2002/10/10 18:44:50  ttakanen
   Implemented dump-command

   Revision 1.6  2002/10/08 20:28:47  ttakanen
   Implemented HELP command for hw-console

   Revision 1.5  2002/10/03 16:35:12  ttakanen
   New hw-console commands

   Revision 1.4  2002/10/03 15:54:19  ttakanen
   Added unbreak-command

   Revision 1.3  2002/10/03 14:19:26  ttakanen
   *** empty log message ***

   Revision 1.2  2002/10/03 13:05:32  javirta2
   *** empty log message ***

   Revision 1.1  2002/10/03 11:25:59  ttakanen
   Hardware console implementation skeleton works.

*/

/*
  Simulator hardware console commands
*/

#ifndef HWCONSOLE_H
#define HWCONSOLE_H

#include "simulator.h"

/* for flex input */
extern char *hwc_lexer_input_ptr;

int hwconsole_run();
int hwconsole_handle_command(char *cmd);

int command_source(char *filename);
void command_start();
void command_step(uint32_t stepcount);
void command_breakpoint(uint32_t address);
void command_quit(uint32_t exit_value);
void command_memread(uint32_t address, uint32_t length, char *filename);
int  command_memwrite(uint32_t address, char *filename);
void command_regdump(uint32_t processor_id);
void command_unbreak();
void command_interrupt(uint32_t interrupt_number, uint32_t cpu_nro);
void command_cpuregwrite(uint32_t processor_number, 
			 uint32_t register_number,
			 uint32_t value);
void command_cp0regwrite(uint32_t processor_number, 
			 uint32_t register_number,
			 uint32_t value);
void command_help(char *str);
void command_dump(uint32_t address, uint32_t words);
void command_dump_regsource(uint32_t processor_number,
			    uint32_t register_number,
			    uint32_t words);
void command_poke(uint32_t address, uint32_t word);
void command_tlbdump(uint32_t processor_id);
void command_boot(char *kernel_file, char *args);
void command_gdb(uint16_t port);

#endif
