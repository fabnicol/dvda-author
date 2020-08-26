/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         dvdvmparse
#define yylex           dvdvmlex
#define yyerror         dvdvmerror
#define yydebug         dvdvmdebug
#define yynerrs         dvdvmnerrs
#define yylval          dvdvmlval
#define yychar          dvdvmchar

/* First part of user prologue.  */
#line 1 "dvdvmy.y"


/*
 * Copyright (C) 2002 Scott Smith (trckjunky@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 */

#include "compat.h" /* needed for bool */
#include "dvdvm.h"


#define YYERROR_VERBOSE


#line 106 "dvdvmy.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_DVDVM_DVDVMY_H_INCLUDED
# define YY_DVDVM_DVDVMY_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int dvdvmdebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    NUM_TOK = 258,
    G_TOK = 259,
    S_TOK = 260,
    ID_TOK = 261,
    ANGLE_TOK = 262,
    AUDIO_TOK = 263,
    BREAK_TOK = 264,
    BUTTON_TOK = 265,
    CALL_TOK = 266,
    CELL_TOK = 267,
    CHAPTER_TOK = 268,
    CLOSEBRACE_TOK = 269,
    CLOSEPAREN_TOK = 270,
    COUNTER_TOK = 271,
    ELSE_TOK = 272,
    ENTRY_TOK = 273,
    EXIT_TOK = 274,
    FPC_TOK = 275,
    GOTO_TOK = 276,
    IF_TOK = 277,
    JUMP_TOK = 278,
    MENU_TOK = 279,
    NEXT_TOK = 280,
    OPENBRACE_TOK = 281,
    OPENPAREN_TOK = 282,
    PGC_TOK = 283,
    PREV_TOK = 284,
    PROGRAM_TOK = 285,
    PTT_TOK = 286,
    REGION_TOK = 287,
    RESUME_TOK = 288,
    RND_TOK = 289,
    ROOT_TOK = 290,
    SET_TOK = 291,
    SUBTITLE_TOK = 292,
    TAIL_TOK = 293,
    TITLE_TOK = 294,
    TITLESET_TOK = 295,
    TOP_TOK = 296,
    UP_TOK = 297,
    VMGM_TOK = 298,
    AMGM_TOK = 299,
    GROUP_TOK = 300,
    TRACK_TOK = 301,
    _OR_TOK = 302,
    XOR_TOK = 303,
    LOR_TOK = 304,
    BOR_TOK = 305,
    _AND_TOK = 306,
    LAND_TOK = 307,
    BAND_TOK = 308,
    NOT_TOK = 309,
    EQ_TOK = 310,
    NE_TOK = 311,
    GE_TOK = 312,
    GT_TOK = 313,
    LE_TOK = 314,
    LT_TOK = 315,
    ADD_TOK = 316,
    SUB_TOK = 317,
    MUL_TOK = 318,
    DIV_TOK = 319,
    MOD_TOK = 320,
    ADDSET_TOK = 321,
    SUBSET_TOK = 322,
    MULSET_TOK = 323,
    DIVSET_TOK = 324,
    MODSET_TOK = 325,
    ANDSET_TOK = 326,
    ORSET_TOK = 327,
    XORSET_TOK = 328,
    SEMICOLON_TOK = 329,
    COLON_TOK = 330,
    ERROR_TOK = 331
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 93 "dvdvmy.y"

    unsigned int int_val;
    char *str_val;
    struct vm_statement *statement;

#line 241 "dvdvmy.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE dvdvmlval;

int dvdvmparse (void);

#endif /* !YY_DVDVM_DVDVMY_H_INCLUDED  */



#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  49
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   433

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  77
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  19
/* YYNRULES -- Number of rules.  */
#define YYNRULES  99
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  197

#define YYUNDEFTOK  2
#define YYMAXUTOK   331


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   104,   104,   109,   112,   118,   121,   124,   128,   133,
     138,   143,   147,   150,   153,   158,   165,   168,   171,   177,
     183,   190,   193,   196,   199,   202,   205,   208,   211,   214,
     221,   228,   233,   240,   245,   253,   262,   269,   278,   283,
     288,   293,   298,   303,   308,   313,   318,   323,   328,   335,
     342,   347,   358,   361,   364,   367,   370,   373,   376,   381,
     384,   389,   392,   397,   400,   403,   406,   409,   412,   415,
     418,   421,   424,   427,   434,   437,   440,   443,   446,   449,
     452,   455,   458,   461,   464,   467,   474,   477,   482,   488,
     491,   494,   497,   500,   503,   506,   509,   514,   524,   527
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NUM_TOK", "G_TOK", "S_TOK", "ID_TOK",
  "ANGLE_TOK", "AUDIO_TOK", "BREAK_TOK", "BUTTON_TOK", "CALL_TOK",
  "CELL_TOK", "CHAPTER_TOK", "CLOSEBRACE_TOK", "CLOSEPAREN_TOK",
  "COUNTER_TOK", "ELSE_TOK", "ENTRY_TOK", "EXIT_TOK", "FPC_TOK",
  "GOTO_TOK", "IF_TOK", "JUMP_TOK", "MENU_TOK", "NEXT_TOK",
  "OPENBRACE_TOK", "OPENPAREN_TOK", "PGC_TOK", "PREV_TOK", "PROGRAM_TOK",
  "PTT_TOK", "REGION_TOK", "RESUME_TOK", "RND_TOK", "ROOT_TOK", "SET_TOK",
  "SUBTITLE_TOK", "TAIL_TOK", "TITLE_TOK", "TITLESET_TOK", "TOP_TOK",
  "UP_TOK", "VMGM_TOK", "AMGM_TOK", "GROUP_TOK", "TRACK_TOK", "_OR_TOK",
  "XOR_TOK", "LOR_TOK", "BOR_TOK", "_AND_TOK", "LAND_TOK", "BAND_TOK",
  "NOT_TOK", "EQ_TOK", "NE_TOK", "GE_TOK", "GT_TOK", "LE_TOK", "LT_TOK",
  "ADD_TOK", "SUB_TOK", "MUL_TOK", "DIV_TOK", "MOD_TOK", "ADDSET_TOK",
  "SUBSET_TOK", "MULSET_TOK", "DIVSET_TOK", "MODSET_TOK", "ANDSET_TOK",
  "ORSET_TOK", "XORSET_TOK", "SEMICOLON_TOK", "COLON_TOK", "ERROR_TOK",
  "$accept", "finalparse", "statements", "statement", "jtsl", "jgrl",
  "jtml", "jcl", "jumpstatement", "resumel", "callstatement", "reg",
  "regornum", "expression", "boolexpr", "regorcounter", "setstatement",
  "ifstatement", "ifelsestatement", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331
};
# endif

#define YYPACT_NINF (-61)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     182,   -61,   -61,   -60,   -61,   -61,   -46,   -61,    56,    20,
     -39,    57,    -1,   188,   182,   -61,     6,   -61,    82,   -61,
     182,   -61,   -61,   154,    81,   -61,   102,   -61,   -61,   -61,
     119,   -61,   -61,    -2,   -61,   -61,    49,    84,     7,     1,
      -5,     4,     9,    96,   122,    -2,    -2,   114,   -61,   -61,
     -61,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     182,   -61,   -61,    87,   123,   126,   118,   -61,   -61,    84,
     108,    84,   -61,   -61,   368,     2,    58,    66,    80,    83,
      90,    97,    98,   107,   110,   120,   121,   125,   132,   -61,
     118,   155,   -61,   175,   196,   215,   234,   253,   272,   291,
     310,   329,   348,   -61,   -61,   203,   -61,   -61,   158,   163,
      86,    46,   175,   -61,   175,   175,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     182,    84,    84,    84,    84,   -61,   -61,   -61,   -61,   -61,
     -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   161,   -61,
     105,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,
     -61,   -61,   -61,   -61,   -61,   -61,   -61,   194,   162,   -61,
     -61,   112,    -6,    -6,    -6,    45,    45,   -23,   -23,   -23,
     -23,   -23,   -23,    39,    39,   -61,   -61,   -61,   -61,    61,
      61,   -61,   -61,   -61,   -61,   -61,   -61
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    52,    53,     0,    56,    54,     0,    57,    18,     0,
       0,     0,     0,    18,     0,    58,     0,    55,     0,     2,
       3,     5,     6,    86,     0,    12,    98,    14,    10,    11,
       0,    16,    17,    31,    87,     7,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    31,    31,     0,     8,     1,
       4,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    15,    28,    21,     0,     0,    33,     9,    60,     0,
       0,     0,    59,    62,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    19,
      33,     0,    13,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    99,    20,     0,    29,    30,     0,    50,
       0,     0,     0,    85,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,    38,    39,    45,    42,
      48,    44,    40,    46,    43,    37,    41,    47,     0,    36,
       0,    89,    90,    91,    92,    93,    94,    95,    96,    88,
      26,    25,    27,    23,    24,    22,    32,     0,     0,    61,
      74,     0,    71,    72,    69,    70,    68,    75,    76,    77,
      78,    79,    80,    63,    64,    65,    66,    67,    97,    83,
      81,    84,    82,    34,    49,    51,    73
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -61,   -61,    32,   -49,   224,   -61,    69,   149,   -61,   -61,
     -61,     0,   -61,   -50,   -48,   -61,   -61,   -61,   -61
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    18,    19,    20,    33,    46,    66,   109,    21,   168,
      22,    72,    73,    74,    75,    24,    25,    26,    27
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      23,    94,    95,    96,    97,    98,    99,   100,   101,   102,
      76,   103,    86,    78,    23,    28,    83,   130,    62,   110,
      23,   111,    63,   113,    34,   115,    37,   116,    29,    79,
     118,    80,    84,    81,    85,    35,    82,    64,   125,   126,
     127,   128,   129,   150,    65,   117,    47,   118,    77,   131,
      87,   132,    50,   133,   134,   125,   126,   127,   128,   129,
      23,   170,   171,    36,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
      48,   188,    49,   189,   190,   191,   192,    68,     1,     2,
     104,     4,     5,   131,     7,   132,    30,   133,   134,    31,
      32,   169,   127,   128,   129,   105,   125,   126,   127,   128,
     129,    69,   133,   134,    90,    91,    15,    59,    70,    60,
     169,    17,    61,    67,    88,    89,   106,   196,    92,   107,
      23,   108,   135,   114,   115,   112,   116,   117,    71,   118,
     136,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   114,   115,   137,   116,   117,   138,   118,   114,
     115,   166,   116,   117,   139,   118,   125,   126,   127,   128,
     129,   140,   141,   125,   126,   127,   128,   129,    68,     1,
       2,   142,     4,     5,   143,     7,     1,     2,     3,     4,
       5,     6,     7,     8,   144,   145,   167,   194,     9,   146,
      38,    10,    93,    11,    12,    13,   147,    15,    14,    70,
     160,   161,    17,    39,    15,    16,    40,    41,    42,    17,
      51,    52,    53,    54,    55,    56,    57,    58,    30,   149,
      43,    31,    32,    44,   162,   193,   195,    45,   163,   148,
     164,     0,   165,   114,   115,     0,   116,   117,     0,   118,
       0,     0,     0,     0,     0,     0,     0,   125,   126,   127,
     128,   129,   114,   115,     0,   116,   117,     0,   118,     0,
     151,     0,     0,     0,     0,     0,   125,   126,   127,   128,
     129,   114,   115,     0,   116,   117,     0,   118,     0,   152,
       0,     0,     0,     0,     0,   125,   126,   127,   128,   129,
     114,   115,     0,   116,   117,     0,   118,     0,   153,     0,
       0,     0,     0,     0,   125,   126,   127,   128,   129,   114,
     115,     0,   116,   117,     0,   118,     0,   154,     0,     0,
       0,     0,     0,   125,   126,   127,   128,   129,   114,   115,
       0,   116,   117,     0,   118,     0,   155,     0,     0,     0,
       0,     0,   125,   126,   127,   128,   129,   114,   115,     0,
     116,   117,     0,   118,     0,   156,     0,     0,     0,     0,
       0,   125,   126,   127,   128,   129,   114,   115,     0,   116,
     117,     0,   118,     0,   157,     0,     0,     0,     0,     0,
     125,   126,   127,   128,   129,   114,   115,     0,   116,   117,
       0,   118,     0,   158,     0,     0,     0,     0,     0,   125,
     126,   127,   128,   129,     0,   114,   115,     0,   116,   117,
       0,   118,   159,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129
};

static const yytype_int16 yycheck[] =
{
       0,    51,    52,    53,    54,    55,    56,    57,    58,    59,
       3,    60,     3,    12,    14,    75,    12,    15,    20,    69,
      20,    69,    24,    71,     4,    48,    27,    50,    74,    28,
      53,    30,    28,    38,    30,    74,    41,    39,    61,    62,
      63,    64,    65,    93,    46,    51,    14,    53,    41,    47,
      41,    49,    20,    51,    52,    61,    62,    63,    64,    65,
      60,    15,   112,     6,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
      74,   130,     0,   131,   132,   133,   134,     3,     4,     5,
       3,     7,     8,    47,    10,    49,    40,    51,    52,    43,
      44,    15,    63,    64,    65,    18,    61,    62,    63,    64,
      65,    27,    51,    52,    45,    46,    32,    36,    34,    17,
      15,    37,     3,    74,    28,     3,     3,    15,    14,     3,
     130,    13,    74,    47,    48,    27,    50,    51,    54,    53,
      74,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    47,    48,    74,    50,    51,    74,    53,    47,
      48,     3,    50,    51,    74,    53,    61,    62,    63,    64,
      65,    74,    74,    61,    62,    63,    64,    65,     3,     4,
       5,    74,     7,     8,    74,    10,     4,     5,     6,     7,
       8,     9,    10,    11,    74,    74,    33,     3,    16,    74,
      12,    19,    27,    21,    22,    23,    74,    32,    26,    34,
       7,     8,    37,    25,    32,    33,    28,    29,    30,    37,
      66,    67,    68,    69,    70,    71,    72,    73,    40,    74,
      42,    43,    44,    45,    31,    74,    74,    13,    35,    90,
      37,    -1,    39,    47,    48,    -1,    50,    51,    -1,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    62,    63,
      64,    65,    47,    48,    -1,    50,    51,    -1,    53,    -1,
      74,    -1,    -1,    -1,    -1,    -1,    61,    62,    63,    64,
      65,    47,    48,    -1,    50,    51,    -1,    53,    -1,    74,
      -1,    -1,    -1,    -1,    -1,    61,    62,    63,    64,    65,
      47,    48,    -1,    50,    51,    -1,    53,    -1,    74,    -1,
      -1,    -1,    -1,    -1,    61,    62,    63,    64,    65,    47,
      48,    -1,    50,    51,    -1,    53,    -1,    74,    -1,    -1,
      -1,    -1,    -1,    61,    62,    63,    64,    65,    47,    48,
      -1,    50,    51,    -1,    53,    -1,    74,    -1,    -1,    -1,
      -1,    -1,    61,    62,    63,    64,    65,    47,    48,    -1,
      50,    51,    -1,    53,    -1,    74,    -1,    -1,    -1,    -1,
      -1,    61,    62,    63,    64,    65,    47,    48,    -1,    50,
      51,    -1,    53,    -1,    74,    -1,    -1,    -1,    -1,    -1,
      61,    62,    63,    64,    65,    47,    48,    -1,    50,    51,
      -1,    53,    -1,    74,    -1,    -1,    -1,    -1,    -1,    61,
      62,    63,    64,    65,    -1,    47,    48,    -1,    50,    51,
      -1,    53,    74,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     4,     5,     6,     7,     8,     9,    10,    11,    16,
      19,    21,    22,    23,    26,    32,    33,    37,    78,    79,
      80,    85,    87,    88,    92,    93,    94,    95,    75,    74,
      40,    43,    44,    81,     4,    74,     6,    27,    12,    25,
      28,    29,    30,    42,    45,    81,    82,    79,    74,     0,
      79,    66,    67,    68,    69,    70,    71,    72,    73,    36,
      17,     3,    20,    24,    39,    46,    83,    74,     3,    27,
      34,    54,    88,    89,    90,    91,     3,    41,    12,    28,
      30,    38,    41,    12,    28,    30,     3,    41,    28,     3,
      83,    83,    14,    27,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    80,     3,    18,     3,     3,    13,    84,
      90,    91,    27,    91,    47,    48,    50,    51,    53,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      15,    47,    49,    51,    52,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    84,    74,
      90,    74,    74,    74,    74,    74,    74,    74,    74,    74,
       7,     8,    31,    35,    37,    39,     3,    33,    86,    15,
      15,    90,    90,    90,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    90,    90,    90,    90,    80,    91,
      91,    91,    91,    74,     3,    74,    15
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    77,    78,    79,    79,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    81,    81,    81,    81,    82,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    84,    84,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    85,    85,    86,
      86,    87,    88,    88,    88,    88,    88,    88,    88,    89,
      89,    90,    90,    90,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    91,    91,    91,    91,    91,    91,
      91,    91,    91,    91,    91,    91,    92,    92,    93,    93,
      93,    93,    93,    93,    93,    93,    93,    94,    95,    95
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     2,     2,     3,
       2,     2,     1,     3,     1,     2,     1,     1,     0,     2,
       2,     1,     3,     3,     3,     3,     3,     3,     1,     2,
       2,     0,     2,     0,     5,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     2,
       0,     6,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     4,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     1,     2,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     5,     1,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2:
#line 104 "dvdvmy.y"
                       {
    dvd_vm_parsed_cmd=(yyval.statement);
}
#line 1608 "dvdvmy.c"
    break;

  case 3:
#line 109 "dvdvmy.y"
                      {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1616 "dvdvmy.c"
    break;

  case 4:
#line 112 "dvdvmy.y"
                       {
    (yyval.statement)=(yyvsp[-1].statement);
    (yyval.statement)->next=(yyvsp[0].statement);
}
#line 1625 "dvdvmy.c"
    break;

  case 5:
#line 118 "dvdvmy.y"
                         {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1633 "dvdvmy.c"
    break;

  case 6:
#line 121 "dvdvmy.y"
                {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1641 "dvdvmy.c"
    break;

  case 7:
#line 124 "dvdvmy.y"
                         {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_EXIT;
}
#line 1650 "dvdvmy.c"
    break;

  case 8:
#line 128 "dvdvmy.y"
                           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=16;
}
#line 1660 "dvdvmy.c"
    break;

  case 9:
#line 133 "dvdvmy.y"
                                {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_GOTO;
    (yyval.statement)->s1=(yyvsp[-1].str_val);
}
#line 1670 "dvdvmy.c"
    break;

  case 10:
#line 138 "dvdvmy.y"
                   {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LABEL;
    (yyval.statement)->s1=(yyvsp[-1].str_val);
}
#line 1680 "dvdvmy.c"
    break;

  case 11:
#line 143 "dvdvmy.y"
                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_BREAK;
}
#line 1689 "dvdvmy.c"
    break;

  case 12:
#line 147 "dvdvmy.y"
               {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1697 "dvdvmy.c"
    break;

  case 13:
#line 150 "dvdvmy.y"
                                          {
    (yyval.statement)=(yyvsp[-1].statement);
}
#line 1705 "dvdvmy.c"
    break;

  case 14:
#line 153 "dvdvmy.y"
                  {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1713 "dvdvmy.c"
    break;

  case 15:
#line 158 "dvdvmy.y"
                           {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
      {
        yyerror("titleset number out of range");
      } /*if*/
    (yyval.int_val)=((yyvsp[0].int_val))+1;
}
#line 1725 "dvdvmy.c"
    break;

  case 16:
#line 165 "dvdvmy.y"
           {
    (yyval.int_val)=1;
}
#line 1733 "dvdvmy.c"
    break;

  case 17:
#line 168 "dvdvmy.y"
           {
    (yyval.int_val)=1;
}
#line 1741 "dvdvmy.c"
    break;

  case 18:
#line 171 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1749 "dvdvmy.c"
    break;

  case 19:
#line 177 "dvdvmy.y"
                        {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1757 "dvdvmy.c"
    break;

  case 20:
#line 183 "dvdvmy.y"
                       {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
      {
        yyerror("menu number out of range");
      } /*if*/
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1769 "dvdvmy.c"
    break;

  case 21:
#line 190 "dvdvmy.y"
           {
    (yyval.int_val)=120; // default entry
}
#line 1777 "dvdvmy.c"
    break;

  case 22:
#line 193 "dvdvmy.y"
                               {
    (yyval.int_val)=122;
}
#line 1785 "dvdvmy.c"
    break;

  case 23:
#line 196 "dvdvmy.y"
                              {
    (yyval.int_val)=123;
}
#line 1793 "dvdvmy.c"
    break;

  case 24:
#line 199 "dvdvmy.y"
                                  {
    (yyval.int_val)=124;
}
#line 1801 "dvdvmy.c"
    break;

  case 25:
#line 202 "dvdvmy.y"
                               {
    (yyval.int_val)=125;
}
#line 1809 "dvdvmy.c"
    break;

  case 26:
#line 205 "dvdvmy.y"
                               {
    (yyval.int_val)=126;
}
#line 1817 "dvdvmy.c"
    break;

  case 27:
#line 208 "dvdvmy.y"
                             {
    (yyval.int_val)=127;
}
#line 1825 "dvdvmy.c"
    break;

  case 28:
#line 211 "dvdvmy.y"
          {
    (yyval.int_val)=121;
}
#line 1833 "dvdvmy.c"
    break;

  case 29:
#line 214 "dvdvmy.y"
                    {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
      {
        yyerror("title number out of range");
      } /*if*/
    (yyval.int_val)=((yyvsp[0].int_val))|128;
}
#line 1845 "dvdvmy.c"
    break;

  case 30:
#line 221 "dvdvmy.y"
                    {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
  {
    yyerror("title number out of range");
  }
    (yyval.int_val)=((yyvsp[0].int_val))|128;
}
#line 1857 "dvdvmy.c"
    break;

  case 31:
#line 228 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1865 "dvdvmy.c"
    break;

  case 32:
#line 233 "dvdvmy.y"
                         {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 65535)
      {
        yyerror("chapter number out of range");
      } /*if*/
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1877 "dvdvmy.c"
    break;

  case 33:
#line 240 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1885 "dvdvmy.c"
    break;

  case 34:
#line 245 "dvdvmy.y"
                                                    {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_JUMP;
  /* values already range-checked: */
    (yyval.statement)->i1=(yyvsp[-3].int_val);
    (yyval.statement)->i2=(yyvsp[-2].int_val);
    (yyval.statement)->i3=(yyvsp[-1].int_val);
}
#line 1898 "dvdvmy.c"
    break;

  case 35:
#line 253 "dvdvmy.y"
                                          {
    if ((yyvsp[-1].int_val) < 1 || (yyvsp[-1].int_val) > 65535)
      {
        yyerror("cell number out of range");
      } /*if*/
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_JUMP;
    (yyval.statement)->i3=2*65536+(yyvsp[-1].int_val);
}
#line 1912 "dvdvmy.c"
    break;

  case 36:
#line 262 "dvdvmy.y"
                                   {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_JUMP_AUDIOGROUP;
    (yyval.statement)->audiogroup=(yyvsp[-2].int_val);
    (yyval.statement)->i2=(yyvsp[-1].int_val);

}
#line 1924 "dvdvmy.c"
    break;

  case 37:
#line 269 "dvdvmy.y"
                                             {
    if ((yyvsp[-1].int_val) < 1 || (yyvsp[-1].int_val) > 65535)
      {
        yyerror("program number out of range");
      } /*if*/
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_JUMP;
    (yyval.statement)->i3=65536+(yyvsp[-1].int_val);
}
#line 1938 "dvdvmy.c"
    break;

  case 38:
#line 278 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=1;
}
#line 1948 "dvdvmy.c"
    break;

  case 39:
#line 283 "dvdvmy.y"
                                           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=2;
}
#line 1958 "dvdvmy.c"
    break;

  case 40:
#line 288 "dvdvmy.y"
                                           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=3;
}
#line 1968 "dvdvmy.c"
    break;

  case 41:
#line 293 "dvdvmy.y"
                                             {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=5;
}
#line 1978 "dvdvmy.c"
    break;

  case 42:
#line 298 "dvdvmy.y"
                                              {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=6;
}
#line 1988 "dvdvmy.c"
    break;

  case 43:
#line 303 "dvdvmy.y"
                                              {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=7;
}
#line 1998 "dvdvmy.c"
    break;

  case 44:
#line 308 "dvdvmy.y"
                                         {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=9;
}
#line 2008 "dvdvmy.c"
    break;

  case 45:
#line 313 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=10;
}
#line 2018 "dvdvmy.c"
    break;

  case 46:
#line 318 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=11;
}
#line 2028 "dvdvmy.c"
    break;

  case 47:
#line 323 "dvdvmy.y"
                                        {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=12;
}
#line 2038 "dvdvmy.c"
    break;

  case 48:
#line 328 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=13;
}
#line 2048 "dvdvmy.c"
    break;

  case 49:
#line 335 "dvdvmy.y"
                            {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 65535)
      {
        yyerror("resume cell number out of range");
      } /*if*/
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 2060 "dvdvmy.c"
    break;

  case 50:
#line 342 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 2068 "dvdvmy.c"
    break;

  case 51:
#line 347 "dvdvmy.y"
                                                            {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_CALL;
  /* values already range-checked: */
    (yyval.statement)->i1=(yyvsp[-4].int_val);
    (yyval.statement)->i2=(yyvsp[-3].int_val);
    (yyval.statement)->i3=(yyvsp[-2].int_val);
    (yyval.statement)->i4=(yyvsp[-1].int_val);
}
#line 2082 "dvdvmy.c"
    break;

  case 52:
#line 358 "dvdvmy.y"
           {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 2090 "dvdvmy.c"
    break;

  case 53:
#line 361 "dvdvmy.y"
        {
    (yyval.int_val)=(yyvsp[0].int_val)+0x80;
}
#line 2098 "dvdvmy.c"
    break;

  case 54:
#line 364 "dvdvmy.y"
            {
    (yyval.int_val)=0x81;
}
#line 2106 "dvdvmy.c"
    break;

  case 55:
#line 367 "dvdvmy.y"
               {
    (yyval.int_val)=0x82;
}
#line 2114 "dvdvmy.c"
    break;

  case 56:
#line 370 "dvdvmy.y"
            {
    (yyval.int_val)=0x83;
}
#line 2122 "dvdvmy.c"
    break;

  case 57:
#line 373 "dvdvmy.y"
             {
    (yyval.int_val)=0x88;
}
#line 2130 "dvdvmy.c"
    break;

  case 58:
#line 376 "dvdvmy.y"
             {
    (yyval.int_val)=0x80+20;
}
#line 2138 "dvdvmy.c"
    break;

  case 59:
#line 381 "dvdvmy.y"
              {
    (yyval.int_val)=(yyvsp[0].int_val)-256;
}
#line 2146 "dvdvmy.c"
    break;

  case 60:
#line 384 "dvdvmy.y"
          {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 2154 "dvdvmy.c"
    break;

  case 61:
#line 389 "dvdvmy.y"
                                                    {
    (yyval.statement)=(yyvsp[-1].statement);
}
#line 2162 "dvdvmy.c"
    break;

  case 62:
#line 392 "dvdvmy.y"
           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_VAL;
    (yyval.statement)->i1=(yyvsp[0].int_val);
}
#line 2172 "dvdvmy.c"
    break;

  case 63:
#line 397 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_ADD,(yyvsp[0].statement));
}
#line 2180 "dvdvmy.c"
    break;

  case 64:
#line 400 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_SUB,(yyvsp[0].statement));
}
#line 2188 "dvdvmy.c"
    break;

  case 65:
#line 403 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_MUL,(yyvsp[0].statement));
}
#line 2196 "dvdvmy.c"
    break;

  case 66:
#line 406 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_DIV,(yyvsp[0].statement));
}
#line 2204 "dvdvmy.c"
    break;

  case 67:
#line 409 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_MOD,(yyvsp[0].statement));
}
#line 2212 "dvdvmy.c"
    break;

  case 68:
#line 412 "dvdvmy.y"
                                 {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_AND,(yyvsp[0].statement));
}
#line 2220 "dvdvmy.c"
    break;

  case 69:
#line 415 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_OR, (yyvsp[0].statement));
}
#line 2228 "dvdvmy.c"
    break;

  case 70:
#line 418 "dvdvmy.y"
                                 {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_AND,(yyvsp[0].statement));
}
#line 2236 "dvdvmy.c"
    break;

  case 71:
#line 421 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_OR, (yyvsp[0].statement));
}
#line 2244 "dvdvmy.c"
    break;

  case 72:
#line 424 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_XOR,(yyvsp[0].statement));
}
#line 2252 "dvdvmy.c"
    break;

  case 73:
#line 427 "dvdvmy.y"
                                                  {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_RND;
    (yyval.statement)->param=(yyvsp[-1].statement);
}
#line 2262 "dvdvmy.c"
    break;

  case 74:
#line 434 "dvdvmy.y"
                                                {
    (yyval.statement)=(yyvsp[-1].statement);
}
#line 2270 "dvdvmy.c"
    break;

  case 75:
#line 437 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_EQ,(yyvsp[0].statement));
}
#line 2278 "dvdvmy.c"
    break;

  case 76:
#line 440 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_NE,(yyvsp[0].statement));
}
#line 2286 "dvdvmy.c"
    break;

  case 77:
#line 443 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_GTE,(yyvsp[0].statement));
}
#line 2294 "dvdvmy.c"
    break;

  case 78:
#line 446 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_GT,(yyvsp[0].statement));
}
#line 2302 "dvdvmy.c"
    break;

  case 79:
#line 449 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LTE,(yyvsp[0].statement));
}
#line 2310 "dvdvmy.c"
    break;

  case 80:
#line 452 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LT,(yyvsp[0].statement));
}
#line 2318 "dvdvmy.c"
    break;

  case 81:
#line 455 "dvdvmy.y"
                            {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LOR,(yyvsp[0].statement));
}
#line 2326 "dvdvmy.c"
    break;

  case 82:
#line 458 "dvdvmy.y"
                             {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LAND,(yyvsp[0].statement));
}
#line 2334 "dvdvmy.c"
    break;

  case 83:
#line 461 "dvdvmy.y"
                            {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LOR,(yyvsp[0].statement));
}
#line 2342 "dvdvmy.c"
    break;

  case 84:
#line 464 "dvdvmy.y"
                             {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LAND,(yyvsp[0].statement));
}
#line 2350 "dvdvmy.c"
    break;

  case 85:
#line 467 "dvdvmy.y"
                   {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_NOT;
    (yyval.statement)->param=(yyvsp[0].statement);
}
#line 2360 "dvdvmy.c"
    break;

  case 86:
#line 474 "dvdvmy.y"
                  {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 2368 "dvdvmy.c"
    break;

  case 87:
#line 477 "dvdvmy.y"
                    {
    (yyval.int_val)=(yyvsp[0].int_val)+0x20;
}
#line 2376 "dvdvmy.c"
    break;

  case 88:
#line 482 "dvdvmy.y"
                                                            {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_SET;
    (yyval.statement)->i1=(yyvsp[-3].int_val);
    (yyval.statement)->param=(yyvsp[-1].statement);
}
#line 2387 "dvdvmy.c"
    break;

  case 89:
#line 488 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_ADD,(yyvsp[-1].statement));
}
#line 2395 "dvdvmy.c"
    break;

  case 90:
#line 491 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_SUB,(yyvsp[-1].statement));
}
#line 2403 "dvdvmy.c"
    break;

  case 91:
#line 494 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_MUL,(yyvsp[-1].statement));
}
#line 2411 "dvdvmy.c"
    break;

  case 92:
#line 497 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_DIV,(yyvsp[-1].statement));
}
#line 2419 "dvdvmy.c"
    break;

  case 93:
#line 500 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_MOD,(yyvsp[-1].statement));
}
#line 2427 "dvdvmy.c"
    break;

  case 94:
#line 503 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_AND,(yyvsp[-1].statement));
}
#line 2435 "dvdvmy.c"
    break;

  case 95:
#line 506 "dvdvmy.y"
                                         {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_OR,(yyvsp[-1].statement));
}
#line 2443 "dvdvmy.c"
    break;

  case 96:
#line 509 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_XOR,(yyvsp[-1].statement));
}
#line 2451 "dvdvmy.c"
    break;

  case 97:
#line 514 "dvdvmy.y"
                                                                    {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_IF;
    (yyval.statement)->param=(yyvsp[-2].statement);
    (yyvsp[-2].statement)->next=statement_new();
    (yyvsp[-2].statement)->next->op=VM_IF;
    (yyvsp[-2].statement)->next->param=(yyvsp[0].statement);
}
#line 2464 "dvdvmy.c"
    break;

  case 98:
#line 524 "dvdvmy.y"
                             {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 2472 "dvdvmy.c"
    break;

  case 99:
#line 527 "dvdvmy.y"
                                 {
    (yyval.statement)=(yyvsp[-2].statement);
    (yyval.statement)->param->next->next=(yyvsp[0].statement);
}
#line 2481 "dvdvmy.c"
    break;


#line 2485 "dvdvmy.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
