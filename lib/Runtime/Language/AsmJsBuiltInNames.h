//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Default all macros to nothing

#ifndef ASMJS_JSBUILTIN_MATH_FUNC_NAMES
#define ASMJS_JSBUILTIN_MATH_FUNC_NAMES(propertyId, funcName)
#endif

#ifndef ASMJS_MATH_FUNC_NAMES
#define ASMJS_MATH_FUNC_NAMES(name, propertyName, funcInfo)
#endif

#ifndef ASMJS_MATH_CONST_NAMES
#define ASMJS_MATH_CONST_NAMES(name, propertyName, value)
#endif

#ifndef ASMJS_MATH_DOUBLE_CONST_NAMES
#define ASMJS_MATH_DOUBLE_CONST_NAMES(name, propertyName, value) ASMJS_MATH_CONST_NAMES(name, propertyName, value)
#endif

#ifndef ASMJS_ARRAY_NAMES
#define ASMJS_ARRAY_NAMES(name, propertyName)
#endif

#ifndef ASMJS_TYPED_ARRAY_NAMES
#define ASMJS_TYPED_ARRAY_NAMES(name, propertyName) ASMJS_ARRAY_NAMES(name, propertyName)
#endif

#ifdef ENABLE_JS_BUILTINS
ASMJS_JSBUILTIN_MATH_FUNC_NAMES(Js::PropertyIds::min,   Min     )
ASMJS_JSBUILTIN_MATH_FUNC_NAMES(Js::PropertyIds::max,   Max     )
#endif

ASMJS_MATH_FUNC_NAMES(sin,      sin,    Math::EntryInfo::Sin    )
ASMJS_MATH_FUNC_NAMES(cos,      cos,    Math::EntryInfo::Cos    )
ASMJS_MATH_FUNC_NAMES(tan,      tan,    Math::EntryInfo::Tan    )
ASMJS_MATH_FUNC_NAMES(asin,     asin,   Math::EntryInfo::Asin   )
ASMJS_MATH_FUNC_NAMES(acos,     acos,   Math::EntryInfo::Acos   )
ASMJS_MATH_FUNC_NAMES(atan,     atan,   Math::EntryInfo::Atan   )
ASMJS_MATH_FUNC_NAMES(ceil,     ceil,   Math::EntryInfo::Ceil   )
ASMJS_MATH_FUNC_NAMES(floor,    floor,  Math::EntryInfo::Floor  )
ASMJS_MATH_FUNC_NAMES(exp,      exp,    Math::EntryInfo::Exp    )
ASMJS_MATH_FUNC_NAMES(log,      log,    Math::EntryInfo::Log    )
ASMJS_MATH_FUNC_NAMES(pow,      pow,    Math::EntryInfo::Pow    )
ASMJS_MATH_FUNC_NAMES(sqrt,     sqrt,   Math::EntryInfo::Sqrt   )
ASMJS_MATH_FUNC_NAMES(abs,      abs,    Math::EntryInfo::Abs    )
ASMJS_MATH_FUNC_NAMES(atan2,    atan2,  Math::EntryInfo::Atan2  )
ASMJS_MATH_FUNC_NAMES(imul,     imul,   Math::EntryInfo::Imul   )
ASMJS_MATH_FUNC_NAMES(fround,   fround, Math::EntryInfo::Fround )
ASMJS_MATH_FUNC_NAMES(min,      min,    Math::EntryInfo::Min    )
ASMJS_MATH_FUNC_NAMES(max,      max,    Math::EntryInfo::Max    )
ASMJS_MATH_FUNC_NAMES(clz32,    clz32,  Math::EntryInfo::Clz32  )

ASMJS_MATH_DOUBLE_CONST_NAMES(e,        E,        Math::E       )
ASMJS_MATH_DOUBLE_CONST_NAMES(ln10,     LN10,     Math::LN10    )
ASMJS_MATH_DOUBLE_CONST_NAMES(ln2,      LN2,      Math::LN2     )
ASMJS_MATH_DOUBLE_CONST_NAMES(log2e,    LOG2E,    Math::LOG2E   )
ASMJS_MATH_DOUBLE_CONST_NAMES(log10e,   LOG10E,   Math::LOG10E  )
ASMJS_MATH_DOUBLE_CONST_NAMES(pi,       PI,       Math::PI      )
ASMJS_MATH_DOUBLE_CONST_NAMES(sqrt1_2,  SQRT1_2,  Math::SQRT1_2 )
ASMJS_MATH_DOUBLE_CONST_NAMES(sqrt2,    SQRT2,    Math::SQRT2   )
ASMJS_MATH_CONST_NAMES(infinity, Infinity, 0             )
ASMJS_MATH_CONST_NAMES(nan,      NaN,      0             )

ASMJS_TYPED_ARRAY_NAMES(Uint8Array,   Uint8Array)
ASMJS_TYPED_ARRAY_NAMES(Int8Array,    Int8Array)
ASMJS_TYPED_ARRAY_NAMES(Uint16Array,  Uint16Array)
ASMJS_TYPED_ARRAY_NAMES(Int16Array,   Int16Array)
ASMJS_TYPED_ARRAY_NAMES(Uint32Array,  Uint32Array)
ASMJS_TYPED_ARRAY_NAMES(Int32Array,   Int32Array)
ASMJS_TYPED_ARRAY_NAMES(Float32Array, Float32Array)
ASMJS_TYPED_ARRAY_NAMES(Float64Array, Float64Array)
ASMJS_ARRAY_NAMES(byteLength,   byteLength)

// help the caller to undefine all the macros
#undef ASMJS_JSBUILTIN_MATH_FUNC_NAMES
#undef ASMJS_MATH_FUNC_NAMES
#undef ASMJS_MATH_CONST_NAMES
#undef ASMJS_MATH_DOUBLE_CONST_NAMES
#undef ASMJS_ARRAY_NAMES
#undef ASMJS_TYPED_ARRAY_NAMES
