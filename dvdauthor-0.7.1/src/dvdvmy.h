/* A Bison parser, made by GNU Bison 3.7.3.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_DVDVM_DVDVMY_H_INCLUDED
# define YY_DVDVM_DVDVMY_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int dvdvmdebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    NUM_TOK = 258,                 /* NUM_TOK  */
    G_TOK = 259,                   /* G_TOK  */
    S_TOK = 260,                   /* S_TOK  */
    ID_TOK = 261,                  /* ID_TOK  */
    ANGLE_TOK = 262,               /* ANGLE_TOK  */
    AUDIO_TOK = 263,               /* AUDIO_TOK  */
    BREAK_TOK = 264,               /* BREAK_TOK  */
    BUTTON_TOK = 265,              /* BUTTON_TOK  */
    CALL_TOK = 266,                /* CALL_TOK  */
    CELL_TOK = 267,                /* CELL_TOK  */
    CHAPTER_TOK = 268,             /* CHAPTER_TOK  */
    CLOSEBRACE_TOK = 269,          /* CLOSEBRACE_TOK  */
    CLOSEPAREN_TOK = 270,          /* CLOSEPAREN_TOK  */
    COUNTER_TOK = 271,             /* COUNTER_TOK  */
    ELSE_TOK = 272,                /* ELSE_TOK  */
    ENTRY_TOK = 273,               /* ENTRY_TOK  */
    EXIT_TOK = 274,                /* EXIT_TOK  */
    FPC_TOK = 275,                 /* FPC_TOK  */
    GOTO_TOK = 276,                /* GOTO_TOK  */
    IF_TOK = 277,                  /* IF_TOK  */
    JUMP_TOK = 278,                /* JUMP_TOK  */
    MENU_TOK = 279,                /* MENU_TOK  */
    NEXT_TOK = 280,                /* NEXT_TOK  */
    OPENBRACE_TOK = 281,           /* OPENBRACE_TOK  */
    OPENPAREN_TOK = 282,           /* OPENPAREN_TOK  */
    PGC_TOK = 283,                 /* PGC_TOK  */
    PREV_TOK = 284,                /* PREV_TOK  */
    PROGRAM_TOK = 285,             /* PROGRAM_TOK  */
    PTT_TOK = 286,                 /* PTT_TOK  */
    REGION_TOK = 287,              /* REGION_TOK  */
    RESUME_TOK = 288,              /* RESUME_TOK  */
    RND_TOK = 289,                 /* RND_TOK  */
    ROOT_TOK = 290,                /* ROOT_TOK  */
    SET_TOK = 291,                 /* SET_TOK  */
    SUBTITLE_TOK = 292,            /* SUBTITLE_TOK  */
    TAIL_TOK = 293,                /* TAIL_TOK  */
    TITLE_TOK = 294,               /* TITLE_TOK  */
    TITLESET_TOK = 295,            /* TITLESET_TOK  */
    TOP_TOK = 296,                 /* TOP_TOK  */
    UP_TOK = 297,                  /* UP_TOK  */
    VMGM_TOK = 298,                /* VMGM_TOK  */
    AMGM_TOK = 299,                /* AMGM_TOK  */
    GROUP_TOK = 300,               /* GROUP_TOK  */
    TRACK_TOK = 301,               /* TRACK_TOK  */
    _OR_TOK = 302,                 /* _OR_TOK  */
    XOR_TOK = 303,                 /* XOR_TOK  */
    LOR_TOK = 304,                 /* LOR_TOK  */
    BOR_TOK = 305,                 /* BOR_TOK  */
    _AND_TOK = 306,                /* _AND_TOK  */
    LAND_TOK = 307,                /* LAND_TOK  */
    BAND_TOK = 308,                /* BAND_TOK  */
    NOT_TOK = 309,                 /* NOT_TOK  */
    EQ_TOK = 310,                  /* EQ_TOK  */
    NE_TOK = 311,                  /* NE_TOK  */
    GE_TOK = 312,                  /* GE_TOK  */
    GT_TOK = 313,                  /* GT_TOK  */
    LE_TOK = 314,                  /* LE_TOK  */
    LT_TOK = 315,                  /* LT_TOK  */
    ADD_TOK = 316,                 /* ADD_TOK  */
    SUB_TOK = 317,                 /* SUB_TOK  */
    MUL_TOK = 318,                 /* MUL_TOK  */
    DIV_TOK = 319,                 /* DIV_TOK  */
    MOD_TOK = 320,                 /* MOD_TOK  */
    ADDSET_TOK = 321,              /* ADDSET_TOK  */
    SUBSET_TOK = 322,              /* SUBSET_TOK  */
    MULSET_TOK = 323,              /* MULSET_TOK  */
    DIVSET_TOK = 324,              /* DIVSET_TOK  */
    MODSET_TOK = 325,              /* MODSET_TOK  */
    ANDSET_TOK = 326,              /* ANDSET_TOK  */
    ORSET_TOK = 327,               /* ORSET_TOK  */
    XORSET_TOK = 328,              /* XORSET_TOK  */
    SEMICOLON_TOK = 329,           /* SEMICOLON_TOK  */
    COLON_TOK = 330,               /* COLON_TOK  */
    ERROR_TOK = 331                /* ERROR_TOK  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 93 "dvdvmy.y"

    unsigned int int_val;
    char *str_val;
    struct vm_statement *statement;

#line 146 "dvdvmy.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE dvdvmlval;

int dvdvmparse (void);

#endif /* !YY_DVDVM_DVDVMY_H_INCLUDED  */
