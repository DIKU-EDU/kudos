/* -*- fundamental -*- (for emacs) */

/* yams.c -- Yet Another Machine Simulator
   Copyright (C) 2002 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: hwcons.y,v 1.23 2010/11/24 08:54:15 tlilja Exp $

   $Log: hwcons.y,v $
   Revision 1.23  2010/11/24 08:54:15  tlilja
   Reworked the GDB serial line interface support
    - drooped hw console support
    - made gdb.c use async_input interface
    - added documentation to info

   Revision 1.22  2006/01/18 14:43:37  tlilja
   Added support for the GDB remote debugging interface.

   Revision 1.21  2002/11/20 21:13:14  ttakanen
   Added register based memory dump to hw-console

   Revision 1.20  2002/11/20 13:28:48  ttakanen
   Implemented command help

   Revision 1.19  2002/11/20 12:44:30  ttakanen
   YAMS = Yet Another Machine Simulator

   Revision 1.18  2002/11/17 16:02:23  ttakanen
   Implemented boot-command and kernel arguments

   Revision 1.17  2002/11/12 13:59:03  ttakanen
   TLBDUMP command

   Revision 1.16  2002/11/11 15:28:30  tlilja
   Updated copyright notices

   Revision 1.15  2002/11/01 20:33:34  ttakanen
   *** empty log message ***

   Revision 1.14  2002/10/16 12:30:40  ttakanen
   Fixed address adding bug in mem_direct*

   Revision 1.13  2002/10/16 12:18:30  ttakanen
   Implemented interrupt raising from console

   Revision 1.12  2002/10/10 18:44:49  ttakanen
   Implemented dump-command

   Revision 1.11  2002/10/08 20:28:46  ttakanen
   Implemented HELP command for hw-console

   Revision 1.10  2002/10/08 16:47:11  ttakanen
   Corrected compiler warnings

   Revision 1.9  2002/10/04 09:53:45  ttakanen
   *** empty log message ***

   Revision 1.8  2002/10/03 16:54:07  ttakanen
   *** empty log message ***

   Revision 1.7  2002/10/03 16:35:11  ttakanen
   New hw-console commands

   Revision 1.6  2002/10/03 15:54:18  ttakanen
   Added unbreak-command

   Revision 1.5  2002/10/03 15:29:22  tlilja
   Configured automake/conf to support (f)lex and yacc/bison.
   Had to do some renaming of files because of autoconf
   (hwcons.yy -> hwcons-lex.l)

   Revision 1.4  2002/10/03 14:14:28  ttakanen
   Regdump implementation.

   Revision 1.3  2002/10/03 13:05:31  javirta2
   *** empty log message ***

   Revision 1.2  2002/10/03 11:25:58  ttakanen
   Hardware console implementation skeleton works.

   Revision 1.1  2002/10/02 18:12:28  ttakanen
   *** empty log message ***


*/

/*
  Hardware console command parser.
*/

%{

#include <stdio.h>
#include "hwconsole.h"
#include <sys/types.h>

int yylex();
     
void yyerror (s)  /* Called by yyparse on error */
char *s;
{
    printf ("Invalid command (type 'help' for help)\n");
}
%}

%union {
	uint32_t intvalue;
	char *stringvalue;
}

%token CMDTERM
%token START
%token STEP
%token BREAK
%token QUIT
%token INTERRUPT
%token REGDUMP
%token REGWRITE
%token MEMWRITE
%token MEMREAD
%token HELP
%token UNBREAK
%token DUMP
%token POKE
%token TLBDUMP
%token GDB
%token BOOT
%token <intvalue> CPUREGISTER
%token <intvalue> CP0REGISTER
%token <intvalue> INTEGER32
%token COLON
%token <stringvalue> STRING
%token ERROR

%%
cmds:	cmd cmds {}
	| cmd {}
;

cmd:	command {}
;

command:	start
	| step
	| break
	| quit
	| interrupt
	| registerdump
	| regwrite
	| memwrite
	| memread
	| help
	| unbreak
	| dump
	| poke
	| tlbdump
	| boot
	| CMDTERM /*empty command */
;	

start:	START CMDTERM	{ command_start(); }
;
  
step:	STEP INTEGER32 CMDTERM	{ command_step($2); }
	| STEP CMDTERM 		{ command_step(1); }
;

break:	BREAK INTEGER32 CMDTERM	{ command_breakpoint($2); }
;

quit:	QUIT CMDTERM	{ command_quit(0); }
	| QUIT INTEGER32	{ command_quit($2); }
;

interrupt:	INTERRUPT INTEGER32 CMDTERM	{ command_interrupt($2, 0); }
	|	INTERRUPT INTEGER32 INTEGER32 CMDTERM	{ command_interrupt($2, $3); }
;

registerdump:	REGDUMP INTEGER32 CMDTERM	{ command_regdump($2); }
	| REGDUMP CMDTERM	{ command_regdump(0); }
;

regwrite:	REGWRITE CPUREGISTER INTEGER32 CMDTERM	{command_cpuregwrite(0,$2,$3);}
	|	REGWRITE CP0REGISTER INTEGER32 CMDTERM	{command_cp0regwrite(0,$2,$3);}
	|	REGWRITE INTEGER32 COLON CPUREGISTER INTEGER32 CMDTERM	{
		command_cpuregwrite($2, $4, $5);
		}
	|	REGWRITE INTEGER32 COLON CP0REGISTER INTEGER32 CMDTERM	{
		command_cp0regwrite($2, $4, $5);
		}
;
       
memwrite:	MEMWRITE INTEGER32 STRING CMDTERM	{ command_memwrite($2,$3); }
;

memread:	MEMREAD INTEGER32 INTEGER32 STRING CMDTERM	{
    command_memread($2,$3,$4); 
}
;

help:	HELP CMDTERM	{ command_help(NULL); }
    |   HELP HELP CMDTERM	{ command_help("help"); }
    |   HELP START CMDTERM	{ command_help("start"); }
    |   HELP STEP CMDTERM	{ command_help("step"); }
    |   HELP BREAK CMDTERM	{ command_help("break"); }
    |   HELP QUIT CMDTERM	{ command_help("quit"); }
    |   HELP INTERRUPT CMDTERM	{ command_help("interrupt"); }
    |   HELP REGDUMP CMDTERM	{ command_help("regdump"); }
    |   HELP REGWRITE CMDTERM	{ command_help("regwrite"); }
    |   HELP MEMWRITE CMDTERM	{ command_help("memwrite"); }
    |   HELP MEMREAD CMDTERM	{ command_help("memread"); }
    |   HELP UNBREAK CMDTERM	{ command_help("unbreak"); }
    |	HELP DUMP CMDTERM	{ command_help("dump"); }
    | 	HELP POKE CMDTERM	{ command_help("poke"); }
    | 	HELP BOOT CMDTERM	{ command_help("boot"); }
    |	HELP TLBDUMP CMDTERM	{ command_help("tlbdump"); }
    |   HELP GDB CMDTERM        { command_help("gdb"); }
;

unbreak:	UNBREAK CMDTERM	{ command_unbreak(); }
;

dump:	DUMP CMDTERM	{ command_dump(0xffffffff, 11); }
      | DUMP INTEGER32 CMDTERM	{ command_dump($2, 10); }
      | DUMP INTEGER32 INTEGER32 CMDTERM	{ command_dump($2, $3); }
      | DUMP CPUREGISTER CMDTERM	{ command_dump_regsource(0, $2, 10); }
      | DUMP INTEGER32 COLON CPUREGISTER CMDTERM	{ command_dump_regsource($2, $4, 10); }
      | DUMP CPUREGISTER INTEGER32 CMDTERM	{ command_dump_regsource(0, $2, $3); }
      | DUMP INTEGER32 COLON CPUREGISTER INTEGER32	{ command_dump_regsource($2, $4, $5); }
;

poke:	POKE INTEGER32 INTEGER32	{ command_poke($2, $3); }
;

tlbdump:	TLBDUMP	{ command_tlbdump(0); }
	|	TLBDUMP INTEGER32	{ command_tlbdump($2); }
;

boot:	BOOT STRING	{ command_boot($2, ""); }
	|	BOOT STRING STRING	{ command_boot($2, $3); }
;

%%


