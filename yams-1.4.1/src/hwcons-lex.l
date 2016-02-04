/* -*- fundamental -*- (for emacs) */

%option noyywrap
%{

#include <math.h>

#include "includes.h"
#include "hwconsole.h"
#include "hwcons.h"
#include "cpuregs.h"
#include <stdio.h>

#define YY_INPUT(buf, result, max_size) { \
	if(*hwc_lexer_input_ptr == 0) \
		result = YY_NULL; \
	else { \
		result = 1; \
	        buf[0] = *hwc_lexer_input_ptr; \
		hwc_lexer_input_ptr++; \
	} \
    }

void reset_lexer() {
	YY_FLUSH_BUFFER;
}
%}

DIGIT		[0-9]
HEXDIGIT	[0-9a-fA-F]
BINARYDIGIT	[0-1]

%%

{DIGIT}{1,10}	{
			/*printf("Integer: %s (%u)\n", yytext, atoi(yytext));*/
			yylval.intvalue=(unsigned int) atoi(yytext);
			return INTEGER32;
		}

{DIGIT}{11,}	{
			printf("Decimal constant '%s' is too long (> 32 bits)\n",
				yytext);
			return ERROR;
		}

#{HEXDIGIT}{1,8}    {
			/*printf("Integer: %s (%u)\n", yytext,
				strtol(yytext+1, NULL, 16));*/
			sscanf(yytext+1, "%x", &yylval.intvalue);
			return INTEGER32;
		}
"0x"{HEXDIGIT}{1,8}    {
			/*printf("Integer: %s (%u)\n", yytext,
				strtol(yytext+2, NULL, 16));*/
			sscanf(yytext+2, "%x", &yylval.intvalue);
			return INTEGER32;
		}

("#"|"0x"){HEXDIGIT}{9,}	{
			printf("Hexadecimal constant '%s' is too long (> 32 bits)\n",
				yytext);
			return ERROR;
		}

b{BINARYDIGIT}{1,32} {
			int i;
			/*printf("Integer: %s (%u)\n", yytext,
				strtol(yytext+1, NULL, 2));*/
			
			i = strlen(yytext+1) -1;
			yylval.intvalue=0;
			while(*(++yytext) != 0) { 
				if(*yytext == '1') {
					yylval.intvalue += 1 << i;
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

start|s		{
			/*printf("Start command: %s\n", yytext);*/
			return START;
		}

step|t		{
			/*printf("Step command: %s\n", yytext);*/
			return STEP;
		}

break|b		{
			/*printf("Break command: %s\n", yytext);*/
			return BREAK;
		}

quit|q		{
			/*printf("Quit command: %s\n", yytext);*/
			return QUIT;
		}

interrupt|i	{
			/*printf("Interrupt command: %s\n", yytext);*/
			return INTERRUPT;
		}

regdump|r	{
			/*printf("Registerdump command: %s\n", yytext);*/
			return REGDUMP;
		}

regwrite|w	{
			/*printf("Regwrite command: %s\n", yytext);*/
			return REGWRITE;
		}

memwrite|l	{
			/*printf("Memwrite command: %s\n", yytext);*/
			return MEMWRITE;
		}

memread|m	{
			/*printf("Memread command: %s\n", yytext);*/
			return MEMREAD;
		}

help|h		{
			/*printf("Help command: %s\n", yytext); */
			return HELP;
		}

unbreak|u	{
			/*printf("Unbreak command: %s\n", yytext);*/
			return UNBREAK;
		}

dump|d	{
		/*printf("Dump command: %s\n", yytext); */
		return DUMP;
	}

poke|p	{
		/*printf("Poke command: %s\n", yytext); */
		return POKE;
	}

tlbdump {
		/*printf("Tlbdump command: %s\n", yytext); */
		return TLBDUMP;
}

boot|b {
		/*printf("Boot command: %s\n", yytext); */
		return BOOT;
}

gdb|g {
                return GDB;
}

r[0-9]|r[1-2][0-9]|r30|r31|HI|hi|LO|lo|pc|PC	{
			/*printf("CPU register: %s\n", yytext);*/
			yylval.intvalue= cpu_register_to_number(yytext);
			return CPUREGISTER;
		}

zero|at|v[0-1]|a[0-3]|t[0-7]|s[0-7]|t8|t9|k0|k1|gp|sp|fp|ra	{
			/*printf("CPU register: %s\n", yytext);*/
			yylval.intvalue= cpu_register_to_number(yytext);
			return CPUREGISTER;
		}

cp0r[0-9]|cp0r[1-2][0-9]|cp0r30|cp0r31		{
			/*printf("CP0 register: %s\n", yytext);*/
			yylval.intvalue=cp0_register_to_number(yytext); 
			return CP0REGISTER;
		}

Index|Random|EntLo0|EntLo1|Contxt|PgMask|Wired|BadVAd|Count|EntrHi|Compar|Status|Cause|EPC|PRId|Conf0|Conf1|LLAddr|WtchLo|WtchHi|Debug|DEPC|PrfCnt|ErrCtl|CacheE|CP0Lo|CP0Hi|ErrEPC|DESAVE	{
			/*printf("CP0 register: %s\n", yytext);*/
			yylval.intvalue=cp0_register_to_number(yytext); 
			return CP0REGISTER;
		}


":"		{
			/*printf("COLON\n");*/
			return COLON;
		}

"\n"|";"	{
			/*printf("CMDTERM\n");*/
			return CMDTERM;
		}

[ \t]+	{ /* eat up whitespace */ }

\"[^\n"]*\"	{ 
			/*printf("String: '%s'\n", yytext);*/
			yylval.stringvalue=strdup(yytext+1);
			/* remove trailing '"' */
			yylval.stringvalue[strlen(yylval.stringvalue)-1] = 0;
			return STRING;
	}

[a-zA-Z0-9]+	{
			printf("Invalid keyword: '%s'\n", yytext);
			return ERROR;
		}

.	{
		/*printf("Invalid character '%s'\n", yytext);*/
		return ERROR;
	}

%%
