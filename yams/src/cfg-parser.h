/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_CFG_Y_TAB_H_INCLUDED
# define YY_CFG_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int cfg_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    SECTION = 258,
    ENDSECTION = 259,
    SIMULATOR = 260,
    DISK = 261,
    TTY = 262,
    NIC = 263,
    PLUGIN = 264,
    CLOCKSPEED = 265,
    MEMORY = 266,
    CPUS = 267,
    CPUIRQ = 268,
    BIGENDIAN = 269,
    LITTLEENDIAN = 270,
    VENDOR = 271,
    IRQ = 272,
    SECTORSIZE = 273,
    CYLINDERS = 274,
    NUMSECTORS = 275,
    ROTTIME = 276,
    SEEKTIME = 277,
    FILENAME = 278,
    SOCKET = 279,
    SENDDELAY = 280,
    MTU = 281,
    MAC = 282,
    RELIABILITY = 283,
    DMADELAY = 284,
    OPTIONS = 285,
    ASYNC = 286,
    UNIXSOCKET = 287,
    TCPHOST = 288,
    UDPHOST = 289,
    PORT = 290,
    SOCKETLISTEN = 291,
    INTEGER32 = 292,
    STRING = 293,
    ERROR = 294
  };
#endif
/* Tokens.  */
#define SECTION 258
#define ENDSECTION 259
#define SIMULATOR 260
#define DISK 261
#define TTY 262
#define NIC 263
#define PLUGIN 264
#define CLOCKSPEED 265
#define MEMORY 266
#define CPUS 267
#define CPUIRQ 268
#define BIGENDIAN 269
#define LITTLEENDIAN 270
#define VENDOR 271
#define IRQ 272
#define SECTORSIZE 273
#define CYLINDERS 274
#define NUMSECTORS 275
#define ROTTIME 276
#define SEEKTIME 277
#define FILENAME 278
#define SOCKET 279
#define SENDDELAY 280
#define MTU 281
#define MAC 282
#define RELIABILITY 283
#define DMADELAY 284
#define OPTIONS 285
#define ASYNC 286
#define UNIXSOCKET 287
#define TCPHOST 288
#define UDPHOST 289
#define PORT 290
#define SOCKETLISTEN 291
#define INTEGER32 292
#define STRING 293
#define ERROR 294

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 20 "cfg-parser.y" /* yacc.c:1909  */

	uint32_t intvalue;
	char *stringvalue;

#line 137 "cfg-parser.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE cfg_lval;

int cfg_parse (void);

#endif /* !YY_CFG_Y_TAB_H_INCLUDED  */
