//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
/*
 * These are javascript library functions that might be inlined
 * by the JIT.
 *
 * Notes:
 * - the argc is the number of args to pass to InlineXXX call, e.g. 2 for Math.pow and 2 for String.CharAt.
 * - TODO: consider having dst/src1/src2 in separate columns rather than bitmask, this seems to be better for design but we won't be able to see 'all float' by single check.
 * - TODO: enable string inlines when string type spec is available
 *
 *               target         name                argc  flags                                               EntryInfo
 */
LIBRARY_FUNCTION(Math,          Abs,                1,    BIF_TypeSpecSrcAndDstToFloatOrInt                 , Math::EntryInfo::Abs)
LIBRARY_FUNCTION(Math,          Acos,               1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Acos)
LIBRARY_FUNCTION(Math,          Asin,               1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Asin)
LIBRARY_FUNCTION(Math,          Atan,               1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Atan)
LIBRARY_FUNCTION(Math,          Atan2,              2,    BIF_TypeSpecAllToFloat                            , Math::EntryInfo::Atan2)
LIBRARY_FUNCTION(Math,          Ceil,               1,    BIF_TypeSpecDstToInt | BIF_TypeSpecSrc1ToFloat    , Math::EntryInfo::Ceil)
LIBRARY_FUNCTION(JavascriptString,        CodePointAt,        2,    BIF_TypeSpecSrc2ToInt | BIF_UseSrc0               , JavascriptString::EntryInfo::CodePointAt)
LIBRARY_FUNCTION(JavascriptString,        CharAt,             2,    BIF_UseSrc0                                       , JavascriptString::EntryInfo::CharAt  )
LIBRARY_FUNCTION(JavascriptString,        CharCodeAt,         2,    BIF_UseSrc0                                       , JavascriptString::EntryInfo::CharCodeAt )
LIBRARY_FUNCTION(JavascriptString,        Concat,             15,   BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptString::EntryInfo::Concat )
LIBRARY_FUNCTION(JavascriptString,        FromCharCode,       1,    BIF_None                                          , JavascriptString::EntryInfo::FromCharCode)
LIBRARY_FUNCTION(JavascriptString,        FromCodePoint,      1,    BIF_None                                          , JavascriptString::EntryInfo::FromCodePoint)
LIBRARY_FUNCTION(JavascriptString,        IndexOf,            3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptString::EntryInfo::IndexOf)
LIBRARY_FUNCTION(JavascriptString,        LastIndexOf,        3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptString::EntryInfo::LastIndexOf)
LIBRARY_FUNCTION(JavascriptString,        Link,               2,    BIF_UseSrc0                                       , JavascriptString::EntryInfo::Link)
LIBRARY_FUNCTION(JavascriptString,        LocaleCompare,      2,    BIF_UseSrc0                                       , JavascriptString::EntryInfo::LocaleCompare)
LIBRARY_FUNCTION(JavascriptString,        Match,              2,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::Match)
LIBRARY_FUNCTION(JavascriptString,        Replace,            3,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::Replace)
LIBRARY_FUNCTION(JavascriptString,        Search,             2,    BIF_UseSrc0                                       , JavascriptString::EntryInfo::Search)
LIBRARY_FUNCTION(JavascriptString,        Slice,              3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptString::EntryInfo::Slice )
LIBRARY_FUNCTION(JavascriptString,        Split,              3,    BIF_UseSrc0 | BIF_VariableArgsNumber | BIF_IgnoreDst , JavascriptString::EntryInfo::Split)
LIBRARY_FUNCTION(JavascriptString,        Substr,             3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptString::EntryInfo::Substr)
LIBRARY_FUNCTION(JavascriptString,        Substring,          3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptString::EntryInfo::Substring)
LIBRARY_FUNCTION(JavascriptString,        ToLocaleLowerCase,  1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::ToLocaleLowerCase)
LIBRARY_FUNCTION(JavascriptString,        ToLocaleUpperCase,  1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::ToLocaleUpperCase)
LIBRARY_FUNCTION(JavascriptString,        ToLowerCase,        1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::ToLowerCase)
LIBRARY_FUNCTION(JavascriptString,        ToUpperCase,        1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::ToUpperCase)
LIBRARY_FUNCTION(JavascriptString,        Trim,               1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::Trim)
LIBRARY_FUNCTION(JavascriptString,        TrimLeft,           1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::TrimLeft)
LIBRARY_FUNCTION(JavascriptString,        TrimRight,          1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptString::EntryInfo::TrimRight)
LIBRARY_FUNCTION(Math,          Cos,                1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Cos)
LIBRARY_FUNCTION(Math,          Exp,                1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Exp)
LIBRARY_FUNCTION(Math,          Floor,              1,    BIF_TypeSpecDstToInt | BIF_TypeSpecSrc1ToFloat    , Math::EntryInfo::Floor)
LIBRARY_FUNCTION(Math,          Log,                1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Log)
LIBRARY_FUNCTION(Math,          Max,                2,    BIF_TypeSpecSrcAndDstToFloatOrInt                 , Math::EntryInfo::Max)
LIBRARY_FUNCTION(Math,          Min,                2,    BIF_TypeSpecSrcAndDstToFloatOrInt                 , Math::EntryInfo::Min)
LIBRARY_FUNCTION(Math,          Pow,                2,    BIF_TypeSpecSrcAndDstToFloatOrInt                 , Math::EntryInfo::Pow)
LIBRARY_FUNCTION(Math,          Imul,               2,    BIF_TypeSpecAllToInt                              , Math::EntryInfo::Imul)
LIBRARY_FUNCTION(Math,          Clz32,              1,    BIF_TypeSpecAllToInt                              , Math::EntryInfo::Clz32)
LIBRARY_FUNCTION(JavascriptArray,         Push,               2,    BIF_UseSrc0 | BIF_IgnoreDst | BIF_TypeSpecSrc1ToFloatOrInt, JavascriptArray::EntryInfo::Push)
LIBRARY_FUNCTION(JavascriptArray,         Pop,                1,    BIF_UseSrc0 | BIF_TypeSpecDstToFloatOrInt         , JavascriptArray::EntryInfo::Pop)
LIBRARY_FUNCTION(Math,          Random,             0,    BIF_TypeSpecDstToFloat                            , Math::EntryInfo::Random)
LIBRARY_FUNCTION(Math,          Round,              1,    BIF_TypeSpecDstToInt | BIF_TypeSpecSrc1ToFloat    , Math::EntryInfo::Round)
LIBRARY_FUNCTION(Math,          Sin,                1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Sin)
LIBRARY_FUNCTION(Math,          Sqrt,               1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Sqrt)
LIBRARY_FUNCTION(Math,          Tan,                1,    BIF_TypeSpecUnaryToFloat                          , Math::EntryInfo::Tan)
LIBRARY_FUNCTION(JavascriptArray,         Concat,             15,   BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptArray::EntryInfo::Concat)
LIBRARY_FUNCTION(JavascriptArray,         IndexOf,            2,    BIF_UseSrc0                                       , JavascriptArray::EntryInfo::IndexOf)
LIBRARY_FUNCTION(JavascriptArray,         Includes,           2,    BIF_UseSrc0                                       , JavascriptArray::EntryInfo::Includes)
LIBRARY_FUNCTION(JavascriptArray,         IsArray,            1,    BIF_None                                          , JavascriptArray::EntryInfo::IsArray)
LIBRARY_FUNCTION(JavascriptArray,         Join,               2,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptArray::EntryInfo::Join)
LIBRARY_FUNCTION(JavascriptArray,         LastIndexOf,        3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptArray::EntryInfo::LastIndexOf)
LIBRARY_FUNCTION(JavascriptArray,         Reverse,            1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptArray::EntryInfo::Reverse)
LIBRARY_FUNCTION(JavascriptArray,         Shift,              1,    BIF_UseSrc0 | BIF_IgnoreDst                       , JavascriptArray::EntryInfo::Shift)
LIBRARY_FUNCTION(JavascriptArray,         Slice,              3,    BIF_UseSrc0 | BIF_VariableArgsNumber              , JavascriptArray::EntryInfo::Slice)
LIBRARY_FUNCTION(JavascriptArray,         Splice,             15,   BIF_UseSrc0 | BIF_VariableArgsNumber | BIF_IgnoreDst  , JavascriptArray::EntryInfo::Splice)
LIBRARY_FUNCTION(JavascriptArray,         Unshift,            15,   BIF_UseSrc0 | BIF_VariableArgsNumber | BIF_IgnoreDst  , JavascriptArray::EntryInfo::Unshift)
LIBRARY_FUNCTION(JavascriptFunction,      Apply,              3,    BIF_UseSrc0 | BIF_IgnoreDst | BIF_VariableArgsNumber  , JavascriptFunction::EntryInfo::Apply)
LIBRARY_FUNCTION(JavascriptFunction,      Call,               15,   BIF_UseSrc0 | BIF_IgnoreDst | BIF_VariableArgsNumber  , JavascriptFunction::EntryInfo::Call)
LIBRARY_FUNCTION(GlobalObject,  ParseInt,           1,    BIF_IgnoreDst                                         , GlobalObject::EntryInfo::ParseInt)
LIBRARY_FUNCTION(JavascriptRegExp,        Exec,               2,    BIF_UseSrc0 | BIF_IgnoreDst                           , JavascriptRegExp::EntryInfo::Exec)
LIBRARY_FUNCTION(JavascriptRegExp,        SymbolSearch,       2,    BIF_UseSrc0                                           , JavascriptRegExp::EntryInfo::SymbolSearch)
LIBRARY_FUNCTION(Math,          Fround,             1,    BIF_TypeSpecUnaryToFloat                              , Math::EntryInfo::Fround)
LIBRARY_FUNCTION(JavascriptString,        PadStart,           2,    BIF_UseSrc0 | BIF_VariableArgsNumber                  , JavascriptString::EntryInfo::PadStart)
LIBRARY_FUNCTION(JavascriptString,        PadEnd,             2,    BIF_UseSrc0 | BIF_VariableArgsNumber                  , JavascriptString::EntryInfo::PadEnd)
LIBRARY_FUNCTION(JavascriptObject,        HasOwnProperty,     2,    BIF_UseSrc0                                           , JavascriptObject::EntryInfo::HasOwnProperty)

// Note: 1st column is currently used only for debug tracing.

// SIMD_JS
#ifdef ENABLE_SIMDJS
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Float32x4,         4, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Float32x4)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Check,             1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Check)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Splat,             1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Splat)

LIBRARY_FUNCTION(SIMDFloat32x4Lib,    ExtractLane,       2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::ExtractLane)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    ReplaceLane,       3, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::ReplaceLane)

LIBRARY_FUNCTION(SIMDFloat32x4Lib,    FromInt32x4,       1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::FromInt32x4)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    FromInt32x4Bits,   1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::FromInt32x4Bits)

LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Add,               2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Add)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Sub,               2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Sub)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Mul,               2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Mul)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Div,               2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Div)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Min,               2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Min)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Max,               2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Max)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Abs,               1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Abs)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Neg,               1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Neg)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Sqrt,              1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Sqrt)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Reciprocal,        1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Reciprocal)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    ReciprocalSqrt,    1, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::ReciprocalSqrt)

LIBRARY_FUNCTION(SIMDFloat32x4Lib,    LessThan,          2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::LessThan)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    LessThanOrEqual,   2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::LessThanOrEqual)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Equal,             2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Equal)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    NotEqual,          2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::NotEqual)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    GreaterThan,       2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::GreaterThan)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    GreaterThanOrEqual,2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::GreaterThanOrEqual)

LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Shuffle,           6, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Shuffle)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Swizzle,           5, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Swizzle)

LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Load,              2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Load)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Load1,             2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Load1)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Load2,             2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Load2)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Load3,             2, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Load3)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Store,             3, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Store)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Store1,            3, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Store1)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Store2,            3, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Store2)
LIBRARY_FUNCTION(SIMDFloat32x4Lib,    Store3,            3, BIF_IgnoreDst                                                 , SIMDFloat32x4Lib::EntryInfo::Store3)

LIBRARY_FUNCTION(SIMDInt32x4Lib,      Int32x4,           4, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Int32x4)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Check,             1, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Check)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Splat,             1, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Splat)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      ExtractLane,       2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::ExtractLane)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      ReplaceLane,       3, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::ReplaceLane)

LIBRARY_FUNCTION(SIMDInt32x4Lib,      FromFloat32x4,       1, BIF_IgnoreDst                                               , SIMDInt32x4Lib::EntryInfo::FromFloat32x4)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      FromFloat32x4Bits,   1, BIF_IgnoreDst                                               , SIMDInt32x4Lib::EntryInfo::FromFloat32x4Bits)

LIBRARY_FUNCTION(SIMDInt32x4Lib,      Add,               2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Add)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Sub,               2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Sub)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Mul,               2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Mul)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      And,               2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::And)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Or,                2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Or)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Xor,               2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Xor)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Neg,               1, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Neg)
LIBRARY_FUNCTION(SIMDInt32x4Lib,      Not,               1, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Not)

LIBRARY_FUNCTION(SIMDInt32x4Lib,    Shuffle,             6, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Shuffle)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Swizzle,             5, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Swizzle)

LIBRARY_FUNCTION(SIMDInt32x4Lib,    Load,              2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Load)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Load1,             2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Load1)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Load2,             2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Load2)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Load3,             2, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Load3)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Store,             3, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Store)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Store1,            3, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Store1)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Store2,            3, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Store2)
LIBRARY_FUNCTION(SIMDInt32x4Lib,    Store3,            3, BIF_IgnoreDst                                                 , SIMDInt32x4Lib::EntryInfo::Store3)

LIBRARY_FUNCTION(SIMDInt16x8Lib,    Int16x8,           8, BIF_IgnoreDst                                                 , SIMDInt16x8Lib::EntryInfo::Int16x8)
LIBRARY_FUNCTION(SIMDInt8x16Lib,    Int8x16,          16, BIF_IgnoreDst                                                 , SIMDInt8x16Lib::EntryInfo::Int8x16)

LIBRARY_FUNCTION(SIMDUint32x4Lib,    Uint32x4,         4, BIF_IgnoreDst                                                 , SIMDUint32x4Lib::EntryInfo::Uint32x4)
LIBRARY_FUNCTION(SIMDUint16x8Lib,    Uint16x8,         8, BIF_IgnoreDst                                                 , SIMDUint16x8Lib::EntryInfo::Uint16x8)
LIBRARY_FUNCTION(SIMDUint8x16Lib,    Uint8x16,        16, BIF_IgnoreDst                                                 , SIMDUint8x16Lib::EntryInfo::Uint8x16)

LIBRARY_FUNCTION(SIMDBool32x4Lib,    Bool32x4,         4, BIF_IgnoreDst                                                 , SIMDBool32x4Lib::EntryInfo::Bool32x4)
LIBRARY_FUNCTION(SIMDBool16x8Lib,    Bool16x8,         8, BIF_IgnoreDst                                                 , SIMDBool16x8Lib::EntryInfo::Bool16x8)
LIBRARY_FUNCTION(SIMDBool8x16Lib,    Bool8x16,        16, BIF_IgnoreDst                                                 , SIMDBool8x16Lib::EntryInfo::Bool8x16)
#endif
