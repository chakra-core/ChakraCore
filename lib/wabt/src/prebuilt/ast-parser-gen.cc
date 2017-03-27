/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         WABT_AST_PARSER_STYPE
#define YYLTYPE         WABT_AST_PARSER_LTYPE
/* Substitute the variable and function names.  */
#define yyparse         wabt_ast_parser_parse
#define yylex           wabt_ast_parser_lex
#define yyerror         wabt_ast_parser_error
#define yydebug         wabt_ast_parser_debug
#define yynerrs         wabt_ast_parser_nerrs


/* Copy the first part of user declarations.  */
#line 17 "src/ast-parser.y" /* yacc.c:339  */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <utility>

#include "ast-parser.h"
#include "ast-parser-lexer-shared.h"
#include "binary-reader-ast.h"
#include "binary-reader.h"
#include "literal.h"

#define INVALID_VAR_INDEX (-1)

#define RELOCATE_STACK(type, array, stack_base, old_size, new_size)   \
  do {                                                                \
    type* new_stack = new type[new_size]();                           \
    std::move((stack_base), (stack_base) + (old_size), (new_stack));  \
    if ((stack_base) != (array)) {                                    \
      delete[](stack_base);                                           \
    } else {                                                          \
      for (size_t i = 0; i < (old_size); ++i) {                       \
        (stack_base)[i].~type();                                      \
      }                                                               \
    }                                                                 \
    /* Cache the pointer in the parser struct to be deleted later. */ \
    parser->array = (stack_base) = new_stack;                         \
  } while (0)

#define yyoverflow(message, ss, ss_size, vs, vs_size, ls, ls_size, new_size) \
  do {                                                                       \
    size_t old_size = *(new_size);                                           \
    *(new_size) *= 2;                                                        \
    RELOCATE_STACK(yytype_int16, yyssa, *(ss), old_size, *(new_size));       \
    RELOCATE_STACK(YYSTYPE, yyvsa, *(vs), old_size, *(new_size));            \
    RELOCATE_STACK(YYLTYPE, yylsa, *(ls), old_size, *(new_size));            \
  } while (0)

#define DUPTEXT(dst, src)                                \
  (dst).start = wabt_strndup((src).start, (src).length); \
  (dst).length = (src).length

#define YYLLOC_DEFAULT(Current, Rhs, N)                       \
  do                                                          \
    if (N) {                                                  \
      (Current).filename = YYRHSLOC(Rhs, 1).filename;         \
      (Current).line = YYRHSLOC(Rhs, 1).line;                 \
      (Current).first_column = YYRHSLOC(Rhs, 1).first_column; \
      (Current).last_column = YYRHSLOC(Rhs, N).last_column;   \
    } else {                                                  \
      (Current).filename = nullptr;                           \
      (Current).line = YYRHSLOC(Rhs, 0).line;                 \
      (Current).first_column = (Current).last_column =        \
          YYRHSLOC(Rhs, 0).last_column;                       \
    }                                                         \
  while (0)

#define APPEND_FIELD_TO_LIST(module, field, Kind, kind, loc_, item) \
  do {                                                              \
    field = append_module_field(module);                            \
    field->loc = loc_;                                              \
    field->type = ModuleFieldType::Kind;                            \
    field->kind = item;                                             \
  } while (0)

#define APPEND_ITEM_TO_VECTOR(module, kinds, item_ptr) \
  (module)->kinds.push_back(item_ptr)

#define INSERT_BINDING(module, kind, kinds, loc_, name) \
  do                                                    \
    if ((name).start) {                                 \
      (module)->kind##_bindings.emplace(                \
          string_slice_to_string(name),                 \
          Binding(loc_, (module)->kinds.size() - 1));   \
    }                                                   \
  while (0)

#define APPEND_INLINE_EXPORT(module, Kind, loc_, value, index_)         \
  do                                                                    \
    if ((value)->export_.has_export) {                                  \
      ModuleField* export_field;                                        \
      APPEND_FIELD_TO_LIST(module, export_field, Export, export_, loc_, \
                           (value)->export_.export_.release());         \
      export_field->export_->kind = ExternalKind::Kind;                 \
      export_field->export_->var.loc = loc_;                            \
      export_field->export_->var.index = index_;                        \
      APPEND_ITEM_TO_VECTOR(module, exports, export_field->export_);    \
      INSERT_BINDING(module, export, exports, export_field->loc,        \
                     export_field->export_->name);                      \
    }                                                                   \
  while (0)

#define CHECK_IMPORT_ORDERING(module, kind, kinds, loc_)            \
  do {                                                              \
    if ((module)->kinds.size() != (module)->num_##kind##_imports) { \
      ast_parser_error(                                             \
          &loc_, lexer, parser,                                     \
          "imports must occur before all non-import definitions");  \
    }                                                               \
  } while (0)

#define CHECK_END_LABEL(loc, begin_label, end_label)                       \
  do {                                                                     \
    if (!string_slice_is_empty(&(end_label))) {                            \
      if (string_slice_is_empty(&(begin_label))) {                         \
        ast_parser_error(&loc, lexer, parser,                              \
                         "unexpected label \"" PRIstringslice "\"",        \
                         WABT_PRINTF_STRING_SLICE_ARG(end_label));         \
      } else if (!string_slices_are_equal(&(begin_label), &(end_label))) { \
        ast_parser_error(&loc, lexer, parser,                              \
                         "mismatching label \"" PRIstringslice             \
                         "\" != \"" PRIstringslice "\"",                   \
                         WABT_PRINTF_STRING_SLICE_ARG(begin_label),        \
                         WABT_PRINTF_STRING_SLICE_ARG(end_label));         \
      }                                                                    \
      destroy_string_slice(&(end_label));                                  \
    }                                                                      \
  } while (0)

#define YYMALLOC(size) new char [size]
#define YYFREE(p) delete [] (p)

#define USE_NATURAL_ALIGNMENT (~0)

namespace wabt {

ExprList join_exprs1(Location* loc, Expr* expr1);
ExprList join_exprs2(Location* loc,
                         ExprList* expr1,
                         Expr* expr2);

Result parse_const(Type type,
                       LiteralType literal_type,
                       const char* s,
                       const char* end,
                       Const* out);
void dup_text_list(TextList* text_list, char** out_data, size_t* out_size);

bool is_empty_signature(const FuncSignature* sig);

void append_implicit_func_declaration(Location*,
                                      Module*,
                                      FuncDeclaration*);

struct BinaryErrorCallbackData {
  Location* loc;
  AstLexer* lexer;
  AstParser* parser;
};

static bool on_read_binary_error(uint32_t offset, const char* error,
                                 void* user_data);

#define wabt_ast_parser_lex ast_lexer_lex
#define wabt_ast_parser_error ast_parser_error


#line 235 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "ast-parser-gen.hh".  */
#ifndef YY_WABT_AST_PARSER_SRC_PREBUILT_AST_PARSER_GEN_HH_INCLUDED
# define YY_WABT_AST_PARSER_SRC_PREBUILT_AST_PARSER_GEN_HH_INCLUDED
/* Debug traces.  */
#ifndef WABT_AST_PARSER_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define WABT_AST_PARSER_DEBUG 1
#  else
#   define WABT_AST_PARSER_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define WABT_AST_PARSER_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined WABT_AST_PARSER_DEBUG */
#if WABT_AST_PARSER_DEBUG
extern int wabt_ast_parser_debug;
#endif

/* Token type.  */
#ifndef WABT_AST_PARSER_TOKENTYPE
# define WABT_AST_PARSER_TOKENTYPE
  enum wabt_ast_parser_tokentype
  {
    WABT_TOKEN_TYPE_EOF = 0,
    WABT_TOKEN_TYPE_LPAR = 258,
    WABT_TOKEN_TYPE_RPAR = 259,
    WABT_TOKEN_TYPE_NAT = 260,
    WABT_TOKEN_TYPE_INT = 261,
    WABT_TOKEN_TYPE_FLOAT = 262,
    WABT_TOKEN_TYPE_TEXT = 263,
    WABT_TOKEN_TYPE_VAR = 264,
    WABT_TOKEN_TYPE_VALUE_TYPE = 265,
    WABT_TOKEN_TYPE_ANYFUNC = 266,
    WABT_TOKEN_TYPE_MUT = 267,
    WABT_TOKEN_TYPE_NOP = 268,
    WABT_TOKEN_TYPE_DROP = 269,
    WABT_TOKEN_TYPE_BLOCK = 270,
    WABT_TOKEN_TYPE_END = 271,
    WABT_TOKEN_TYPE_IF = 272,
    WABT_TOKEN_TYPE_THEN = 273,
    WABT_TOKEN_TYPE_ELSE = 274,
    WABT_TOKEN_TYPE_LOOP = 275,
    WABT_TOKEN_TYPE_BR = 276,
    WABT_TOKEN_TYPE_BR_IF = 277,
    WABT_TOKEN_TYPE_BR_TABLE = 278,
    WABT_TOKEN_TYPE_CALL = 279,
    WABT_TOKEN_TYPE_CALL_IMPORT = 280,
    WABT_TOKEN_TYPE_CALL_INDIRECT = 281,
    WABT_TOKEN_TYPE_RETURN = 282,
    WABT_TOKEN_TYPE_GET_LOCAL = 283,
    WABT_TOKEN_TYPE_SET_LOCAL = 284,
    WABT_TOKEN_TYPE_TEE_LOCAL = 285,
    WABT_TOKEN_TYPE_GET_GLOBAL = 286,
    WABT_TOKEN_TYPE_SET_GLOBAL = 287,
    WABT_TOKEN_TYPE_LOAD = 288,
    WABT_TOKEN_TYPE_STORE = 289,
    WABT_TOKEN_TYPE_OFFSET_EQ_NAT = 290,
    WABT_TOKEN_TYPE_ALIGN_EQ_NAT = 291,
    WABT_TOKEN_TYPE_CONST = 292,
    WABT_TOKEN_TYPE_UNARY = 293,
    WABT_TOKEN_TYPE_BINARY = 294,
    WABT_TOKEN_TYPE_COMPARE = 295,
    WABT_TOKEN_TYPE_CONVERT = 296,
    WABT_TOKEN_TYPE_SELECT = 297,
    WABT_TOKEN_TYPE_UNREACHABLE = 298,
    WABT_TOKEN_TYPE_CURRENT_MEMORY = 299,
    WABT_TOKEN_TYPE_GROW_MEMORY = 300,
    WABT_TOKEN_TYPE_FUNC = 301,
    WABT_TOKEN_TYPE_START = 302,
    WABT_TOKEN_TYPE_TYPE = 303,
    WABT_TOKEN_TYPE_PARAM = 304,
    WABT_TOKEN_TYPE_RESULT = 305,
    WABT_TOKEN_TYPE_LOCAL = 306,
    WABT_TOKEN_TYPE_GLOBAL = 307,
    WABT_TOKEN_TYPE_MODULE = 308,
    WABT_TOKEN_TYPE_TABLE = 309,
    WABT_TOKEN_TYPE_ELEM = 310,
    WABT_TOKEN_TYPE_MEMORY = 311,
    WABT_TOKEN_TYPE_DATA = 312,
    WABT_TOKEN_TYPE_OFFSET = 313,
    WABT_TOKEN_TYPE_IMPORT = 314,
    WABT_TOKEN_TYPE_EXPORT = 315,
    WABT_TOKEN_TYPE_REGISTER = 316,
    WABT_TOKEN_TYPE_INVOKE = 317,
    WABT_TOKEN_TYPE_GET = 318,
    WABT_TOKEN_TYPE_ASSERT_MALFORMED = 319,
    WABT_TOKEN_TYPE_ASSERT_INVALID = 320,
    WABT_TOKEN_TYPE_ASSERT_UNLINKABLE = 321,
    WABT_TOKEN_TYPE_ASSERT_RETURN = 322,
    WABT_TOKEN_TYPE_ASSERT_RETURN_CANONICAL_NAN = 323,
    WABT_TOKEN_TYPE_ASSERT_RETURN_ARITHMETIC_NAN = 324,
    WABT_TOKEN_TYPE_ASSERT_TRAP = 325,
    WABT_TOKEN_TYPE_ASSERT_EXHAUSTION = 326,
    WABT_TOKEN_TYPE_INPUT = 327,
    WABT_TOKEN_TYPE_OUTPUT = 328,
    WABT_TOKEN_TYPE_LOW = 329
  };
#endif

/* Value type.  */
#if ! defined WABT_AST_PARSER_STYPE && ! defined WABT_AST_PARSER_STYPE_IS_DECLARED
typedef ::wabt::Token WABT_AST_PARSER_STYPE;
# define WABT_AST_PARSER_STYPE_IS_TRIVIAL 1
# define WABT_AST_PARSER_STYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined WABT_AST_PARSER_LTYPE && ! defined WABT_AST_PARSER_LTYPE_IS_DECLARED
typedef struct WABT_AST_PARSER_LTYPE WABT_AST_PARSER_LTYPE;
struct WABT_AST_PARSER_LTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define WABT_AST_PARSER_LTYPE_IS_DECLARED 1
# define WABT_AST_PARSER_LTYPE_IS_TRIVIAL 1
#endif



int wabt_ast_parser_parse (::wabt::AstLexer* lexer, ::wabt::AstParser* parser);

#endif /* !YY_WABT_AST_PARSER_SRC_PREBUILT_AST_PARSER_GEN_HH_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 383 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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
         || (defined WABT_AST_PARSER_LTYPE_IS_TRIVIAL && WABT_AST_PARSER_LTYPE_IS_TRIVIAL \
             && defined WABT_AST_PARSER_STYPE_IS_TRIVIAL && WABT_AST_PARSER_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   797

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  75
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  63
/* YYNRULES -- Number of rules.  */
#define YYNRULES  172
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  405

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   329

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
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
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74
};

#if WABT_AST_PARSER_DEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   293,   293,   299,   309,   310,   314,   332,   333,   339,
     342,   347,   354,   357,   358,   363,   370,   378,   384,   390,
     395,   402,   408,   419,   423,   427,   434,   439,   446,   447,
     453,   454,   457,   461,   462,   466,   467,   477,   478,   489,
     490,   491,   494,   497,   500,   503,   506,   509,   512,   515,
     518,   521,   524,   527,   530,   533,   536,   539,   542,   545,
     558,   561,   564,   567,   570,   573,   578,   583,   588,   593,
     601,   610,   614,   617,   622,   627,   637,   641,   645,   649,
     653,   657,   664,   665,   673,   674,   682,   687,   688,   694,
     700,   710,   716,   722,   732,   784,   794,   801,   809,   819,
     822,   826,   833,   845,   853,   875,   882,   894,   902,   923,
     945,   953,   966,   974,   982,   988,   994,  1002,  1007,  1015,
    1023,  1029,  1035,  1044,  1052,  1057,  1062,  1067,  1074,  1081,
    1085,  1088,  1100,  1105,  1114,  1118,  1121,  1128,  1137,  1154,
    1171,  1181,  1187,  1193,  1199,  1232,  1242,  1262,  1273,  1301,
    1306,  1314,  1324,  1334,  1340,  1346,  1352,  1358,  1364,  1369,
    1374,  1380,  1389,  1394,  1395,  1400,  1409,  1410,  1417,  1429,
    1430,  1437,  1503
};
#endif

#if WABT_AST_PARSER_DEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"EOF\"", "error", "$undefined", "\"(\"", "\")\"", "NAT", "INT",
  "FLOAT", "TEXT", "VAR", "VALUE_TYPE", "ANYFUNC", "MUT", "NOP", "DROP",
  "BLOCK", "END", "IF", "THEN", "ELSE", "LOOP", "BR", "BR_IF", "BR_TABLE",
  "CALL", "CALL_IMPORT", "CALL_INDIRECT", "RETURN", "GET_LOCAL",
  "SET_LOCAL", "TEE_LOCAL", "GET_GLOBAL", "SET_GLOBAL", "LOAD", "STORE",
  "OFFSET_EQ_NAT", "ALIGN_EQ_NAT", "CONST", "UNARY", "BINARY", "COMPARE",
  "CONVERT", "SELECT", "UNREACHABLE", "CURRENT_MEMORY", "GROW_MEMORY",
  "FUNC", "START", "TYPE", "PARAM", "RESULT", "LOCAL", "GLOBAL", "MODULE",
  "TABLE", "ELEM", "MEMORY", "DATA", "OFFSET", "IMPORT", "EXPORT",
  "REGISTER", "INVOKE", "GET", "ASSERT_MALFORMED", "ASSERT_INVALID",
  "ASSERT_UNLINKABLE", "ASSERT_RETURN", "ASSERT_RETURN_CANONICAL_NAN",
  "ASSERT_RETURN_ARITHMETIC_NAN", "ASSERT_TRAP", "ASSERT_EXHAUSTION",
  "INPUT", "OUTPUT", "LOW", "$accept", "non_empty_text_list", "text_list",
  "quoted_text", "value_type_list", "elem_type", "global_type",
  "func_type", "func_sig", "table_sig", "memory_sig", "limits", "type_use",
  "nat", "literal", "var", "var_list", "bind_var_opt", "bind_var",
  "labeling_opt", "offset_opt", "align_opt", "instr", "plain_instr",
  "block_instr", "block", "expr", "expr1", "if_", "instr_list",
  "expr_list", "const_expr", "func_fields", "func_body", "func_info",
  "func", "offset", "elem", "table", "data", "memory", "global",
  "import_kind", "import", "inline_import", "export_kind", "export",
  "inline_export_opt", "inline_export", "type_def", "start",
  "module_fields", "raw_module", "module", "script_var_opt", "action",
  "assertion", "cmd", "cmd_list", "const", "const_list", "script",
  "script_start", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329
};
# endif

#define YYPACT_NINF -274

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-274)))

#define YYTABLE_NINF -30

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -274,    41,  -274,    46,    68,  -274,  -274,  -274,  -274,  -274,
    -274,    60,    73,    82,    82,   121,   121,   121,   123,   123,
     123,   142,   123,  -274,   166,  -274,  -274,    82,  -274,    73,
      73,   128,    73,    73,    73,   104,  -274,   184,   205,     0,
      73,    73,    73,  -274,   161,   221,   213,  -274,   243,   245,
     254,   255,   228,  -274,  -274,   256,   263,   264,  -274,  -274,
     116,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,  -274,  -274,   231,  -274,  -274,  -274,  -274,   153,  -274,
    -274,  -274,  -274,  -274,    60,    98,    69,    60,    60,   111,
      60,   111,    73,    73,  -274,   237,   408,  -274,  -274,  -274,
     265,   224,   267,   204,    58,   270,   329,   271,  -274,  -274,
     272,   271,   166,    73,   273,  -274,  -274,  -274,   275,   288,
    -274,  -274,    60,    60,    60,    98,    98,  -274,    98,    98,
    -274,    98,    98,    98,    98,    98,   242,   242,   237,  -274,
    -274,  -274,  -274,  -274,  -274,  -274,  -274,   441,   474,  -274,
    -274,  -274,  -274,  -274,  -274,   276,   278,   507,  -274,   279,
    -274,   280,     6,  -274,   474,    89,    89,   186,   284,   114,
    -274,    60,    60,    60,   474,   287,   289,  -274,   118,   170,
     284,   284,   291,   166,   283,   292,   301,    34,   302,  -274,
      98,    60,  -274,    60,    73,    73,  -274,  -274,  -274,  -274,
    -274,  -274,    98,  -274,  -274,  -274,  -274,  -274,  -274,  -274,
    -274,   247,   247,  -274,   612,   303,   752,  -274,  -274,   203,
     309,   319,   573,   441,   320,   206,   330,  -274,   282,  -274,
     331,   328,   337,   474,   341,   343,   284,  -274,   361,   371,
    -274,  -274,  -274,   372,   287,  -274,  -274,   197,  -274,  -274,
     166,   373,  -274,   376,   238,   377,  -274,    52,   378,    98,
      98,    98,    98,  -274,   379,   173,   355,   174,   175,   369,
      73,   380,   375,   370,    48,   384,   214,  -274,  -274,  -274,
    -274,  -274,  -274,  -274,  -274,   387,  -274,  -274,   389,  -274,
    -274,   390,  -274,  -274,  -274,   388,  -274,  -274,    99,  -274,
    -274,  -274,  -274,   406,  -274,  -274,   166,  -274,    60,    60,
      60,    60,  -274,   420,   422,   423,   429,  -274,   441,  -274,
     453,   540,   540,   455,   456,  -274,  -274,    60,    60,    60,
      60,   188,   189,  -274,  -274,  -274,  -274,   686,   463,  -274,
     472,   486,   278,    89,   284,   284,  -274,  -274,  -274,  -274,
    -274,   441,   651,  -274,  -274,   540,  -274,  -274,  -274,   474,
    -274,   489,  -274,   217,   474,   719,   287,  -274,   495,   505,
     519,   521,   522,   528,  -274,  -274,   477,   492,   552,   554,
     474,  -274,  -274,  -274,  -274,  -274,  -274,  -274,    60,  -274,
    -274,   556,   561,  -274,   193,   557,   572,  -274,   474,   570,
     587,   474,  -274,   588,  -274
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
     166,   171,   172,     0,     0,   148,   164,   162,   163,   167,
       1,    30,     0,   149,   149,     0,     0,     0,     0,     0,
       0,     0,     0,    32,   135,    31,     6,   149,   150,     0,
       0,     0,     0,     0,     0,     0,   169,     0,     0,     0,
       0,     0,     0,     2,     0,     0,     0,   169,     0,     0,
       0,     0,     0,   158,   159,     0,     0,     0,   147,     3,
       0,   146,   140,   141,   138,   142,   139,   137,   144,   145,
     136,   143,   165,     0,   152,   153,   154,   155,     0,   157,
     170,   156,   160,   161,    30,     0,     0,    30,    30,     0,
      30,     0,     0,     0,   151,     0,    82,    22,    27,    26,
       0,     0,     0,     0,     0,   129,     0,     0,   100,    28,
     129,     0,     4,     0,     0,    23,    24,    25,     0,     0,
      43,    44,    33,    33,    33,     0,     0,    28,     0,     0,
      49,     0,     0,     0,     0,     0,    35,    35,     0,    60,
      61,    62,    63,    45,    42,    64,    65,    82,    82,    39,
      40,    41,    91,    94,    87,     0,    13,    82,   134,    13,
     132,     0,     0,    10,    82,     0,     0,     0,     0,     0,
     130,    33,    33,    33,    82,    84,     0,    28,     0,     0,
       0,     0,   130,     4,     5,     0,     0,     0,     0,   168,
       0,     7,     7,     7,     0,     0,    34,     7,     7,     7,
      46,    47,     0,    50,    51,    52,    53,    54,    55,    56,
      36,    37,    37,    59,     0,     0,     0,    83,    98,     0,
       0,     0,     0,    82,     0,     0,     0,   133,     0,    86,
       0,     0,     0,    82,     0,     0,    19,     9,     0,     0,
       7,     7,     7,     0,    84,    72,    71,     0,   102,    29,
       4,     0,    18,     0,     0,     0,   106,     0,     0,     0,
       0,     0,     0,   128,     0,     0,     0,     0,     0,     0,
       0,     0,    82,     0,     0,     0,    48,    38,    57,    58,
      96,     7,     7,   119,   118,     0,    97,    12,     0,   111,
     122,     0,   120,    17,    20,     0,   103,    73,     0,    74,
      99,    85,   101,     0,   121,   107,     4,   105,    30,    30,
      30,    30,   117,     0,     0,     0,     0,    21,    82,     8,
       0,    82,    82,     0,     0,   131,    70,    33,    33,    33,
      33,     0,     0,    95,    11,   110,    28,     0,     0,    75,
       0,     0,    13,     0,     0,     0,   124,   127,   125,   126,
      89,    82,     0,    88,    92,    82,   123,    66,    68,    82,
      67,    14,    16,     0,    82,     0,    81,   109,     0,     0,
       0,     0,     0,     0,    90,    93,     0,     0,     0,     0,
      82,    80,   108,   113,   112,   116,   114,   115,    33,     7,
     104,    77,     0,    69,     0,     0,    79,    15,    82,     0,
       0,    82,    76,     0,    78
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -274,   574,  -156,    -8,  -181,   374,  -149,   516,  -146,  -155,
    -150,  -145,  -151,  -140,   470,    26,  -118,   -45,   -11,  -116,
     483,   416,  -274,  -104,  -274,  -147,   -88,  -274,  -274,  -144,
     386,  -136,  -268,  -273,  -110,  -274,   -37,  -274,  -274,  -274,
    -274,  -274,  -274,  -274,    39,  -274,  -274,   527,    43,  -274,
    -274,  -274,   125,  -274,    44,   219,  -274,  -274,  -274,  -274,
     584,  -274,  -274
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   184,   185,    27,   272,   238,   164,   102,   220,   234,
     251,   235,   147,    99,   118,   249,   178,    24,   196,   197,
     211,   278,   148,   149,   150,   273,   151,   176,   339,   152,
     245,   230,   153,   154,   155,    62,   109,    63,    64,    65,
      66,    67,   258,    68,   156,   188,    69,   169,   157,    70,
      71,    45,     5,     6,    29,     7,     8,     9,     1,    80,
      52,     2,     3
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      25,   108,   175,   108,   217,   221,   223,   198,   199,   202,
     265,   267,   268,   226,   239,   175,   232,   233,   228,   108,
     229,    47,    48,   108,    49,    50,    51,   255,   236,   236,
     229,   253,    55,    56,    57,   252,   252,   215,   243,    96,
     236,   236,   104,   105,     4,   110,    10,   224,   353,   354,
     350,   274,   275,    11,   112,   240,   241,   242,    30,   247,
     298,   162,    13,    14,   328,   194,   195,   329,   163,    23,
     177,    46,   101,    25,   183,   103,    25,    25,    23,    25,
     259,    26,   375,   374,   113,   114,   260,   244,   261,   229,
     262,    28,   231,   297,   303,   299,   294,   291,   308,   163,
     331,   332,   337,    97,   309,   186,   310,    98,   311,   319,
     175,   100,   175,   285,   106,   107,    97,   111,   175,    97,
      98,    11,   248,    97,    31,   237,    35,    98,   326,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      32,    33,    34,   165,   168,    39,    40,   166,   170,   180,
     341,   200,   201,   182,   203,   204,   244,   205,   206,   207,
     208,   209,    84,    85,    86,    58,    13,    14,    87,    59,
      88,    89,    90,    91,    43,    92,    93,   318,   321,   322,
     266,    11,   269,   319,   319,   319,   270,   271,    53,   372,
      95,   370,   361,   362,   371,   373,   369,   397,   319,   319,
     252,   302,    97,   319,   236,   236,    98,   101,   394,    54,
     338,   357,   358,   359,   360,   376,   264,    72,   363,   -29,
     379,   378,    97,   -29,    60,    61,    98,   250,   276,   194,
     195,    78,    79,   175,    78,    94,   392,    36,    37,    38,
      41,    42,   115,   116,   117,   194,   195,    74,   175,    75,
     366,   190,   281,   282,   400,   281,   282,   403,    76,    77,
      81,   175,   324,   342,   343,   344,   345,    82,    83,   158,
     159,   160,   393,   167,   106,   179,   187,   210,   381,   189,
     218,   219,   225,   277,   227,   313,   314,   315,   316,    97,
     216,    59,   288,   246,   254,   306,   256,    25,    25,    25,
      25,   120,   121,   171,   257,   172,   263,   280,   173,   125,
     126,   127,   128,   283,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   284,   286,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   287,   289,   190,   191,   192,   193,
     228,   290,   120,   121,   171,   292,   172,   194,   195,   173,
     125,   126,   127,   128,   237,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   295,   320,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   296,   300,   304,   216,   323,
     305,   307,   312,   317,   325,   319,   327,   174,   120,   121,
     122,   333,   123,   334,   335,   124,   125,   126,   127,   128,
     330,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     340,   119,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   120,   121,   122,   346,   123,   347,   348,   124,   125,
     126,   127,   128,   349,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   336,   214,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   120,   121,   122,   351,   123,   355,
     356,   124,   125,   126,   127,   128,   365,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   367,   216,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   120,   121,   122,
     368,   123,   377,   388,   124,   125,   126,   127,   128,   382,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   383,
     222,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     120,   121,   122,   384,   123,   385,   386,   124,   125,   126,
     127,   128,   387,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   389,   352,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   120,   121,   122,   390,   123,   391,   395,
     124,   125,   126,   127,   128,   396,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   399,   398,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   120,   121,   171,   401,
     172,   402,   404,   173,   125,   126,   127,   128,    44,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   213,   293,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   161,
     212,   190,   191,   192,   193,   120,   121,   171,   279,   172,
     301,    73,   173,   125,   126,   127,   128,   181,   129,   130,
     131,   132,   133,   134,   135,   136,   137,     0,     0,   138,
     139,   140,   141,   142,   143,   144,   145,   146,     0,     0,
       0,   191,   192,   193,   120,   121,   171,     0,   172,     0,
       0,   173,   125,   126,   127,   128,     0,   129,   130,   131,
     132,   133,   134,   135,   136,   137,     0,     0,   138,   139,
     140,   141,   142,   143,   144,   145,   146,     0,     0,   120,
     121,   171,   193,   172,   364,     0,   173,   125,   126,   127,
     128,     0,   129,   130,   131,   132,   133,   134,   135,   136,
     137,     0,     0,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   120,   121,   171,     0,   172,   380,     0,   173,
     125,   126,   127,   128,     0,   129,   130,   131,   132,   133,
     134,   135,   136,   137,     0,     0,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   120,   121,   171,     0,   172,
       0,     0,   173,   125,   126,   127,   128,     0,   129,   130,
     131,   132,   133,   134,   135,   136,   137,     0,     0,   138,
     139,   140,   141,   142,   143,   144,   145,   146
};

static const yytype_int16 yycheck[] =
{
      11,    89,   106,    91,   148,   156,   157,   123,   124,   127,
     191,   192,   193,   159,   169,   119,   165,   166,    12,   107,
     164,    29,    30,   111,    32,    33,    34,   183,   168,   169,
     174,   181,    40,    41,    42,   180,   181,   147,   174,    84,
     180,   181,    87,    88,     3,    90,     0,   157,   321,   322,
     318,   198,   199,    53,    91,   171,   172,   173,    14,   177,
     241,     3,    62,    63,    16,    59,    60,    19,    10,     9,
     107,    27,     3,    84,   111,    86,    87,    88,     9,    90,
      46,     8,   355,   351,    92,    93,    52,   175,    54,   233,
      56,     9,     3,   240,   250,   242,   236,   233,    46,    10,
     281,   282,     3,     5,    52,   113,    54,     9,    56,    10,
     214,    85,   216,   223,     3,    89,     5,    91,   222,     5,
       9,    53,     4,     5,     3,    11,     3,     9,   272,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      15,    16,    17,   104,   105,     3,    21,   104,   105,   110,
     306,   125,   126,   110,   128,   129,   244,   131,   132,   133,
     134,   135,    46,    47,    48,     4,    62,    63,    52,     8,
      54,    55,    56,    57,     8,    59,    60,     4,     4,     4,
     191,    53,   193,    10,    10,    10,   194,   195,     4,   344,
      37,   342,     4,     4,   343,   345,   342,     4,    10,    10,
     345,     4,     5,    10,   344,   345,     9,     3,   389,     4,
     298,   327,   328,   329,   330,   359,   190,     4,   336,     5,
     364,     4,     5,     9,     3,     4,     9,    57,   202,    59,
      60,     3,     4,   337,     3,     4,   380,    18,    19,    20,
      21,    22,     5,     6,     7,    59,    60,     4,   352,     4,
     338,    48,    49,    50,   398,    49,    50,   401,     4,     4,
       4,   365,   270,   308,   309,   310,   311,     4,     4,     4,
      46,     4,   388,     3,     3,     3,     3,    35,   366,     4,
       4,     3,     3,    36,     4,   259,   260,   261,   262,     5,
       3,     8,    10,     4,     3,    57,     4,   308,   309,   310,
     311,    13,    14,    15,     3,    17,     4,     4,    20,    21,
      22,    23,    24,     4,    26,    27,    28,    29,    30,    31,
      32,    33,    34,     4,     4,    37,    38,    39,    40,    41,
      42,    43,    44,    45,     4,     4,    48,    49,    50,    51,
      12,     4,    13,    14,    15,     4,    17,    59,    60,    20,
      21,    22,    23,    24,    11,    26,    27,    28,    29,    30,
      31,    32,    33,    34,     3,    10,    37,    38,    39,    40,
      41,    42,    43,    44,    45,     4,     4,     4,     3,    10,
       4,     4,     4,     4,     4,    10,    16,    58,    13,    14,
      15,     4,    17,     4,     4,    20,    21,    22,    23,    24,
      16,    26,    27,    28,    29,    30,    31,    32,    33,    34,
       4,     3,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    13,    14,    15,     4,    17,     4,     4,    20,    21,
      22,    23,    24,     4,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    55,     3,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    13,    14,    15,     4,    17,     4,
       4,    20,    21,    22,    23,    24,     3,    26,    27,    28,
      29,    30,    31,    32,    33,    34,     4,     3,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    13,    14,    15,
       4,    17,     3,    16,    20,    21,    22,    23,    24,     4,
      26,    27,    28,    29,    30,    31,    32,    33,    34,     4,
       3,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      13,    14,    15,     4,    17,     4,     4,    20,    21,    22,
      23,    24,     4,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    50,     3,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    13,    14,    15,     4,    17,     4,     3,
      20,    21,    22,    23,    24,     4,    26,    27,    28,    29,
      30,    31,    32,    33,    34,     3,    19,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    13,    14,    15,    19,
      17,     4,     4,    20,    21,    22,    23,    24,    24,    26,
      27,    28,    29,    30,    31,    32,    33,    34,   138,   235,
      37,    38,    39,    40,    41,    42,    43,    44,    45,   103,
     137,    48,    49,    50,    51,    13,    14,    15,   212,    17,
     244,    47,    20,    21,    22,    23,    24,   110,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    -1,
      -1,    49,    50,    51,    13,    14,    15,    -1,    17,    -1,
      -1,    20,    21,    22,    23,    24,    -1,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    -1,    13,
      14,    15,    51,    17,    18,    -1,    20,    21,    22,    23,
      24,    -1,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    13,    14,    15,    -1,    17,    18,    -1,    20,
      21,    22,    23,    24,    -1,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    13,    14,    15,    -1,    17,
      -1,    -1,    20,    21,    22,    23,    24,    -1,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    37,
      38,    39,    40,    41,    42,    43,    44,    45
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   133,   136,   137,     3,   127,   128,   130,   131,   132,
       0,    53,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,     9,    92,    93,     8,    78,     9,   129,
     129,     3,   127,   127,   127,     3,   130,   130,   130,     3,
     127,   130,   130,     8,    76,   126,   129,    78,    78,    78,
      78,    78,   135,     4,     4,    78,    78,    78,     4,     8,
       3,     4,   110,   112,   113,   114,   115,   116,   118,   121,
     124,   125,     4,   135,     4,     4,     4,     4,     3,     4,
     134,     4,     4,     4,    46,    47,    48,    52,    54,    55,
      56,    57,    59,    60,     4,    37,    92,     5,     9,    88,
      90,     3,    82,    93,    92,    92,     3,    90,   101,   111,
      92,    90,   111,    78,    78,     5,     6,     7,    89,     3,
      13,    14,    15,    17,    20,    21,    22,    23,    24,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    87,    97,    98,
      99,   101,   104,   107,   108,   109,   119,   123,     4,    46,
       4,    82,     3,    10,    81,   119,   123,     3,   119,   122,
     123,    15,    17,    20,    58,    98,   102,   111,    91,     3,
     119,   122,   123,   111,    76,    77,    78,     3,   120,     4,
      48,    49,    50,    51,    59,    60,    93,    94,    94,    94,
      90,    90,    91,    90,    90,    90,    90,    90,    90,    90,
      35,    95,    95,    89,     3,   109,     3,   104,     4,     3,
      83,    87,     3,    87,   109,     3,    83,     4,    12,   104,
     106,     3,    81,    81,    84,    86,    88,    11,    80,    84,
      94,    94,    94,   106,   101,   105,     4,    91,     4,    90,
      57,    85,    86,    85,     3,    77,     4,     3,   117,    46,
      52,    54,    56,     4,    90,    79,    93,    79,    79,    93,
      78,    78,    79,   100,   100,   100,    90,    36,    96,    96,
       4,    49,    50,     4,     4,   109,     4,     4,    10,     4,
       4,   106,     4,    80,    88,     3,     4,   100,    79,   100,
       4,   105,     4,    77,     4,     4,    57,     4,    46,    52,
      54,    56,     4,    90,    90,    90,    90,     4,     4,    10,
      10,     4,     4,    10,    78,     4,   104,    16,    16,    19,
      16,    79,    79,     4,     4,     4,    55,     3,   101,   103,
       4,    77,    92,    92,    92,    92,     4,     4,     4,     4,
     107,     4,     3,   108,   108,     4,     4,    94,    94,    94,
      94,     4,     4,    91,    18,     3,   101,     4,     4,    83,
      87,    81,    84,    85,   107,   108,   104,     3,     4,   104,
      18,   101,     4,     4,     4,     4,     4,     4,    16,    50,
       4,     4,   104,    94,    79,     3,     4,     4,    19,     3,
     104,    19,     4,   104,     4
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    75,    76,    76,    77,    77,    78,    79,    79,    80,
      81,    81,    82,    83,    83,    83,    83,    84,    85,    86,
      86,    87,    88,    89,    89,    89,    90,    90,    91,    91,
      92,    92,    93,    94,    94,    95,    95,    96,    96,    97,
      97,    97,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    99,    99,    99,    99,
     100,   101,   102,   102,   102,   102,   103,   103,   103,   103,
     103,   103,   104,   104,   105,   105,   106,   107,   107,   107,
     107,   108,   108,   108,   109,   110,   110,   110,   110,   111,
     111,   112,   112,   113,   113,   114,   114,   115,   115,   115,
     116,   116,   117,   117,   117,   117,   117,   118,   118,   118,
     118,   118,   118,   119,   120,   120,   120,   120,   121,   122,
     122,   123,   124,   124,   125,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   127,   127,   128,   129,
     129,   130,   130,   131,   131,   131,   131,   131,   131,   131,
     131,   131,   132,   132,   132,   132,   133,   133,   134,   135,
     135,   136,   137
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     0,     1,     1,     0,     2,     1,
       1,     4,     4,     0,     4,     8,     4,     2,     1,     1,
       2,     4,     1,     1,     1,     1,     1,     1,     0,     2,
       0,     1,     1,     0,     1,     0,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     3,     1,
       2,     2,     2,     2,     2,     2,     2,     3,     3,     2,
       1,     1,     1,     1,     1,     1,     5,     5,     5,     8,
       2,     3,     2,     3,     3,     4,     8,     4,     9,     5,
       3,     2,     0,     2,     0,     2,     1,     1,     5,     5,
       6,     1,     5,     6,     1,     7,     6,     6,     5,     4,
       1,     6,     5,     6,    10,     6,     5,     6,     9,     8,
       7,     6,     5,     5,     5,     5,     5,     6,     6,     6,
       6,     6,     6,     5,     4,     4,     4,     4,     5,     0,
       1,     4,     4,     5,     4,     0,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     5,     5,     1,     0,
       1,     6,     5,     5,     5,     5,     5,     5,     4,     4,
       5,     5,     1,     1,     1,     5,     0,     2,     4,     0,
       2,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
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
      yyerror (&yylloc, lexer, parser, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if WABT_AST_PARSER_DEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined WABT_AST_PARSER_LTYPE_IS_TRIVIAL && WABT_AST_PARSER_LTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, lexer, parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, ::wabt::AstLexer* lexer, ::wabt::AstParser* parser)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  YYUSE (lexer);
  YYUSE (parser);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, ::wabt::AstLexer* lexer, ::wabt::AstParser* parser)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, lexer, parser);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, ::wabt::AstLexer* lexer, ::wabt::AstParser* parser)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , lexer, parser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, lexer, parser); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !WABT_AST_PARSER_DEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !WABT_AST_PARSER_DEBUG */


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
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
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
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
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
            /* Fall through.  */
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

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
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
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

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
      int yyn = yypact[*yyssp];
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
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
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
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
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, ::wabt::AstLexer* lexer, ::wabt::AstParser* parser)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (lexer);
  YYUSE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yytype)
    {
          case 5: /* NAT  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1648 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 6: /* INT  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1654 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 7: /* FLOAT  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1660 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 8: /* TEXT  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1666 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 9: /* VAR  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1672 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 35: /* OFFSET_EQ_NAT  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1678 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 36: /* ALIGN_EQ_NAT  */
#line 250 "src/ast-parser.y" /* yacc.c:1257  */
      {}
#line 1684 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 76: /* non_empty_text_list  */
#line 277 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_text_list(&((*yyvaluep).text_list)); }
#line 1690 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 77: /* text_list  */
#line 277 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_text_list(&((*yyvaluep).text_list)); }
#line 1696 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 78: /* quoted_text  */
#line 251 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_string_slice(&((*yyvaluep).text)); }
#line 1702 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 79: /* value_type_list  */
#line 278 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).types); }
#line 1708 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 81: /* global_type  */
#line 270 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).global); }
#line 1714 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 82: /* func_type  */
#line 268 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).func_sig); }
#line 1720 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 83: /* func_sig  */
#line 268 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).func_sig); }
#line 1726 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 85: /* memory_sig  */
#line 273 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).memory); }
#line 1732 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 87: /* type_use  */
#line 279 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_var(&((*yyvaluep).var)); }
#line 1738 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 89: /* literal  */
#line 252 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_string_slice(&((*yyvaluep).literal).text); }
#line 1744 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 90: /* var  */
#line 279 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_var(&((*yyvaluep).var)); }
#line 1750 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 91: /* var_list  */
#line 280 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).vars); }
#line 1756 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 92: /* bind_var_opt  */
#line 251 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_string_slice(&((*yyvaluep).text)); }
#line 1762 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 93: /* bind_var  */
#line 251 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_string_slice(&((*yyvaluep).text)); }
#line 1768 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 94: /* labeling_opt  */
#line 251 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_string_slice(&((*yyvaluep).text)); }
#line 1774 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 97: /* instr  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1780 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 98: /* plain_instr  */
#line 264 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1786 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 99: /* block_instr  */
#line 264 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expr); }
#line 1792 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 100: /* block  */
#line 254 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).block); }
#line 1798 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 101: /* expr  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1804 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 102: /* expr1  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1810 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 103: /* if_  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1816 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 104: /* instr_list  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1822 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 105: /* expr_list  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1828 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 106: /* const_expr  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1834 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 107: /* func_fields  */
#line 266 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_func_fields(((*yyvaluep).func_fields)); }
#line 1840 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 108: /* func_body  */
#line 266 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_func_fields(((*yyvaluep).func_fields)); }
#line 1846 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 109: /* func_info  */
#line 267 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).func); }
#line 1852 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 110: /* func  */
#line 261 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).exported_func); }
#line 1858 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 111: /* offset  */
#line 265 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_expr_list(((*yyvaluep).expr_list).first); }
#line 1864 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 112: /* elem  */
#line 259 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).elem_segment); }
#line 1870 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 113: /* table  */
#line 263 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).exported_table); }
#line 1876 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 114: /* data  */
#line 258 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).data_segment); }
#line 1882 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 115: /* memory  */
#line 262 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).exported_memory); }
#line 1888 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 117: /* import_kind  */
#line 271 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).import); }
#line 1894 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 118: /* import  */
#line 271 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).import); }
#line 1900 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 119: /* inline_import  */
#line 271 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).import); }
#line 1906 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 120: /* export_kind  */
#line 260 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).export_); }
#line 1912 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 121: /* export  */
#line 260 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).export_); }
#line 1918 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 122: /* inline_export_opt  */
#line 272 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).optional_export); }
#line 1924 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 123: /* inline_export  */
#line 272 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).optional_export); }
#line 1930 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 124: /* type_def  */
#line 269 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).func_type); }
#line 1936 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 125: /* start  */
#line 279 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_var(&((*yyvaluep).var)); }
#line 1942 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 126: /* module_fields  */
#line 274 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).module); }
#line 1948 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 127: /* raw_module  */
#line 275 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).raw_module); }
#line 1954 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 128: /* module  */
#line 274 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).module); }
#line 1960 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 129: /* script_var_opt  */
#line 279 "src/ast-parser.y" /* yacc.c:1257  */
      { destroy_var(&((*yyvaluep).var)); }
#line 1966 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 130: /* action  */
#line 253 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).action); }
#line 1972 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 131: /* assertion  */
#line 255 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).command); }
#line 1978 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 132: /* cmd  */
#line 255 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).command); }
#line 1984 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 133: /* cmd_list  */
#line 256 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).commands); }
#line 1990 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 135: /* const_list  */
#line 257 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).consts); }
#line 1996 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;

    case 136: /* script  */
#line 276 "src/ast-parser.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).script); }
#line 2002 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1257  */
        break;


      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (::wabt::AstLexer* lexer, ::wabt::AstParser* parser)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined WABT_AST_PARSER_LTYPE_IS_TRIVIAL && WABT_AST_PARSER_LTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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
      yychar = yylex (&yylval, &yylloc, lexer, parser);
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
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
| yyreduce -- Do a reduction.  |
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

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 293 "src/ast-parser.y" /* yacc.c:1646  */
    {
      TextListNode* node = new TextListNode();
      DUPTEXT(node->text, (yyvsp[0].text));
      node->next = nullptr;
      (yyval.text_list).first = (yyval.text_list).last = node;
    }
#line 2301 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 3:
#line 299 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.text_list) = (yyvsp[-1].text_list);
      TextListNode* node = new TextListNode();
      DUPTEXT(node->text, (yyvsp[0].text));
      node->next = nullptr;
      (yyval.text_list).last->next = node;
      (yyval.text_list).last = node;
    }
#line 2314 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 4:
#line 309 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.text_list).first = (yyval.text_list).last = nullptr; }
#line 2320 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 6:
#line 314 "src/ast-parser.y" /* yacc.c:1646  */
    {
      TextListNode node;
      node.text = (yyvsp[0].text);
      node.next = nullptr;
      TextList text_list;
      text_list.first = &node;
      text_list.last = &node;
      char* data;
      size_t size;
      dup_text_list(&text_list, &data, &size);
      (yyval.text).start = data;
      (yyval.text).length = size;
    }
#line 2338 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 7:
#line 332 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.types) = new TypeVector(); }
#line 2344 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 8:
#line 333 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.types) = (yyvsp[-1].types);
      (yyval.types)->push_back((yyvsp[0].type));
    }
#line 2353 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 9:
#line 339 "src/ast-parser.y" /* yacc.c:1646  */
    {}
#line 2359 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 10:
#line 342 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.global) = new Global();
      (yyval.global)->type = (yyvsp[0].type);
      (yyval.global)->mutable_ = false;
    }
#line 2369 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 11:
#line 347 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.global) = new Global();
      (yyval.global)->type = (yyvsp[-1].type);
      (yyval.global)->mutable_ = true;
    }
#line 2379 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 12:
#line 354 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.func_sig) = (yyvsp[-1].func_sig); }
#line 2385 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 13:
#line 357 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.func_sig) = new FuncSignature(); }
#line 2391 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 14:
#line 358 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_sig) = new FuncSignature();
      (yyval.func_sig)->param_types = std::move(*(yyvsp[-1].types));
      delete (yyvsp[-1].types);
    }
#line 2401 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 15:
#line 363 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_sig) = new FuncSignature();
      (yyval.func_sig)->param_types = std::move(*(yyvsp[-5].types));
      delete (yyvsp[-5].types);
      (yyval.func_sig)->result_types = std::move(*(yyvsp[-1].types));
      delete (yyvsp[-1].types);
    }
#line 2413 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 16:
#line 370 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_sig) = new FuncSignature();
      (yyval.func_sig)->result_types = std::move(*(yyvsp[-1].types));
      delete (yyvsp[-1].types);
    }
#line 2423 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 17:
#line 378 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.table) = new Table();
      (yyval.table)->elem_limits = (yyvsp[-1].limits);
    }
#line 2432 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 18:
#line 384 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.memory) = new Memory();
      (yyval.memory)->page_limits = (yyvsp[0].limits);
    }
#line 2441 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 19:
#line 390 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.limits).has_max = false;
      (yyval.limits).initial = (yyvsp[0].u64);
      (yyval.limits).max = 0;
    }
#line 2451 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 20:
#line 395 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.limits).has_max = true;
      (yyval.limits).initial = (yyvsp[-1].u64);
      (yyval.limits).max = (yyvsp[0].u64);
    }
#line 2461 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 21:
#line 402 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.var) = (yyvsp[-1].var); }
#line 2467 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 22:
#line 408 "src/ast-parser.y" /* yacc.c:1646  */
    {
      if (WABT_FAILED(parse_uint64((yyvsp[0].literal).text.start,
                                        (yyvsp[0].literal).text.start + (yyvsp[0].literal).text.length, &(yyval.u64)))) {
        ast_parser_error(&(yylsp[0]), lexer, parser,
                              "invalid int " PRIstringslice "\"",
                              WABT_PRINTF_STRING_SLICE_ARG((yyvsp[0].literal).text));
      }
    }
#line 2480 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 23:
#line 419 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.literal).type = (yyvsp[0].literal).type;
      DUPTEXT((yyval.literal).text, (yyvsp[0].literal).text);
    }
#line 2489 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 24:
#line 423 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.literal).type = (yyvsp[0].literal).type;
      DUPTEXT((yyval.literal).text, (yyvsp[0].literal).text);
    }
#line 2498 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 25:
#line 427 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.literal).type = (yyvsp[0].literal).type;
      DUPTEXT((yyval.literal).text, (yyvsp[0].literal).text);
    }
#line 2507 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 26:
#line 434 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.var).loc = (yylsp[0]);
      (yyval.var).type = VarType::Index;
      (yyval.var).index = (yyvsp[0].u64);
    }
#line 2517 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 27:
#line 439 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.var).loc = (yylsp[0]);
      (yyval.var).type = VarType::Name;
      DUPTEXT((yyval.var).name, (yyvsp[0].text));
    }
#line 2527 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 28:
#line 446 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.vars) = new VarVector(); }
#line 2533 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 29:
#line 447 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.vars) = (yyvsp[-1].vars);
      (yyval.vars)->push_back((yyvsp[0].var));
    }
#line 2542 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 30:
#line 453 "src/ast-parser.y" /* yacc.c:1646  */
    { WABT_ZERO_MEMORY((yyval.text)); }
#line 2548 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 32:
#line 457 "src/ast-parser.y" /* yacc.c:1646  */
    { DUPTEXT((yyval.text), (yyvsp[0].text)); }
#line 2554 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 33:
#line 461 "src/ast-parser.y" /* yacc.c:1646  */
    { WABT_ZERO_MEMORY((yyval.text)); }
#line 2560 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 35:
#line 466 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.u64) = 0; }
#line 2566 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 36:
#line 467 "src/ast-parser.y" /* yacc.c:1646  */
    {
    if (WABT_FAILED(parse_int64((yyvsp[0].text).start, (yyvsp[0].text).start + (yyvsp[0].text).length, &(yyval.u64),
                                ParseIntType::SignedAndUnsigned))) {
      ast_parser_error(&(yylsp[0]), lexer, parser,
                            "invalid offset \"" PRIstringslice "\"",
                            WABT_PRINTF_STRING_SLICE_ARG((yyvsp[0].text)));
      }
    }
#line 2579 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 37:
#line 477 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.u32) = USE_NATURAL_ALIGNMENT; }
#line 2585 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 38:
#line 478 "src/ast-parser.y" /* yacc.c:1646  */
    {
    if (WABT_FAILED(parse_int32((yyvsp[0].text).start, (yyvsp[0].text).start + (yyvsp[0].text).length, &(yyval.u32),
                                ParseIntType::UnsignedOnly))) {
      ast_parser_error(&(yylsp[0]), lexer, parser,
                       "invalid alignment \"" PRIstringslice "\"",
                       WABT_PRINTF_STRING_SLICE_ARG((yyvsp[0].text)));
      }
    }
#line 2598 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 39:
#line 489 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.expr_list) = join_exprs1(&(yylsp[0]), (yyvsp[0].expr)); }
#line 2604 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 40:
#line 490 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.expr_list) = join_exprs1(&(yylsp[0]), (yyvsp[0].expr)); }
#line 2610 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 41:
#line 491 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.expr_list) = (yyvsp[0].expr_list); }
#line 2616 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 42:
#line 494 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateUnreachable();
    }
#line 2624 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 43:
#line 497 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateNop();
    }
#line 2632 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 44:
#line 500 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateDrop();
    }
#line 2640 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 45:
#line 503 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateSelect();
    }
#line 2648 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 46:
#line 506 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateBr((yyvsp[0].var));
    }
#line 2656 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 47:
#line 509 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateBrIf((yyvsp[0].var));
    }
#line 2664 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 48:
#line 512 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateBrTable((yyvsp[-1].vars), (yyvsp[0].var));
    }
#line 2672 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 49:
#line 515 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateReturn();
    }
#line 2680 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 50:
#line 518 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateCall((yyvsp[0].var));
    }
#line 2688 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 51:
#line 521 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateCallIndirect((yyvsp[0].var));
    }
#line 2696 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 52:
#line 524 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateGetLocal((yyvsp[0].var));
    }
#line 2704 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 53:
#line 527 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateSetLocal((yyvsp[0].var));
    }
#line 2712 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 54:
#line 530 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateTeeLocal((yyvsp[0].var));
    }
#line 2720 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 55:
#line 533 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateGetGlobal((yyvsp[0].var));
    }
#line 2728 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 56:
#line 536 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateSetGlobal((yyvsp[0].var));
    }
#line 2736 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 57:
#line 539 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateLoad((yyvsp[-2].opcode), (yyvsp[0].u32), (yyvsp[-1].u64));
    }
#line 2744 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 58:
#line 542 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateStore((yyvsp[-2].opcode), (yyvsp[0].u32), (yyvsp[-1].u64));
    }
#line 2752 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 59:
#line 545 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Const const_;
      WABT_ZERO_MEMORY(const_);
      const_.loc = (yylsp[-1]);
      if (WABT_FAILED(parse_const((yyvsp[-1].type), (yyvsp[0].literal).type, (yyvsp[0].literal).text.start,
                                  (yyvsp[0].literal).text.start + (yyvsp[0].literal).text.length, &const_))) {
        ast_parser_error(&(yylsp[0]), lexer, parser,
                              "invalid literal \"" PRIstringslice "\"",
                              WABT_PRINTF_STRING_SLICE_ARG((yyvsp[0].literal).text));
      }
      delete [] (yyvsp[0].literal).text.start;
      (yyval.expr) = Expr::CreateConst(const_);
    }
#line 2770 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 60:
#line 558 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateUnary((yyvsp[0].opcode));
    }
#line 2778 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 61:
#line 561 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateBinary((yyvsp[0].opcode));
    }
#line 2786 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 62:
#line 564 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateCompare((yyvsp[0].opcode));
    }
#line 2794 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 63:
#line 567 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateConvert((yyvsp[0].opcode));
    }
#line 2802 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 64:
#line 570 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateCurrentMemory();
    }
#line 2810 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 65:
#line 573 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateGrowMemory();
    }
#line 2818 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 66:
#line 578 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateBlock((yyvsp[-2].block));
      (yyval.expr)->block->label = (yyvsp[-3].text);
      CHECK_END_LABEL((yylsp[0]), (yyval.expr)->block->label, (yyvsp[0].text));
    }
#line 2828 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 67:
#line 583 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateLoop((yyvsp[-2].block));
      (yyval.expr)->loop->label = (yyvsp[-3].text);
      CHECK_END_LABEL((yylsp[0]), (yyval.expr)->loop->label, (yyvsp[0].text));
    }
#line 2838 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 68:
#line 588 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateIf((yyvsp[-2].block), nullptr);
      (yyval.expr)->if_.true_->label = (yyvsp[-3].text);
      CHECK_END_LABEL((yylsp[0]), (yyval.expr)->if_.true_->label, (yyvsp[0].text));
    }
#line 2848 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 69:
#line 593 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr) = Expr::CreateIf((yyvsp[-5].block), (yyvsp[-2].expr_list).first);
      (yyval.expr)->if_.true_->label = (yyvsp[-6].text);
      CHECK_END_LABEL((yylsp[-3]), (yyval.expr)->if_.true_->label, (yyvsp[-3].text));
      CHECK_END_LABEL((yylsp[0]), (yyval.expr)->if_.true_->label, (yyvsp[0].text));
    }
#line 2859 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 70:
#line 601 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.block) = new Block();
      (yyval.block)->sig = std::move(*(yyvsp[-1].types));
      delete (yyvsp[-1].types);
      (yyval.block)->first = (yyvsp[0].expr_list).first;
    }
#line 2870 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 71:
#line 610 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.expr_list) = (yyvsp[-1].expr_list); }
#line 2876 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 72:
#line 614 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr_list) = join_exprs2(&(yylsp[-1]), &(yyvsp[0].expr_list), (yyvsp[-1].expr));
    }
#line 2884 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 73:
#line 617 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateBlock((yyvsp[0].block));
      expr->block->label = (yyvsp[-1].text);
      (yyval.expr_list) = join_exprs1(&(yylsp[-2]), expr);
    }
#line 2894 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 74:
#line 622 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateLoop((yyvsp[0].block));
      expr->loop->label = (yyvsp[-1].text);
      (yyval.expr_list) = join_exprs1(&(yylsp[-2]), expr);
    }
#line 2904 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 75:
#line 627 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr_list) = (yyvsp[0].expr_list);
      Expr* if_ = (yyvsp[0].expr_list).last;
      assert(if_->type == ExprType::If);
      if_->if_.true_->label = (yyvsp[-2].text);
      if_->if_.true_->sig = std::move(*(yyvsp[-1].types));
      delete (yyvsp[-1].types);
    }
#line 2917 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 76:
#line 637 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateIf(new Block((yyvsp[-5].expr_list).first), (yyvsp[-1].expr_list).first);
      (yyval.expr_list) = join_exprs1(&(yylsp[-7]), expr);
    }
#line 2926 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 77:
#line 641 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateIf(new Block((yyvsp[-1].expr_list).first), nullptr);
      (yyval.expr_list) = join_exprs1(&(yylsp[-3]), expr);
    }
#line 2935 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 78:
#line 645 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateIf(new Block((yyvsp[-5].expr_list).first), (yyvsp[-1].expr_list).first);
      (yyval.expr_list) = join_exprs2(&(yylsp[-8]), &(yyvsp[-8].expr_list), expr);
    }
#line 2944 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 79:
#line 649 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateIf(new Block((yyvsp[-1].expr_list).first), nullptr);
      (yyval.expr_list) = join_exprs2(&(yylsp[-4]), &(yyvsp[-4].expr_list), expr);
    }
#line 2953 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 80:
#line 653 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateIf(new Block((yyvsp[-1].expr_list).first), (yyvsp[0].expr_list).first);
      (yyval.expr_list) = join_exprs2(&(yylsp[-2]), &(yyvsp[-2].expr_list), expr);
    }
#line 2962 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 81:
#line 657 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateIf(new Block((yyvsp[0].expr_list).first), nullptr);
      (yyval.expr_list) = join_exprs2(&(yylsp[-1]), &(yyvsp[-1].expr_list), expr);
    }
#line 2971 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 82:
#line 664 "src/ast-parser.y" /* yacc.c:1646  */
    { WABT_ZERO_MEMORY((yyval.expr_list)); }
#line 2977 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 83:
#line 665 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr_list).first = (yyvsp[-1].expr_list).first;
      (yyvsp[-1].expr_list).last->next = (yyvsp[0].expr_list).first;
      (yyval.expr_list).last = (yyvsp[0].expr_list).last ? (yyvsp[0].expr_list).last : (yyvsp[-1].expr_list).last;
      (yyval.expr_list).size = (yyvsp[-1].expr_list).size + (yyvsp[0].expr_list).size;
    }
#line 2988 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 84:
#line 673 "src/ast-parser.y" /* yacc.c:1646  */
    { WABT_ZERO_MEMORY((yyval.expr_list)); }
#line 2994 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 85:
#line 674 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr_list).first = (yyvsp[-1].expr_list).first;
      (yyvsp[-1].expr_list).last->next = (yyvsp[0].expr_list).first;
      (yyval.expr_list).last = (yyvsp[0].expr_list).last ? (yyvsp[0].expr_list).last : (yyvsp[-1].expr_list).last;
      (yyval.expr_list).size = (yyvsp[-1].expr_list).size + (yyvsp[0].expr_list).size;
    }
#line 3005 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 88:
#line 688 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_fields) = new FuncField();
      (yyval.func_fields)->type = FuncFieldType::ResultTypes;
      (yyval.func_fields)->types = (yyvsp[-2].types);
      (yyval.func_fields)->next = (yyvsp[0].func_fields);
    }
#line 3016 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 89:
#line 694 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_fields) = new FuncField();
      (yyval.func_fields)->type = FuncFieldType::ParamTypes;
      (yyval.func_fields)->types = (yyvsp[-2].types);
      (yyval.func_fields)->next = (yyvsp[0].func_fields);
    }
#line 3027 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 90:
#line 700 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_fields) = new FuncField();
      (yyval.func_fields)->type = FuncFieldType::BoundParam;
      (yyval.func_fields)->bound_type.loc = (yylsp[-4]);
      (yyval.func_fields)->bound_type.name = (yyvsp[-3].text);
      (yyval.func_fields)->bound_type.type = (yyvsp[-2].type);
      (yyval.func_fields)->next = (yyvsp[0].func_fields);
    }
#line 3040 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 91:
#line 710 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_fields) = new FuncField();
      (yyval.func_fields)->type = FuncFieldType::Exprs;
      (yyval.func_fields)->first_expr = (yyvsp[0].expr_list).first;
      (yyval.func_fields)->next = nullptr;
    }
#line 3051 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 92:
#line 716 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_fields) = new FuncField();
      (yyval.func_fields)->type = FuncFieldType::LocalTypes;
      (yyval.func_fields)->types = (yyvsp[-2].types);
      (yyval.func_fields)->next = (yyvsp[0].func_fields);
    }
#line 3062 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 93:
#line 722 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_fields) = new FuncField();
      (yyval.func_fields)->type = FuncFieldType::BoundLocal;
      (yyval.func_fields)->bound_type.loc = (yylsp[-4]);
      (yyval.func_fields)->bound_type.name = (yyvsp[-3].text);
      (yyval.func_fields)->bound_type.type = (yyvsp[-2].type);
      (yyval.func_fields)->next = (yyvsp[0].func_fields);
    }
#line 3075 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 94:
#line 732 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func) = new Func();
      FuncField* field = (yyvsp[0].func_fields);

      while (field) {
        FuncField* next = field->next;
        switch (field->type) {
          case FuncFieldType::Exprs:
            (yyval.func)->first_expr = field->first_expr;
            field->first_expr = nullptr;
            break;

          case FuncFieldType::ParamTypes:
          case FuncFieldType::LocalTypes: {
            TypeVector& types = field->type == FuncFieldType::ParamTypes
                                    ? (yyval.func)->decl.sig.param_types
                                    : (yyval.func)->local_types;
            types.insert(types.end(), field->types->begin(),
                         field->types->end());
            break;
          }

          case FuncFieldType::BoundParam:
          case FuncFieldType::BoundLocal: {
            TypeVector* types;
            BindingHash* bindings;
            if (field->type == FuncFieldType::BoundParam) {
              types = &(yyval.func)->decl.sig.param_types;
              bindings = &(yyval.func)->param_bindings;
            } else {
              types = &(yyval.func)->local_types;
              bindings = &(yyval.func)->local_bindings;
            }

            types->push_back(field->bound_type.type);
            bindings->emplace(
                string_slice_to_string(field->bound_type.name),
                Binding(field->bound_type.loc, types->size() - 1));
            break;
          }

          case FuncFieldType::ResultTypes:
            (yyval.func)->decl.sig.result_types = std::move(*field->types);
            break;
        }

        delete field;
        field = next;
      }
    }
#line 3130 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 95:
#line 784 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_func) = new ExportedFunc();
      (yyval.exported_func)->func.reset((yyvsp[-1].func));
      (yyval.exported_func)->func->decl.has_func_type = true;
      (yyval.exported_func)->func->decl.type_var = (yyvsp[-2].var);
      (yyval.exported_func)->func->name = (yyvsp[-4].text);
      (yyval.exported_func)->export_ = std::move(*(yyvsp[-3].optional_export));
      delete (yyvsp[-3].optional_export);
    }
#line 3144 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 96:
#line 794 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_func) = new ExportedFunc();
      (yyval.exported_func)->func.reset((yyvsp[-1].func));
      (yyval.exported_func)->func->decl.has_func_type = true;
      (yyval.exported_func)->func->decl.type_var = (yyvsp[-2].var);
      (yyval.exported_func)->func->name = (yyvsp[-3].text);
    }
#line 3156 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 97:
#line 801 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_func) = new ExportedFunc();
      (yyval.exported_func)->func.reset((yyvsp[-1].func));
      (yyval.exported_func)->func->name = (yyvsp[-3].text);
      (yyval.exported_func)->export_ = std::move(*(yyvsp[-2].optional_export));
      delete (yyvsp[-2].optional_export);
    }
#line 3168 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 98:
#line 809 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_func) = new ExportedFunc();
      (yyval.exported_func)->func.reset((yyvsp[-1].func));
      (yyval.exported_func)->func->name = (yyvsp[-2].text);
    }
#line 3178 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 99:
#line 819 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.expr_list) = (yyvsp[-1].expr_list);
    }
#line 3186 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 101:
#line 826 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.elem_segment) = new ElemSegment();
      (yyval.elem_segment)->table_var = (yyvsp[-3].var);
      (yyval.elem_segment)->offset = (yyvsp[-2].expr_list).first;
      (yyval.elem_segment)->vars = std::move(*(yyvsp[-1].vars));
      delete (yyvsp[-1].vars);
    }
#line 3198 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 102:
#line 833 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.elem_segment) = new ElemSegment();
      (yyval.elem_segment)->table_var.loc = (yylsp[-3]);
      (yyval.elem_segment)->table_var.type = VarType::Index;
      (yyval.elem_segment)->table_var.index = 0;
      (yyval.elem_segment)->offset = (yyvsp[-2].expr_list).first;
      (yyval.elem_segment)->vars = std::move(*(yyvsp[-1].vars));
      delete (yyvsp[-1].vars);
    }
#line 3212 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 103:
#line 845 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_table) = new ExportedTable();
      (yyval.exported_table)->table.reset((yyvsp[-1].table));
      (yyval.exported_table)->table->name = (yyvsp[-3].text);
      (yyval.exported_table)->has_elem_segment = false;
      (yyval.exported_table)->export_ = std::move(*(yyvsp[-2].optional_export));
      delete (yyvsp[-2].optional_export);
    }
#line 3225 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 104:
#line 854 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateConst(Const(Const::I32(), 0));
      expr->loc = (yylsp[-8]);

      (yyval.exported_table) = new ExportedTable();
      (yyval.exported_table)->table.reset(new Table());
      (yyval.exported_table)->table->name = (yyvsp[-7].text);
      (yyval.exported_table)->table->elem_limits.initial = (yyvsp[-2].vars)->size();
      (yyval.exported_table)->table->elem_limits.max = (yyvsp[-2].vars)->size();
      (yyval.exported_table)->table->elem_limits.has_max = true;
      (yyval.exported_table)->has_elem_segment = true;
      (yyval.exported_table)->elem_segment.reset(new ElemSegment());
      (yyval.exported_table)->elem_segment->offset = expr;
      (yyval.exported_table)->elem_segment->vars = std::move(*(yyvsp[-2].vars));
      delete (yyvsp[-2].vars);
      (yyval.exported_table)->export_ = std::move(*(yyvsp[-6].optional_export));
      delete (yyvsp[-6].optional_export);
    }
#line 3248 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 105:
#line 875 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.data_segment) = new DataSegment();
      (yyval.data_segment)->memory_var = (yyvsp[-3].var);
      (yyval.data_segment)->offset = (yyvsp[-2].expr_list).first;
      dup_text_list(&(yyvsp[-1].text_list), &(yyval.data_segment)->data, &(yyval.data_segment)->size);
      destroy_text_list(&(yyvsp[-1].text_list));
    }
#line 3260 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 106:
#line 882 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.data_segment) = new DataSegment();
      (yyval.data_segment)->memory_var.loc = (yylsp[-3]);
      (yyval.data_segment)->memory_var.type = VarType::Index;
      (yyval.data_segment)->memory_var.index = 0;
      (yyval.data_segment)->offset = (yyvsp[-2].expr_list).first;
      dup_text_list(&(yyvsp[-1].text_list), &(yyval.data_segment)->data, &(yyval.data_segment)->size);
      destroy_text_list(&(yyvsp[-1].text_list));
    }
#line 3274 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 107:
#line 894 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_memory) = new ExportedMemory();
      (yyval.exported_memory)->memory.reset((yyvsp[-1].memory));
      (yyval.exported_memory)->memory->name = (yyvsp[-3].text);
      (yyval.exported_memory)->has_data_segment = false;
      (yyval.exported_memory)->export_ = std::move(*(yyvsp[-2].optional_export));
      delete (yyvsp[-2].optional_export);
    }
#line 3287 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 108:
#line 902 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateConst(Const(Const::I32(), 0));
      expr->loc = (yylsp[-7]);

      (yyval.exported_memory) = new ExportedMemory();
      (yyval.exported_memory)->has_data_segment = true;
      (yyval.exported_memory)->data_segment.reset(new DataSegment());
      (yyval.exported_memory)->data_segment->offset = expr;
      dup_text_list(&(yyvsp[-2].text_list), &(yyval.exported_memory)->data_segment->data, &(yyval.exported_memory)->data_segment->size);
      destroy_text_list(&(yyvsp[-2].text_list));
      uint32_t byte_size = WABT_ALIGN_UP_TO_PAGE((yyval.exported_memory)->data_segment->size);
      uint32_t page_size = WABT_BYTES_TO_PAGES(byte_size);
      (yyval.exported_memory)->memory.reset(new Memory());
      (yyval.exported_memory)->memory->name = (yyvsp[-6].text);
      (yyval.exported_memory)->memory->page_limits.initial = page_size;
      (yyval.exported_memory)->memory->page_limits.max = page_size;
      (yyval.exported_memory)->memory->page_limits.has_max = true;
      (yyval.exported_memory)->export_ = std::move(*(yyvsp[-5].optional_export));
      delete (yyvsp[-5].optional_export);
    }
#line 3312 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 109:
#line 923 "src/ast-parser.y" /* yacc.c:1646  */
    {
      Expr* expr = Expr::CreateConst(Const(Const::I32(), 0));
      expr->loc = (yylsp[-6]);

      (yyval.exported_memory) = new ExportedMemory();
      (yyval.exported_memory)->has_data_segment = true;
      (yyval.exported_memory)->data_segment.reset(new DataSegment());
      (yyval.exported_memory)->data_segment->offset = expr;
      dup_text_list(&(yyvsp[-2].text_list), &(yyval.exported_memory)->data_segment->data, &(yyval.exported_memory)->data_segment->size);
      destroy_text_list(&(yyvsp[-2].text_list));
      uint32_t byte_size = WABT_ALIGN_UP_TO_PAGE((yyval.exported_memory)->data_segment->size);
      uint32_t page_size = WABT_BYTES_TO_PAGES(byte_size);
      (yyval.exported_memory)->memory.reset(new Memory());
      (yyval.exported_memory)->memory->name = (yyvsp[-5].text);
      (yyval.exported_memory)->memory->page_limits.initial = page_size;
      (yyval.exported_memory)->memory->page_limits.max = page_size;
      (yyval.exported_memory)->memory->page_limits.has_max = true;
      (yyval.exported_memory)->export_.has_export = false;
    }
#line 3336 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 110:
#line 945 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_global) = new ExportedGlobal();
      (yyval.exported_global)->global.reset((yyvsp[-2].global));
      (yyval.exported_global)->global->name = (yyvsp[-4].text);
      (yyval.exported_global)->global->init_expr = (yyvsp[-1].expr_list).first;
      (yyval.exported_global)->export_ = std::move(*(yyvsp[-3].optional_export));
      delete (yyvsp[-3].optional_export);
    }
#line 3349 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 111:
#line 953 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.exported_global) = new ExportedGlobal();
      (yyval.exported_global)->global.reset((yyvsp[-2].global));
      (yyval.exported_global)->global->name = (yyvsp[-3].text);
      (yyval.exported_global)->global->init_expr = (yyvsp[-1].expr_list).first;
      (yyval.exported_global)->export_.has_export = false;
    }
#line 3361 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 112:
#line 966 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = new Import();
      (yyval.import)->kind = ExternalKind::Func;
      (yyval.import)->func = new Func();
      (yyval.import)->func->name = (yyvsp[-2].text);
      (yyval.import)->func->decl.has_func_type = true;
      (yyval.import)->func->decl.type_var = (yyvsp[-1].var);
    }
#line 3374 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 113:
#line 974 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = new Import();
      (yyval.import)->kind = ExternalKind::Func;
      (yyval.import)->func = new Func();
      (yyval.import)->func->name = (yyvsp[-2].text);
      (yyval.import)->func->decl.sig = std::move(*(yyvsp[-1].func_sig));
      delete (yyvsp[-1].func_sig);
    }
#line 3387 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 114:
#line 982 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = new Import();
      (yyval.import)->kind = ExternalKind::Table;
      (yyval.import)->table = (yyvsp[-1].table);
      (yyval.import)->table->name = (yyvsp[-2].text);
    }
#line 3398 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 115:
#line 988 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = new Import();
      (yyval.import)->kind = ExternalKind::Memory;
      (yyval.import)->memory = (yyvsp[-1].memory);
      (yyval.import)->memory->name = (yyvsp[-2].text);
    }
#line 3409 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 116:
#line 994 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = new Import();
      (yyval.import)->kind = ExternalKind::Global;
      (yyval.import)->global = (yyvsp[-1].global);
      (yyval.import)->global->name = (yyvsp[-2].text);
    }
#line 3420 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 117:
#line 1002 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = (yyvsp[-1].import);
      (yyval.import)->module_name = (yyvsp[-3].text);
      (yyval.import)->field_name = (yyvsp[-2].text);
    }
#line 3430 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 118:
#line 1007 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = (yyvsp[-2].import);
      (yyval.import)->kind = ExternalKind::Func;
      (yyval.import)->func = new Func();
      (yyval.import)->func->name = (yyvsp[-3].text);
      (yyval.import)->func->decl.has_func_type = true;
      (yyval.import)->func->decl.type_var = (yyvsp[-1].var);
    }
#line 3443 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 119:
#line 1015 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = (yyvsp[-2].import);
      (yyval.import)->kind = ExternalKind::Func;
      (yyval.import)->func = new Func();
      (yyval.import)->func->name = (yyvsp[-3].text);
      (yyval.import)->func->decl.sig = std::move(*(yyvsp[-1].func_sig));
      delete (yyvsp[-1].func_sig);
    }
#line 3456 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 120:
#line 1023 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = (yyvsp[-2].import);
      (yyval.import)->kind = ExternalKind::Table;
      (yyval.import)->table = (yyvsp[-1].table);
      (yyval.import)->table->name = (yyvsp[-3].text);
    }
#line 3467 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 121:
#line 1029 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = (yyvsp[-2].import);
      (yyval.import)->kind = ExternalKind::Memory;
      (yyval.import)->memory = (yyvsp[-1].memory);
      (yyval.import)->memory->name = (yyvsp[-3].text);
    }
#line 3478 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 122:
#line 1035 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = (yyvsp[-2].import);
      (yyval.import)->kind = ExternalKind::Global;
      (yyval.import)->global = (yyvsp[-1].global);
      (yyval.import)->global->name = (yyvsp[-3].text);
    }
#line 3489 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 123:
#line 1044 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.import) = new Import();
      (yyval.import)->module_name = (yyvsp[-2].text);
      (yyval.import)->field_name = (yyvsp[-1].text);
    }
#line 3499 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 124:
#line 1052 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.export_) = new Export();
      (yyval.export_)->kind = ExternalKind::Func;
      (yyval.export_)->var = (yyvsp[-1].var);
    }
#line 3509 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 125:
#line 1057 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.export_) = new Export();
      (yyval.export_)->kind = ExternalKind::Table;
      (yyval.export_)->var = (yyvsp[-1].var);
    }
#line 3519 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 126:
#line 1062 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.export_) = new Export();
      (yyval.export_)->kind = ExternalKind::Memory;
      (yyval.export_)->var = (yyvsp[-1].var);
    }
#line 3529 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 127:
#line 1067 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.export_) = new Export();
      (yyval.export_)->kind = ExternalKind::Global;
      (yyval.export_)->var = (yyvsp[-1].var);
    }
#line 3539 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 128:
#line 1074 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.export_) = (yyvsp[-1].export_);
      (yyval.export_)->name = (yyvsp[-2].text);
    }
#line 3548 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 129:
#line 1081 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.optional_export) = new OptionalExport();
      (yyval.optional_export)->has_export = false;
    }
#line 3557 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 131:
#line 1088 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.optional_export) = new OptionalExport();
      (yyval.optional_export)->has_export = true;
      (yyval.optional_export)->export_.reset(new Export());
      (yyval.optional_export)->export_->name = (yyvsp[-1].text);
    }
#line 3568 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 132:
#line 1100 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_type) = new FuncType();
      (yyval.func_type)->sig = std::move(*(yyvsp[-1].func_sig));
      delete (yyvsp[-1].func_sig);
    }
#line 3578 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 133:
#line 1105 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.func_type) = new FuncType();
      (yyval.func_type)->name = (yyvsp[-2].text);
      (yyval.func_type)->sig = std::move(*(yyvsp[-1].func_sig));
      delete (yyvsp[-1].func_sig);
    }
#line 3589 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 134:
#line 1114 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.var) = (yyvsp[-1].var); }
#line 3595 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 135:
#line 1118 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = new Module();
    }
#line 3603 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 136:
#line 1121 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, FuncType, func_type, (yylsp[0]), (yyvsp[0].func_type));
      APPEND_ITEM_TO_VECTOR((yyval.module), func_types, field->func_type);
      INSERT_BINDING((yyval.module), func_type, func_types, (yylsp[0]), (yyvsp[0].func_type)->name);
    }
#line 3615 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 137:
#line 1128 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Global, global, (yylsp[0]), (yyvsp[0].exported_global)->global.release());
      APPEND_ITEM_TO_VECTOR((yyval.module), globals, field->global);
      INSERT_BINDING((yyval.module), global, globals, (yylsp[0]), field->global->name);
      APPEND_INLINE_EXPORT((yyval.module), Global, (yylsp[0]), (yyvsp[0].exported_global), (yyval.module)->globals.size() - 1);
      delete (yyvsp[0].exported_global);
    }
#line 3629 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 138:
#line 1137 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Table, table, (yylsp[0]), (yyvsp[0].exported_table)->table.release());
      APPEND_ITEM_TO_VECTOR((yyval.module), tables, field->table);
      INSERT_BINDING((yyval.module), table, tables, (yylsp[0]), field->table->name);
      APPEND_INLINE_EXPORT((yyval.module), Table, (yylsp[0]), (yyvsp[0].exported_table), (yyval.module)->tables.size() - 1);

      if ((yyvsp[0].exported_table)->has_elem_segment) {
        ModuleField* elem_segment_field;
        APPEND_FIELD_TO_LIST((yyval.module), elem_segment_field, ElemSegment, elem_segment,
                             (yylsp[0]), (yyvsp[0].exported_table)->elem_segment.release());
        APPEND_ITEM_TO_VECTOR((yyval.module), elem_segments,
                              elem_segment_field->elem_segment);
      }
      delete (yyvsp[0].exported_table);
    }
#line 3651 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 139:
#line 1154 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Memory, memory, (yylsp[0]), (yyvsp[0].exported_memory)->memory.release());
      APPEND_ITEM_TO_VECTOR((yyval.module), memories, field->memory);
      INSERT_BINDING((yyval.module), memory, memories, (yylsp[0]), field->memory->name);
      APPEND_INLINE_EXPORT((yyval.module), Memory, (yylsp[0]), (yyvsp[0].exported_memory), (yyval.module)->memories.size() - 1);

      if ((yyvsp[0].exported_memory)->has_data_segment) {
        ModuleField* data_segment_field;
        APPEND_FIELD_TO_LIST((yyval.module), data_segment_field, DataSegment, data_segment,
                             (yylsp[0]), (yyvsp[0].exported_memory)->data_segment.release());
        APPEND_ITEM_TO_VECTOR((yyval.module), data_segments,
                              data_segment_field->data_segment);
      }
      delete (yyvsp[0].exported_memory);
    }
#line 3673 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 140:
#line 1171 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Func, func, (yylsp[0]), (yyvsp[0].exported_func)->func.release());
      append_implicit_func_declaration(&(yylsp[0]), (yyval.module), &field->func->decl);
      APPEND_ITEM_TO_VECTOR((yyval.module), funcs, field->func);
      INSERT_BINDING((yyval.module), func, funcs, (yylsp[0]), field->func->name);
      APPEND_INLINE_EXPORT((yyval.module), Func, (yylsp[0]), (yyvsp[0].exported_func), (yyval.module)->funcs.size() - 1);
      delete (yyvsp[0].exported_func);
    }
#line 3688 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 141:
#line 1181 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, ElemSegment, elem_segment, (yylsp[0]), (yyvsp[0].elem_segment));
      APPEND_ITEM_TO_VECTOR((yyval.module), elem_segments, field->elem_segment);
    }
#line 3699 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 142:
#line 1187 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, DataSegment, data_segment, (yylsp[0]), (yyvsp[0].data_segment));
      APPEND_ITEM_TO_VECTOR((yyval.module), data_segments, field->data_segment);
    }
#line 3710 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 143:
#line 1193 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Start, start, (yylsp[0]), (yyvsp[0].var));
      (yyval.module)->start = &field->start;
    }
#line 3721 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 144:
#line 1199 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Import, import, (yylsp[0]), (yyvsp[0].import));
      CHECK_IMPORT_ORDERING((yyval.module), func, funcs, (yylsp[0]));
      CHECK_IMPORT_ORDERING((yyval.module), table, tables, (yylsp[0]));
      CHECK_IMPORT_ORDERING((yyval.module), memory, memories, (yylsp[0]));
      CHECK_IMPORT_ORDERING((yyval.module), global, globals, (yylsp[0]));
      switch ((yyvsp[0].import)->kind) {
        case ExternalKind::Func:
          append_implicit_func_declaration(&(yylsp[0]), (yyval.module), &field->import->func->decl);
          APPEND_ITEM_TO_VECTOR((yyval.module), funcs, field->import->func);
          INSERT_BINDING((yyval.module), func, funcs, (yylsp[0]), field->import->func->name);
          (yyval.module)->num_func_imports++;
          break;
        case ExternalKind::Table:
          APPEND_ITEM_TO_VECTOR((yyval.module), tables, field->import->table);
          INSERT_BINDING((yyval.module), table, tables, (yylsp[0]), field->import->table->name);
          (yyval.module)->num_table_imports++;
          break;
        case ExternalKind::Memory:
          APPEND_ITEM_TO_VECTOR((yyval.module), memories, field->import->memory);
          INSERT_BINDING((yyval.module), memory, memories, (yylsp[0]), field->import->memory->name);
          (yyval.module)->num_memory_imports++;
          break;
        case ExternalKind::Global:
          APPEND_ITEM_TO_VECTOR((yyval.module), globals, field->import->global);
          INSERT_BINDING((yyval.module), global, globals, (yylsp[0]), field->import->global->name);
          (yyval.module)->num_global_imports++;
          break;
      }
      APPEND_ITEM_TO_VECTOR((yyval.module), imports, field->import);
    }
#line 3759 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 145:
#line 1232 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.module) = (yyvsp[-1].module);
      ModuleField* field;
      APPEND_FIELD_TO_LIST((yyval.module), field, Export, export_, (yylsp[0]), (yyvsp[0].export_));
      APPEND_ITEM_TO_VECTOR((yyval.module), exports, field->export_);
      INSERT_BINDING((yyval.module), export, exports, (yylsp[0]), field->export_->name);
    }
#line 3771 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 146:
#line 1242 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.raw_module) = new RawModule();
      (yyval.raw_module)->type = RawModuleType::Text;
      (yyval.raw_module)->text = (yyvsp[-1].module);
      (yyval.raw_module)->text->name = (yyvsp[-2].text);
      (yyval.raw_module)->text->loc = (yylsp[-3]);

      /* resolve func type variables where the signature was not specified
       * explicitly */
      for (Func* func: (yyvsp[-1].module)->funcs) {
        if (decl_has_func_type(&func->decl) &&
            is_empty_signature(&func->decl.sig)) {
          FuncType* func_type =
              get_func_type_by_var((yyvsp[-1].module), &func->decl.type_var);
          if (func_type) {
            func->decl.sig = func_type->sig;
          }
        }
      }
    }
#line 3796 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 147:
#line 1262 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.raw_module) = new RawModule();
      (yyval.raw_module)->type = RawModuleType::Binary;
      (yyval.raw_module)->binary.name = (yyvsp[-2].text);
      (yyval.raw_module)->binary.loc = (yylsp[-3]);
      dup_text_list(&(yyvsp[-1].text_list), &(yyval.raw_module)->binary.data, &(yyval.raw_module)->binary.size);
      destroy_text_list(&(yyvsp[-1].text_list));
    }
#line 3809 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 148:
#line 1273 "src/ast-parser.y" /* yacc.c:1646  */
    {
      if ((yyvsp[0].raw_module)->type == RawModuleType::Text) {
        (yyval.module) = (yyvsp[0].raw_module)->text;
        (yyvsp[0].raw_module)->text = nullptr;
      } else {
        assert((yyvsp[0].raw_module)->type == RawModuleType::Binary);
        (yyval.module) = new Module();
        ReadBinaryOptions options = WABT_READ_BINARY_OPTIONS_DEFAULT;
        BinaryErrorCallbackData user_data;
        user_data.loc = &(yyvsp[0].raw_module)->binary.loc;
        user_data.lexer = lexer;
        user_data.parser = parser;
        BinaryErrorHandler error_handler;
        error_handler.on_error = on_read_binary_error;
        error_handler.user_data = &user_data;
        read_binary_ast((yyvsp[0].raw_module)->binary.data, (yyvsp[0].raw_module)->binary.size, &options,
                        &error_handler, (yyval.module));
        (yyval.module)->name = (yyvsp[0].raw_module)->binary.name;
        (yyval.module)->loc = (yyvsp[0].raw_module)->binary.loc;
        WABT_ZERO_MEMORY((yyvsp[0].raw_module)->binary.name);
      }
      delete (yyvsp[0].raw_module);
    }
#line 3837 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 149:
#line 1301 "src/ast-parser.y" /* yacc.c:1646  */
    {
      WABT_ZERO_MEMORY((yyval.var));
      (yyval.var).type = VarType::Index;
      (yyval.var).index = INVALID_VAR_INDEX;
    }
#line 3847 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 150:
#line 1306 "src/ast-parser.y" /* yacc.c:1646  */
    {
      WABT_ZERO_MEMORY((yyval.var));
      (yyval.var).type = VarType::Name;
      DUPTEXT((yyval.var).name, (yyvsp[0].text));
    }
#line 3857 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 151:
#line 1314 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.action) = new Action();
      (yyval.action)->loc = (yylsp[-4]);
      (yyval.action)->module_var = (yyvsp[-3].var);
      (yyval.action)->type = ActionType::Invoke;
      (yyval.action)->name = (yyvsp[-2].text);
      (yyval.action)->invoke = new ActionInvoke();
      (yyval.action)->invoke->args = std::move(*(yyvsp[-1].consts));
      delete (yyvsp[-1].consts);
    }
#line 3872 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 152:
#line 1324 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.action) = new Action();
      (yyval.action)->loc = (yylsp[-3]);
      (yyval.action)->module_var = (yyvsp[-2].var);
      (yyval.action)->type = ActionType::Get;
      (yyval.action)->name = (yyvsp[-1].text);
    }
#line 3884 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 153:
#line 1334 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertMalformed;
      (yyval.command)->assert_malformed.module = (yyvsp[-2].raw_module);
      (yyval.command)->assert_malformed.text = (yyvsp[-1].text);
    }
#line 3895 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 154:
#line 1340 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertInvalid;
      (yyval.command)->assert_invalid.module = (yyvsp[-2].raw_module);
      (yyval.command)->assert_invalid.text = (yyvsp[-1].text);
    }
#line 3906 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 155:
#line 1346 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertUnlinkable;
      (yyval.command)->assert_unlinkable.module = (yyvsp[-2].raw_module);
      (yyval.command)->assert_unlinkable.text = (yyvsp[-1].text);
    }
#line 3917 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 156:
#line 1352 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertUninstantiable;
      (yyval.command)->assert_uninstantiable.module = (yyvsp[-2].raw_module);
      (yyval.command)->assert_uninstantiable.text = (yyvsp[-1].text);
    }
#line 3928 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 157:
#line 1358 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertReturn;
      (yyval.command)->assert_return.action = (yyvsp[-2].action);
      (yyval.command)->assert_return.expected = (yyvsp[-1].consts);
    }
#line 3939 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 158:
#line 1364 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertReturnCanonicalNan;
      (yyval.command)->assert_return_canonical_nan.action = (yyvsp[-1].action);
    }
#line 3949 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 159:
#line 1369 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertReturnArithmeticNan;
      (yyval.command)->assert_return_arithmetic_nan.action = (yyvsp[-1].action);
    }
#line 3959 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 160:
#line 1374 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertTrap;
      (yyval.command)->assert_trap.action = (yyvsp[-2].action);
      (yyval.command)->assert_trap.text = (yyvsp[-1].text);
    }
#line 3970 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 161:
#line 1380 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::AssertExhaustion;
      (yyval.command)->assert_trap.action = (yyvsp[-2].action);
      (yyval.command)->assert_trap.text = (yyvsp[-1].text);
    }
#line 3981 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 162:
#line 1389 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::Action;
      (yyval.command)->action = (yyvsp[0].action);
    }
#line 3991 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 164:
#line 1395 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::Module;
      (yyval.command)->module = (yyvsp[0].module);
    }
#line 4001 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 165:
#line 1400 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.command) = new Command();
      (yyval.command)->type = CommandType::Register;
      (yyval.command)->register_.module_name = (yyvsp[-2].text);
      (yyval.command)->register_.var = (yyvsp[-1].var);
      (yyval.command)->register_.var.loc = (yylsp[-1]);
    }
#line 4013 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 166:
#line 1409 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.commands) = new CommandPtrVector(); }
#line 4019 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 167:
#line 1410 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.commands) = (yyvsp[-1].commands);
      (yyval.commands)->emplace_back((yyvsp[0].command));
    }
#line 4028 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 168:
#line 1417 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.const_).loc = (yylsp[-2]);
      if (WABT_FAILED(parse_const((yyvsp[-2].type), (yyvsp[-1].literal).type, (yyvsp[-1].literal).text.start,
                                  (yyvsp[-1].literal).text.start + (yyvsp[-1].literal).text.length, &(yyval.const_)))) {
        ast_parser_error(&(yylsp[-1]), lexer, parser,
                              "invalid literal \"" PRIstringslice "\"",
                              WABT_PRINTF_STRING_SLICE_ARG((yyvsp[-1].literal).text));
      }
      delete [] (yyvsp[-1].literal).text.start;
    }
#line 4043 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 169:
#line 1429 "src/ast-parser.y" /* yacc.c:1646  */
    { (yyval.consts) = new ConstVector(); }
#line 4049 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 170:
#line 1430 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.consts) = (yyvsp[-1].consts);
      (yyval.consts)->push_back((yyvsp[0].const_));
    }
#line 4058 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;

  case 171:
#line 1437 "src/ast-parser.y" /* yacc.c:1646  */
    {
      (yyval.script) = new Script();
      (yyval.script)->commands = std::move(*(yyvsp[0].commands));
      delete (yyvsp[0].commands);

      int last_module_index = -1;
      for (size_t i = 0; i < (yyval.script)->commands.size(); ++i) {
        Command& command = *(yyval.script)->commands[i].get();
        Var* module_var = nullptr;
        switch (command.type) {
          case CommandType::Module: {
            last_module_index = i;

            /* Wire up module name bindings. */
            Module* module = command.module;
            if (module->name.length == 0)
              continue;

            (yyval.script)->module_bindings.emplace(string_slice_to_string(module->name),
                                        Binding(module->loc, i));
            break;
          }

          case CommandType::AssertReturn:
            module_var = &command.assert_return.action->module_var;
            goto has_module_var;
          case CommandType::AssertReturnCanonicalNan:
            module_var =
                &command.assert_return_canonical_nan.action->module_var;
            goto has_module_var;
          case CommandType::AssertReturnArithmeticNan:
            module_var =
                &command.assert_return_arithmetic_nan.action->module_var;
            goto has_module_var;
          case CommandType::AssertTrap:
          case CommandType::AssertExhaustion:
            module_var = &command.assert_trap.action->module_var;
            goto has_module_var;
          case CommandType::Action:
            module_var = &command.action->module_var;
            goto has_module_var;
          case CommandType::Register:
            module_var = &command.register_.var;
            goto has_module_var;

          has_module_var: {
            /* Resolve actions with an invalid index to use the preceding
             * module. */
            if (module_var->type == VarType::Index &&
                module_var->index == INVALID_VAR_INDEX) {
              module_var->index = last_module_index;
            }
            break;
          }

          default:
            break;
        }
      }
      parser->script = (yyval.script);
    }
#line 4124 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
    break;


#line 4128 "src/prebuilt/ast-parser-gen.cc" /* yacc.c:1646  */
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
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

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
      yyerror (&yylloc, lexer, parser, YY_("syntax error"));
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
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
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
        yyerror (&yylloc, lexer, parser, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

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
                      yytoken, &yylval, &yylloc, lexer, parser);
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
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

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, lexer, parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

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
  yyerror (&yylloc, lexer, parser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, lexer, parser);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, lexer, parser);
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
#line 1506 "src/ast-parser.y" /* yacc.c:1906  */


void append_expr_list(ExprList* expr_list, ExprList* expr) {
  if (!expr->first)
    return;
  if (expr_list->last)
    expr_list->last->next = expr->first;
  else
    expr_list->first = expr->first;
  expr_list->last = expr->last;
  expr_list->size += expr->size;
}

void append_expr(ExprList* expr_list, Expr* expr) {
  if (expr_list->last)
    expr_list->last->next = expr;
  else
    expr_list->first = expr;
  expr_list->last = expr;
  expr_list->size++;
}

ExprList join_exprs1(Location* loc, Expr* expr1) {
  ExprList result;
  WABT_ZERO_MEMORY(result);
  append_expr(&result, expr1);
  expr1->loc = *loc;
  return result;
}

ExprList join_exprs2(Location* loc, ExprList* expr1, Expr* expr2) {
  ExprList result;
  WABT_ZERO_MEMORY(result);
  append_expr_list(&result, expr1);
  append_expr(&result, expr2);
  expr2->loc = *loc;
  return result;
}

Result parse_const(Type type,
                   LiteralType literal_type,
                   const char* s,
                   const char* end,
                   Const* out) {
  out->type = type;
  switch (type) {
    case Type::I32:
      return parse_int32(s, end, &out->u32, ParseIntType::SignedAndUnsigned);
    case Type::I64:
      return parse_int64(s, end, &out->u64, ParseIntType::SignedAndUnsigned);
    case Type::F32:
      return parse_float(literal_type, s, end, &out->f32_bits);
    case Type::F64:
      return parse_double(literal_type, s, end, &out->f64_bits);
    default:
      assert(0);
      break;
  }
  return Result::Error;
}

size_t copy_string_contents(StringSlice* text, char* dest) {
  const char* src = text->start + 1;
  const char* end = text->start + text->length - 1;

  char* dest_start = dest;

  while (src < end) {
    if (*src == '\\') {
      src++;
      switch (*src) {
        case 'n':
          *dest++ = '\n';
          break;
        case 't':
          *dest++ = '\t';
          break;
        case '\\':
          *dest++ = '\\';
          break;
        case '\'':
          *dest++ = '\'';
          break;
        case '\"':
          *dest++ = '\"';
          break;
        default: {
          /* The string should be validated already, so we know this is a hex
           * sequence */
          uint32_t hi;
          uint32_t lo;
          if (WABT_SUCCEEDED(parse_hexdigit(src[0], &hi)) &&
              WABT_SUCCEEDED(parse_hexdigit(src[1], &lo))) {
            *dest++ = (hi << 4) | lo;
          } else {
            assert(0);
          }
          src++;
          break;
        }
      }
      src++;
    } else {
      *dest++ = *src++;
    }
  }
  /* return the data length */
  return dest - dest_start;
}

void dup_text_list(TextList* text_list, char** out_data, size_t* out_size) {
  /* walk the linked list to see how much total space is needed */
  size_t total_size = 0;
  for (TextListNode* node = text_list->first; node; node = node->next) {
    /* Always allocate enough space for the entire string including the escape
     * characters. It will only get shorter, and this way we only have to
     * iterate through the string once. */
    const char* src = node->text.start + 1;
    const char* end = node->text.start + node->text.length - 1;
    size_t size = (end > src) ? (end - src) : 0;
    total_size += size;
  }
  char* result = new char [total_size];
  char* dest = result;
  for (TextListNode* node = text_list->first; node; node = node->next) {
    size_t actual_size = copy_string_contents(&node->text, dest);
    dest += actual_size;
  }
  *out_data = result;
  *out_size = dest - result;
}

bool is_empty_signature(const FuncSignature* sig) {
  return sig->result_types.empty() && sig->param_types.empty();
}

void append_implicit_func_declaration(Location* loc,
                                      Module* module,
                                      FuncDeclaration* decl) {
  if (decl_has_func_type(decl))
    return;

  int sig_index = get_func_type_index_by_decl(module, decl);
  if (sig_index == -1) {
    append_implicit_func_type(loc, module, &decl->sig);
  } else {
    decl->sig = module->func_types[sig_index]->sig;
  }
}

Result parse_ast(AstLexer* lexer, Script** out_script,
                 SourceErrorHandler* error_handler) {
  AstParser parser;
  WABT_ZERO_MEMORY(parser);
  parser.error_handler = error_handler;
  int result = wabt_ast_parser_parse(lexer, &parser);
  delete [] parser.yyssa;
  delete [] parser.yyvsa;
  delete [] parser.yylsa;
  *out_script = parser.script;
  return result == 0 && parser.errors == 0 ? Result::Ok : Result::Error;
}

bool on_read_binary_error(uint32_t offset, const char* error, void* user_data) {
  BinaryErrorCallbackData* data = (BinaryErrorCallbackData*)user_data;
  if (offset == WABT_UNKNOWN_OFFSET) {
    ast_parser_error(data->loc, data->lexer, data->parser,
                     "error in binary module: %s", error);
  } else {
    ast_parser_error(data->loc, data->lexer, data->parser,
                     "error in binary module: @0x%08x: %s", offset, error);
  }
  return true;
}

}  // namespace wabt
