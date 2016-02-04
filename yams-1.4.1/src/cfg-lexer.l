/* -*- fundamental -*- (for emacs) */

/* these are needed so that two lexers can be linked to same binary */
%option prefix="cfg_"
%option outfile="lex.yy.c"
%option yylineno
%option noyywrap
%{

#include "includes.h"
#include "cfg-parser.h"

%}

DIGIT		[0-9]
HEXDIGIT	[0-9a-fA-F]
BINARYDIGIT	[0-1]

%%


{DIGIT}{1,10}	{
    cfg_lval.intvalue=(unsigned int) atoi(yytext);
    return INTEGER32;
}

{DIGIT}{11,}	{
    printf("Decimal constant '%s' is too long (> 32 bits)\n",
           yytext);
    return ERROR;
}

"0x"{HEXDIGIT}{1,8}    {
    sscanf(yytext+2, "%x", &cfg_lval.intvalue);
    return INTEGER32;
}

"0x"{HEXDIGIT}{9,}  {
    printf("Hexadecimal constant '%s' is too long (> 32 bits)\n",
           yytext);
     return ERROR;
}

b{BINARYDIGIT}{1,32} {
			int i;
			/*printf("Integer: %s (%u)\n", yytext,
				strtol(yytext+1, NULL, 2));*/
			
			i = strlen(yytext+1) -1;
			cfg_lval.intvalue=0;
			while(*(++yytext) != 0) { 
				if(*yytext == '1') {
					cfg_lval.intvalue += 1 << i;
				}
				i--;
			}
			return INTEGER32;
}

b{BINARYDIGIT}{33,}	{
    printf("Binary constant '%s' too is too long (> 32 bits)\n",
           yytext);
    return ERROR;
}

Section|EndSection {
    if (!strcmp(yytext, "Section"))
        return SECTION;
    else
        return ENDSECTION;
}


clock-speed|memory|cpus|big-endian|little-endian|cpu-irq {
    if (!strcmp(yytext, "clock-speed"))
        return CLOCKSPEED;
    else if (!strcmp(yytext, "memory"))
        return MEMORY;
    else if (!strcmp(yytext, "cpu-irq"))
        return CPUIRQ;
    else if (!strcmp(yytext, "big-endian"))
        return BIGENDIAN;
    else if (!strcmp(yytext, "little-endian"))
        return LITTLEENDIAN;
    else
        return CPUS;
}

\"simulator\"|\"disk\"|\"tty\"|\"nic\"|\"plugin\" {
    if (!strcmp(yytext, "\"simulator\""))
        return SIMULATOR;
    else if (!strcmp(yytext, "\"disk\""))
        return DISK;
    else if (!strcmp(yytext, "\"tty\""))
        return TTY;
    else if (!strcmp(yytext, "\"nic\""))
        return NIC;
    else
        return PLUGIN;
}

vendor|irq {
    if (!strcmp(yytext, "vendor"))
        return VENDOR;
    else
        return IRQ;
}

sector-size|sectors|cylinders|rotation-time|seek-time|filename {
    if (!strcmp(yytext, "sector-size"))
        return SECTORSIZE;
    else if (!strcmp(yytext, "sectors"))
        return NUMSECTORS;
    else if (!strcmp(yytext, "cylinders"))
        return CYLINDERS;
    else if (!strcmp(yytext, "rotation-time"))
        return ROTTIME;
    else if (!strcmp(yytext, "seek-time"))
        return SEEKTIME;
    else
        return FILENAME;
}

mtu|mac|reliability|dma-delay|send-delay {
    if (!strcmp(yytext, "mtu"))
        return MTU;
    else if (!strcmp(yytext, "mac"))
        return MAC;
    else if (!strcmp(yytext, "reliability"))
        return RELIABILITY;
    else if (!strcmp(yytext, "dma-delay"))
        return DMADELAY;
    else
        return SENDDELAY;
}

unix-socket {
    return UNIXSOCKET;
}

listen {
    return SOCKETLISTEN;
}

tcp-host {
    return TCPHOST;
}

udp-host {
    return UDPHOST;
}

port {
    return PORT;
}

options {
    return OPTIONS;
}

async {
    return ASYNC;
}

#.*\n  /* ignore comments */

[ \t\n]+  /* eat up whitespace */



\"[^\n"]*\" {
    /* XXX: add '"' escaping */

    /* copy and remove leading and trailing " */
    cfg_lval.stringvalue=strdup(yytext+1);
    cfg_lval.stringvalue[strlen(cfg_lval.stringvalue)-1] = 0;
    return STRING;
}


[a-zA-Z0-9]+	{
    printf("Invalid keyword: '%s'\n", yytext);
    return ERROR;
}


.          {
    printf("%s\n", yytext);
    return ERROR;
}


%%

