/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

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


#line 107 "dvdvmy.c"

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

#include "dvdvmy.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_NUM_TOK = 3,                    /* NUM_TOK  */
  YYSYMBOL_G_TOK = 4,                      /* G_TOK  */
  YYSYMBOL_S_TOK = 5,                      /* S_TOK  */
  YYSYMBOL_ID_TOK = 6,                     /* ID_TOK  */
  YYSYMBOL_ANGLE_TOK = 7,                  /* ANGLE_TOK  */
  YYSYMBOL_AUDIO_TOK = 8,                  /* AUDIO_TOK  */
  YYSYMBOL_BREAK_TOK = 9,                  /* BREAK_TOK  */
  YYSYMBOL_BUTTON_TOK = 10,                /* BUTTON_TOK  */
  YYSYMBOL_CALL_TOK = 11,                  /* CALL_TOK  */
  YYSYMBOL_CELL_TOK = 12,                  /* CELL_TOK  */
  YYSYMBOL_CHAPTER_TOK = 13,               /* CHAPTER_TOK  */
  YYSYMBOL_CLOSEBRACE_TOK = 14,            /* CLOSEBRACE_TOK  */
  YYSYMBOL_CLOSEPAREN_TOK = 15,            /* CLOSEPAREN_TOK  */
  YYSYMBOL_COUNTER_TOK = 16,               /* COUNTER_TOK  */
  YYSYMBOL_ELSE_TOK = 17,                  /* ELSE_TOK  */
  YYSYMBOL_ENTRY_TOK = 18,                 /* ENTRY_TOK  */
  YYSYMBOL_EXIT_TOK = 19,                  /* EXIT_TOK  */
  YYSYMBOL_FPC_TOK = 20,                   /* FPC_TOK  */
  YYSYMBOL_GOTO_TOK = 21,                  /* GOTO_TOK  */
  YYSYMBOL_IF_TOK = 22,                    /* IF_TOK  */
  YYSYMBOL_JUMP_TOK = 23,                  /* JUMP_TOK  */
  YYSYMBOL_MENU_TOK = 24,                  /* MENU_TOK  */
  YYSYMBOL_NEXT_TOK = 25,                  /* NEXT_TOK  */
  YYSYMBOL_OPENBRACE_TOK = 26,             /* OPENBRACE_TOK  */
  YYSYMBOL_OPENPAREN_TOK = 27,             /* OPENPAREN_TOK  */
  YYSYMBOL_PGC_TOK = 28,                   /* PGC_TOK  */
  YYSYMBOL_PREV_TOK = 29,                  /* PREV_TOK  */
  YYSYMBOL_PROGRAM_TOK = 30,               /* PROGRAM_TOK  */
  YYSYMBOL_PTT_TOK = 31,                   /* PTT_TOK  */
  YYSYMBOL_REGION_TOK = 32,                /* REGION_TOK  */
  YYSYMBOL_RESUME_TOK = 33,                /* RESUME_TOK  */
  YYSYMBOL_RND_TOK = 34,                   /* RND_TOK  */
  YYSYMBOL_ROOT_TOK = 35,                  /* ROOT_TOK  */
  YYSYMBOL_SET_TOK = 36,                   /* SET_TOK  */
  YYSYMBOL_SUBTITLE_TOK = 37,              /* SUBTITLE_TOK  */
  YYSYMBOL_TAIL_TOK = 38,                  /* TAIL_TOK  */
  YYSYMBOL_TITLE_TOK = 39,                 /* TITLE_TOK  */
  YYSYMBOL_TITLESET_TOK = 40,              /* TITLESET_TOK  */
  YYSYMBOL_TOP_TOK = 41,                   /* TOP_TOK  */
  YYSYMBOL_UP_TOK = 42,                    /* UP_TOK  */
  YYSYMBOL_VMGM_TOK = 43,                  /* VMGM_TOK  */
  YYSYMBOL_AMGM_TOK = 44,                  /* AMGM_TOK  */
  YYSYMBOL_GROUP_TOK = 45,                 /* GROUP_TOK  */
  YYSYMBOL_TRACK_TOK = 46,                 /* TRACK_TOK  */
  YYSYMBOL__OR_TOK = 47,                   /* _OR_TOK  */
  YYSYMBOL_XOR_TOK = 48,                   /* XOR_TOK  */
  YYSYMBOL_LOR_TOK = 49,                   /* LOR_TOK  */
  YYSYMBOL_BOR_TOK = 50,                   /* BOR_TOK  */
  YYSYMBOL__AND_TOK = 51,                  /* _AND_TOK  */
  YYSYMBOL_LAND_TOK = 52,                  /* LAND_TOK  */
  YYSYMBOL_BAND_TOK = 53,                  /* BAND_TOK  */
  YYSYMBOL_NOT_TOK = 54,                   /* NOT_TOK  */
  YYSYMBOL_EQ_TOK = 55,                    /* EQ_TOK  */
  YYSYMBOL_NE_TOK = 56,                    /* NE_TOK  */
  YYSYMBOL_GE_TOK = 57,                    /* GE_TOK  */
  YYSYMBOL_GT_TOK = 58,                    /* GT_TOK  */
  YYSYMBOL_LE_TOK = 59,                    /* LE_TOK  */
  YYSYMBOL_LT_TOK = 60,                    /* LT_TOK  */
  YYSYMBOL_ADD_TOK = 61,                   /* ADD_TOK  */
  YYSYMBOL_SUB_TOK = 62,                   /* SUB_TOK  */
  YYSYMBOL_MUL_TOK = 63,                   /* MUL_TOK  */
  YYSYMBOL_DIV_TOK = 64,                   /* DIV_TOK  */
  YYSYMBOL_MOD_TOK = 65,                   /* MOD_TOK  */
  YYSYMBOL_ADDSET_TOK = 66,                /* ADDSET_TOK  */
  YYSYMBOL_SUBSET_TOK = 67,                /* SUBSET_TOK  */
  YYSYMBOL_MULSET_TOK = 68,                /* MULSET_TOK  */
  YYSYMBOL_DIVSET_TOK = 69,                /* DIVSET_TOK  */
  YYSYMBOL_MODSET_TOK = 70,                /* MODSET_TOK  */
  YYSYMBOL_ANDSET_TOK = 71,                /* ANDSET_TOK  */
  YYSYMBOL_ORSET_TOK = 72,                 /* ORSET_TOK  */
  YYSYMBOL_XORSET_TOK = 73,                /* XORSET_TOK  */
  YYSYMBOL_SEMICOLON_TOK = 74,             /* SEMICOLON_TOK  */
  YYSYMBOL_COLON_TOK = 75,                 /* COLON_TOK  */
  YYSYMBOL_ERROR_TOK = 76,                 /* ERROR_TOK  */
  YYSYMBOL_YYACCEPT = 77,                  /* $accept  */
  YYSYMBOL_finalparse = 78,                /* finalparse  */
  YYSYMBOL_statements = 79,                /* statements  */
  YYSYMBOL_statement = 80,                 /* statement  */
  YYSYMBOL_jtsl = 81,                      /* jtsl  */
  YYSYMBOL_jgrl = 82,                      /* jgrl  */
  YYSYMBOL_jtml = 83,                      /* jtml  */
  YYSYMBOL_jcl = 84,                       /* jcl  */
  YYSYMBOL_jumpstatement = 85,             /* jumpstatement  */
  YYSYMBOL_resumel = 86,                   /* resumel  */
  YYSYMBOL_callstatement = 87,             /* callstatement  */
  YYSYMBOL_reg = 88,                       /* reg  */
  YYSYMBOL_regornum = 89,                  /* regornum  */
  YYSYMBOL_expression = 90,                /* expression  */
  YYSYMBOL_boolexpr = 91,                  /* boolexpr  */
  YYSYMBOL_regorcounter = 92,              /* regorcounter  */
  YYSYMBOL_setstatement = 93,              /* setstatement  */
  YYSYMBOL_ifstatement = 94,               /* ifstatement  */
  YYSYMBOL_ifelsestatement = 95            /* ifelsestatement  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
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
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
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

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

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

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   331


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

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

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "NUM_TOK", "G_TOK",
  "S_TOK", "ID_TOK", "ANGLE_TOK", "AUDIO_TOK", "BREAK_TOK", "BUTTON_TOK",
  "CALL_TOK", "CELL_TOK", "CHAPTER_TOK", "CLOSEBRACE_TOK",
  "CLOSEPAREN_TOK", "COUNTER_TOK", "ELSE_TOK", "ENTRY_TOK", "EXIT_TOK",
  "FPC_TOK", "GOTO_TOK", "IF_TOK", "JUMP_TOK", "MENU_TOK", "NEXT_TOK",
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

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

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
static const yytype_uint8 yydefgoto[] =
{
       0,    18,    19,    20,    33,    46,    66,   109,    21,   168,
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

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
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

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
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

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
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


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
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
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

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
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
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
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  case 2: /* finalparse: statements  */
#line 104 "dvdvmy.y"
                       {
    dvd_vm_parsed_cmd=(yyval.statement);
}
#line 1373 "dvdvmy.c"
    break;

  case 3: /* statements: statement  */
#line 109 "dvdvmy.y"
                      {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1381 "dvdvmy.c"
    break;

  case 4: /* statements: statement statements  */
#line 112 "dvdvmy.y"
                       {
    (yyval.statement)=(yyvsp[-1].statement);
    (yyval.statement)->next=(yyvsp[0].statement);
}
#line 1390 "dvdvmy.c"
    break;

  case 5: /* statement: jumpstatement  */
#line 118 "dvdvmy.y"
                         {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1398 "dvdvmy.c"
    break;

  case 6: /* statement: callstatement  */
#line 121 "dvdvmy.y"
                {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1406 "dvdvmy.c"
    break;

  case 7: /* statement: EXIT_TOK SEMICOLON_TOK  */
#line 124 "dvdvmy.y"
                         {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_EXIT;
}
#line 1415 "dvdvmy.c"
    break;

  case 8: /* statement: RESUME_TOK SEMICOLON_TOK  */
#line 128 "dvdvmy.y"
                           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=16;
}
#line 1425 "dvdvmy.c"
    break;

  case 9: /* statement: GOTO_TOK ID_TOK SEMICOLON_TOK  */
#line 133 "dvdvmy.y"
                                {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_GOTO;
    (yyval.statement)->s1=(yyvsp[-1].str_val);
}
#line 1435 "dvdvmy.c"
    break;

  case 10: /* statement: ID_TOK COLON_TOK  */
#line 138 "dvdvmy.y"
                   {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LABEL;
    (yyval.statement)->s1=(yyvsp[-1].str_val);
}
#line 1445 "dvdvmy.c"
    break;

  case 11: /* statement: BREAK_TOK SEMICOLON_TOK  */
#line 143 "dvdvmy.y"
                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_BREAK;
}
#line 1454 "dvdvmy.c"
    break;

  case 12: /* statement: setstatement  */
#line 147 "dvdvmy.y"
               {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1462 "dvdvmy.c"
    break;

  case 13: /* statement: OPENBRACE_TOK statements CLOSEBRACE_TOK  */
#line 150 "dvdvmy.y"
                                          {
    (yyval.statement)=(yyvsp[-1].statement);
}
#line 1470 "dvdvmy.c"
    break;

  case 14: /* statement: ifelsestatement  */
#line 153 "dvdvmy.y"
                  {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 1478 "dvdvmy.c"
    break;

  case 15: /* jtsl: TITLESET_TOK NUM_TOK  */
#line 158 "dvdvmy.y"
                           {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
      {
        yyerror("titleset number out of range");
      } /*if*/
    (yyval.int_val)=((yyvsp[0].int_val))+1;
}
#line 1490 "dvdvmy.c"
    break;

  case 16: /* jtsl: VMGM_TOK  */
#line 165 "dvdvmy.y"
           {
    (yyval.int_val)=1;
}
#line 1498 "dvdvmy.c"
    break;

  case 17: /* jtsl: AMGM_TOK  */
#line 168 "dvdvmy.y"
           {
    (yyval.int_val)=1;
}
#line 1506 "dvdvmy.c"
    break;

  case 18: /* jtsl: %empty  */
#line 171 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1514 "dvdvmy.c"
    break;

  case 19: /* jgrl: GROUP_TOK NUM_TOK  */
#line 177 "dvdvmy.y"
                        {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1522 "dvdvmy.c"
    break;

  case 20: /* jtml: MENU_TOK NUM_TOK  */
#line 183 "dvdvmy.y"
                       {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
      {
        yyerror("menu number out of range");
      } /*if*/
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1534 "dvdvmy.c"
    break;

  case 21: /* jtml: MENU_TOK  */
#line 190 "dvdvmy.y"
           {
    (yyval.int_val)=120; // default entry
}
#line 1542 "dvdvmy.c"
    break;

  case 22: /* jtml: MENU_TOK ENTRY_TOK TITLE_TOK  */
#line 193 "dvdvmy.y"
                               {
    (yyval.int_val)=122;
}
#line 1550 "dvdvmy.c"
    break;

  case 23: /* jtml: MENU_TOK ENTRY_TOK ROOT_TOK  */
#line 196 "dvdvmy.y"
                              {
    (yyval.int_val)=123;
}
#line 1558 "dvdvmy.c"
    break;

  case 24: /* jtml: MENU_TOK ENTRY_TOK SUBTITLE_TOK  */
#line 199 "dvdvmy.y"
                                  {
    (yyval.int_val)=124;
}
#line 1566 "dvdvmy.c"
    break;

  case 25: /* jtml: MENU_TOK ENTRY_TOK AUDIO_TOK  */
#line 202 "dvdvmy.y"
                               {
    (yyval.int_val)=125;
}
#line 1574 "dvdvmy.c"
    break;

  case 26: /* jtml: MENU_TOK ENTRY_TOK ANGLE_TOK  */
#line 205 "dvdvmy.y"
                               {
    (yyval.int_val)=126;
}
#line 1582 "dvdvmy.c"
    break;

  case 27: /* jtml: MENU_TOK ENTRY_TOK PTT_TOK  */
#line 208 "dvdvmy.y"
                             {
    (yyval.int_val)=127;
}
#line 1590 "dvdvmy.c"
    break;

  case 28: /* jtml: FPC_TOK  */
#line 211 "dvdvmy.y"
          {
    (yyval.int_val)=121;
}
#line 1598 "dvdvmy.c"
    break;

  case 29: /* jtml: TITLE_TOK NUM_TOK  */
#line 214 "dvdvmy.y"
                    {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
      {
        yyerror("title number out of range");
      } /*if*/
    (yyval.int_val)=((yyvsp[0].int_val))|128;
}
#line 1610 "dvdvmy.c"
    break;

  case 30: /* jtml: TRACK_TOK NUM_TOK  */
#line 221 "dvdvmy.y"
                    {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 99)
  {
    yyerror("title number out of range");
  }
    (yyval.int_val)=((yyvsp[0].int_val))|128;
}
#line 1622 "dvdvmy.c"
    break;

  case 31: /* jtml: %empty  */
#line 228 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1630 "dvdvmy.c"
    break;

  case 32: /* jcl: CHAPTER_TOK NUM_TOK  */
#line 233 "dvdvmy.y"
                         {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 65535)
      {
        yyerror("chapter number out of range");
      } /*if*/
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1642 "dvdvmy.c"
    break;

  case 33: /* jcl: %empty  */
#line 240 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1650 "dvdvmy.c"
    break;

  case 34: /* jumpstatement: JUMP_TOK jtsl jtml jcl SEMICOLON_TOK  */
#line 245 "dvdvmy.y"
                                                    {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_JUMP;
  /* values already range-checked: */
    (yyval.statement)->i1=(yyvsp[-3].int_val);
    (yyval.statement)->i2=(yyvsp[-2].int_val);
    (yyval.statement)->i3=(yyvsp[-1].int_val);
}
#line 1663 "dvdvmy.c"
    break;

  case 35: /* jumpstatement: JUMP_TOK CELL_TOK NUM_TOK SEMICOLON_TOK  */
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
#line 1677 "dvdvmy.c"
    break;

  case 36: /* jumpstatement: JUMP_TOK jgrl jtml SEMICOLON_TOK  */
#line 262 "dvdvmy.y"
                                   {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_JUMP_AUDIOGROUP;
    (yyval.statement)->audiogroup=(yyvsp[-2].int_val);
    (yyval.statement)->i2=(yyvsp[-1].int_val);

}
#line 1689 "dvdvmy.c"
    break;

  case 37: /* jumpstatement: JUMP_TOK PROGRAM_TOK NUM_TOK SEMICOLON_TOK  */
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
#line 1703 "dvdvmy.c"
    break;

  case 38: /* jumpstatement: JUMP_TOK CELL_TOK TOP_TOK SEMICOLON_TOK  */
#line 278 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=1;
}
#line 1713 "dvdvmy.c"
    break;

  case 39: /* jumpstatement: JUMP_TOK NEXT_TOK CELL_TOK SEMICOLON_TOK  */
#line 283 "dvdvmy.y"
                                           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=2;
}
#line 1723 "dvdvmy.c"
    break;

  case 40: /* jumpstatement: JUMP_TOK PREV_TOK CELL_TOK SEMICOLON_TOK  */
#line 288 "dvdvmy.y"
                                           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=3;
}
#line 1733 "dvdvmy.c"
    break;

  case 41: /* jumpstatement: JUMP_TOK PROGRAM_TOK TOP_TOK SEMICOLON_TOK  */
#line 293 "dvdvmy.y"
                                             {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=5;
}
#line 1743 "dvdvmy.c"
    break;

  case 42: /* jumpstatement: JUMP_TOK NEXT_TOK PROGRAM_TOK SEMICOLON_TOK  */
#line 298 "dvdvmy.y"
                                              {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=6;
}
#line 1753 "dvdvmy.c"
    break;

  case 43: /* jumpstatement: JUMP_TOK PREV_TOK PROGRAM_TOK SEMICOLON_TOK  */
#line 303 "dvdvmy.y"
                                              {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=7;
}
#line 1763 "dvdvmy.c"
    break;

  case 44: /* jumpstatement: JUMP_TOK PGC_TOK TOP_TOK SEMICOLON_TOK  */
#line 308 "dvdvmy.y"
                                         {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=9;
}
#line 1773 "dvdvmy.c"
    break;

  case 45: /* jumpstatement: JUMP_TOK NEXT_TOK PGC_TOK SEMICOLON_TOK  */
#line 313 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=10;
}
#line 1783 "dvdvmy.c"
    break;

  case 46: /* jumpstatement: JUMP_TOK PREV_TOK PGC_TOK SEMICOLON_TOK  */
#line 318 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=11;
}
#line 1793 "dvdvmy.c"
    break;

  case 47: /* jumpstatement: JUMP_TOK UP_TOK PGC_TOK SEMICOLON_TOK  */
#line 323 "dvdvmy.y"
                                        {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=12;
}
#line 1803 "dvdvmy.c"
    break;

  case 48: /* jumpstatement: JUMP_TOK PGC_TOK TAIL_TOK SEMICOLON_TOK  */
#line 328 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_LINK;
    (yyval.statement)->i1=13;
}
#line 1813 "dvdvmy.c"
    break;

  case 49: /* resumel: RESUME_TOK NUM_TOK  */
#line 335 "dvdvmy.y"
                            {
    if ((yyvsp[0].int_val) < 1 || (yyvsp[0].int_val) > 65535)
      {
        yyerror("resume cell number out of range");
      } /*if*/
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1825 "dvdvmy.c"
    break;

  case 50: /* resumel: %empty  */
#line 342 "dvdvmy.y"
  {
    (yyval.int_val)=0;
}
#line 1833 "dvdvmy.c"
    break;

  case 51: /* callstatement: CALL_TOK jtsl jtml jcl resumel SEMICOLON_TOK  */
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
#line 1847 "dvdvmy.c"
    break;

  case 52: /* reg: G_TOK  */
#line 358 "dvdvmy.y"
           {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1855 "dvdvmy.c"
    break;

  case 53: /* reg: S_TOK  */
#line 361 "dvdvmy.y"
        {
    (yyval.int_val)=(yyvsp[0].int_val)+0x80;
}
#line 1863 "dvdvmy.c"
    break;

  case 54: /* reg: AUDIO_TOK  */
#line 364 "dvdvmy.y"
            {
    (yyval.int_val)=0x81;
}
#line 1871 "dvdvmy.c"
    break;

  case 55: /* reg: SUBTITLE_TOK  */
#line 367 "dvdvmy.y"
               {
    (yyval.int_val)=0x82;
}
#line 1879 "dvdvmy.c"
    break;

  case 56: /* reg: ANGLE_TOK  */
#line 370 "dvdvmy.y"
            {
    (yyval.int_val)=0x83;
}
#line 1887 "dvdvmy.c"
    break;

  case 57: /* reg: BUTTON_TOK  */
#line 373 "dvdvmy.y"
             {
    (yyval.int_val)=0x88;
}
#line 1895 "dvdvmy.c"
    break;

  case 58: /* reg: REGION_TOK  */
#line 376 "dvdvmy.y"
             {
    (yyval.int_val)=0x80+20;
}
#line 1903 "dvdvmy.c"
    break;

  case 59: /* regornum: reg  */
#line 381 "dvdvmy.y"
              {
    (yyval.int_val)=(yyvsp[0].int_val)-256;
}
#line 1911 "dvdvmy.c"
    break;

  case 60: /* regornum: NUM_TOK  */
#line 384 "dvdvmy.y"
          {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 1919 "dvdvmy.c"
    break;

  case 61: /* expression: OPENPAREN_TOK expression CLOSEPAREN_TOK  */
#line 389 "dvdvmy.y"
                                                    {
    (yyval.statement)=(yyvsp[-1].statement);
}
#line 1927 "dvdvmy.c"
    break;

  case 62: /* expression: regornum  */
#line 392 "dvdvmy.y"
           {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_VAL;
    (yyval.statement)->i1=(yyvsp[0].int_val);
}
#line 1937 "dvdvmy.c"
    break;

  case 63: /* expression: expression ADD_TOK expression  */
#line 397 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_ADD,(yyvsp[0].statement));
}
#line 1945 "dvdvmy.c"
    break;

  case 64: /* expression: expression SUB_TOK expression  */
#line 400 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_SUB,(yyvsp[0].statement));
}
#line 1953 "dvdvmy.c"
    break;

  case 65: /* expression: expression MUL_TOK expression  */
#line 403 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_MUL,(yyvsp[0].statement));
}
#line 1961 "dvdvmy.c"
    break;

  case 66: /* expression: expression DIV_TOK expression  */
#line 406 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_DIV,(yyvsp[0].statement));
}
#line 1969 "dvdvmy.c"
    break;

  case 67: /* expression: expression MOD_TOK expression  */
#line 409 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_MOD,(yyvsp[0].statement));
}
#line 1977 "dvdvmy.c"
    break;

  case 68: /* expression: expression BAND_TOK expression  */
#line 412 "dvdvmy.y"
                                 {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_AND,(yyvsp[0].statement));
}
#line 1985 "dvdvmy.c"
    break;

  case 69: /* expression: expression BOR_TOK expression  */
#line 415 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_OR, (yyvsp[0].statement));
}
#line 1993 "dvdvmy.c"
    break;

  case 70: /* expression: expression _AND_TOK expression  */
#line 418 "dvdvmy.y"
                                 {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_AND,(yyvsp[0].statement));
}
#line 2001 "dvdvmy.c"
    break;

  case 71: /* expression: expression _OR_TOK expression  */
#line 421 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_OR, (yyvsp[0].statement));
}
#line 2009 "dvdvmy.c"
    break;

  case 72: /* expression: expression XOR_TOK expression  */
#line 424 "dvdvmy.y"
                                {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_XOR,(yyvsp[0].statement));
}
#line 2017 "dvdvmy.c"
    break;

  case 73: /* expression: RND_TOK OPENPAREN_TOK expression CLOSEPAREN_TOK  */
#line 427 "dvdvmy.y"
                                                  {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_RND;
    (yyval.statement)->param=(yyvsp[-1].statement);
}
#line 2027 "dvdvmy.c"
    break;

  case 74: /* boolexpr: OPENPAREN_TOK boolexpr CLOSEPAREN_TOK  */
#line 434 "dvdvmy.y"
                                                {
    (yyval.statement)=(yyvsp[-1].statement);
}
#line 2035 "dvdvmy.c"
    break;

  case 75: /* boolexpr: expression EQ_TOK expression  */
#line 437 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_EQ,(yyvsp[0].statement));
}
#line 2043 "dvdvmy.c"
    break;

  case 76: /* boolexpr: expression NE_TOK expression  */
#line 440 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_NE,(yyvsp[0].statement));
}
#line 2051 "dvdvmy.c"
    break;

  case 77: /* boolexpr: expression GE_TOK expression  */
#line 443 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_GTE,(yyvsp[0].statement));
}
#line 2059 "dvdvmy.c"
    break;

  case 78: /* boolexpr: expression GT_TOK expression  */
#line 446 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_GT,(yyvsp[0].statement));
}
#line 2067 "dvdvmy.c"
    break;

  case 79: /* boolexpr: expression LE_TOK expression  */
#line 449 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LTE,(yyvsp[0].statement));
}
#line 2075 "dvdvmy.c"
    break;

  case 80: /* boolexpr: expression LT_TOK expression  */
#line 452 "dvdvmy.y"
                               {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LT,(yyvsp[0].statement));
}
#line 2083 "dvdvmy.c"
    break;

  case 81: /* boolexpr: boolexpr LOR_TOK boolexpr  */
#line 455 "dvdvmy.y"
                            {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LOR,(yyvsp[0].statement));
}
#line 2091 "dvdvmy.c"
    break;

  case 82: /* boolexpr: boolexpr LAND_TOK boolexpr  */
#line 458 "dvdvmy.y"
                             {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LAND,(yyvsp[0].statement));
}
#line 2099 "dvdvmy.c"
    break;

  case 83: /* boolexpr: boolexpr _OR_TOK boolexpr  */
#line 461 "dvdvmy.y"
                            {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LOR,(yyvsp[0].statement));
}
#line 2107 "dvdvmy.c"
    break;

  case 84: /* boolexpr: boolexpr _AND_TOK boolexpr  */
#line 464 "dvdvmy.y"
                             {
    (yyval.statement)=statement_expression((yyvsp[-2].statement),VM_LAND,(yyvsp[0].statement));
}
#line 2115 "dvdvmy.c"
    break;

  case 85: /* boolexpr: NOT_TOK boolexpr  */
#line 467 "dvdvmy.y"
                   {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_NOT;
    (yyval.statement)->param=(yyvsp[0].statement);
}
#line 2125 "dvdvmy.c"
    break;

  case 86: /* regorcounter: reg  */
#line 474 "dvdvmy.y"
                  {
    (yyval.int_val)=(yyvsp[0].int_val);
}
#line 2133 "dvdvmy.c"
    break;

  case 87: /* regorcounter: COUNTER_TOK G_TOK  */
#line 477 "dvdvmy.y"
                    {
    (yyval.int_val)=(yyvsp[0].int_val)+0x20;
}
#line 2141 "dvdvmy.c"
    break;

  case 88: /* setstatement: regorcounter SET_TOK expression SEMICOLON_TOK  */
#line 482 "dvdvmy.y"
                                                            {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_SET;
    (yyval.statement)->i1=(yyvsp[-3].int_val);
    (yyval.statement)->param=(yyvsp[-1].statement);
}
#line 2152 "dvdvmy.c"
    break;

  case 89: /* setstatement: reg ADDSET_TOK expression SEMICOLON_TOK  */
#line 488 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_ADD,(yyvsp[-1].statement));
}
#line 2160 "dvdvmy.c"
    break;

  case 90: /* setstatement: reg SUBSET_TOK expression SEMICOLON_TOK  */
#line 491 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_SUB,(yyvsp[-1].statement));
}
#line 2168 "dvdvmy.c"
    break;

  case 91: /* setstatement: reg MULSET_TOK expression SEMICOLON_TOK  */
#line 494 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_MUL,(yyvsp[-1].statement));
}
#line 2176 "dvdvmy.c"
    break;

  case 92: /* setstatement: reg DIVSET_TOK expression SEMICOLON_TOK  */
#line 497 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_DIV,(yyvsp[-1].statement));
}
#line 2184 "dvdvmy.c"
    break;

  case 93: /* setstatement: reg MODSET_TOK expression SEMICOLON_TOK  */
#line 500 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_MOD,(yyvsp[-1].statement));
}
#line 2192 "dvdvmy.c"
    break;

  case 94: /* setstatement: reg ANDSET_TOK expression SEMICOLON_TOK  */
#line 503 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_AND,(yyvsp[-1].statement));
}
#line 2200 "dvdvmy.c"
    break;

  case 95: /* setstatement: reg ORSET_TOK expression SEMICOLON_TOK  */
#line 506 "dvdvmy.y"
                                         {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_OR,(yyvsp[-1].statement));
}
#line 2208 "dvdvmy.c"
    break;

  case 96: /* setstatement: reg XORSET_TOK expression SEMICOLON_TOK  */
#line 509 "dvdvmy.y"
                                          {
    (yyval.statement)=statement_setop((yyvsp[-3].int_val),VM_XOR,(yyvsp[-1].statement));
}
#line 2216 "dvdvmy.c"
    break;

  case 97: /* ifstatement: IF_TOK OPENPAREN_TOK boolexpr CLOSEPAREN_TOK statement  */
#line 514 "dvdvmy.y"
                                                                    {
    (yyval.statement)=statement_new();
    (yyval.statement)->op=VM_IF;
    (yyval.statement)->param=(yyvsp[-2].statement);
    (yyvsp[-2].statement)->next=statement_new();
    (yyvsp[-2].statement)->next->op=VM_IF;
    (yyvsp[-2].statement)->next->param=(yyvsp[0].statement);
}
#line 2229 "dvdvmy.c"
    break;

  case 98: /* ifelsestatement: ifstatement  */
#line 524 "dvdvmy.y"
                             {
    (yyval.statement)=(yyvsp[0].statement);
}
#line 2237 "dvdvmy.c"
    break;

  case 99: /* ifelsestatement: ifstatement ELSE_TOK statement  */
#line 527 "dvdvmy.y"
                                 {
    (yyval.statement)=(yyvsp[-2].statement);
    (yyval.statement)->param->next->next=(yyvsp[0].statement);
}
#line 2246 "dvdvmy.c"
    break;


#line 2250 "dvdvmy.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

