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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    CMDTERM = 258,
    START = 259,
    STEP = 260,
    BREAK = 261,
    QUIT = 262,
    INTERRUPT = 263,
    REGDUMP = 264,
    REGWRITE = 265,
    MEMWRITE = 266,
    MEMREAD = 267,
    HELP = 268,
    UNBREAK = 269,
    DUMP = 270,
    POKE = 271,
    TLBDUMP = 272,
    GDB = 273,
    BOOT = 274,
    CPUREGISTER = 275,
    CP0REGISTER = 276,
    INTEGER32 = 277,
    COLON = 278,
    STRING = 279,
    ERROR = 280
  };
#endif
/* Tokens.  */
#define CMDTERM 258
#define START 259
#define STEP 260
#define BREAK 261
#define QUIT 262
#define INTERRUPT 263
#define REGDUMP 264
#define REGWRITE 265
#define MEMWRITE 266
#define MEMREAD 267
#define HELP 268
#define UNBREAK 269
#define DUMP 270
#define POKE 271
#define TLBDUMP 272
#define GDB 273
#define BOOT 274
#define CPUREGISTER 275
#define CP0REGISTER 276
#define INTEGER32 277
#define COLON 278
#define STRING 279
#define ERROR 280

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 124 "hwcons.y" /* yacc.c:1909  */

	uint32_t intvalue;
	char *stringvalue;

#line 109 "hwcons.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
