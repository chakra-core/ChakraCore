/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

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
