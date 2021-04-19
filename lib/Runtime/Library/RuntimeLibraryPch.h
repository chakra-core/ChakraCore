//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifndef IsJsDiag
#include "Parser.h"
#include "RegexCommon.h"
#include "Runtime.h"

#include "Base/EtwTrace.h"

#include "Library/JavascriptNumberObject.h"
#include "Library/String/JavascriptStringObject.h"
#include "Library/JavascriptBooleanObject.h"
#include "Library/JavascriptBigIntObject.h"

#include "Library/ObjectPrototypeObject.h"

#include "Common/ByteSwap.h"
#include "Library/DataView.h"

#include "Library/JSON/LazyJSONString.h"
#include "Library/JSON/JSONStringBuilder.h"
#include "Library/JSON/JSONStringifier.h"
#include "Library/String/ProfileString.h"
#include "Library/String/SingleCharString.h"
#include "Library/String/SubString.h"
#include "Library/String/BufferStringBuilder.h"

#include "Library/Functions/BoundFunction.h"
#include "Library/Generators/JavascriptGeneratorFunction.h"
#include "Library/Generators/JavascriptAsyncFunction.h"
#include "Library/Generators/JavascriptAsyncGeneratorFunction.h"

#include "Library/Regex/RegexHelper.h"
#include "Library/Regex/JavascriptRegularExpression.h"
#include "Library/Regex/JavascriptRegExpConstructor.h"
#include "Library/Regex/JavascriptRegularExpressionResult.h"

#include "Library/Iterators/JavascriptAsyncFromSyncIterator.h"
#include "Library/JavascriptPromise.h"
#include "Library/JavascriptSymbolObject.h"
#ifdef _CHAKRACOREBUILD
#include "Library/CustomExternalWrapperObject.h"
#endif
#include "Library/JavascriptProxy.h"
#include "Library/JavascriptReflect.h"
#include "Library/Generators/JavascriptGenerator.h"
#include "Library/Generators/JavascriptAsyncGenerator.h"

#include "Library/SameValueComparer.h"
#include "Library/MapOrSetDataList.h"
#include "Library/JavascriptMap.h"
#include "Library/JavascriptSet.h"
#include "Library/JavascriptWeakMap.h"
#include "Library/JavascriptWeakSet.h"

#include "Types/UnscopablesWrapperObject.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/DictionaryPropertyDescriptor.h"
#include "Types/DictionaryTypeHandler.h"
#include "Types/ES5ArrayTypeHandler.h"
#include "Library/Array/ES5Array.h"

#include "Library/Array/JavascriptArrayIndexEnumeratorBase.h"
#include "Library/Array/JavascriptArrayIndexEnumerator.h"
#include "Library/Array/JavascriptArrayIndexSnapshotEnumerator.h"
#include "Library/Array/JavascriptArrayIndexStaticEnumerator.h"
#include "Library/Array/ES5ArrayIndexEnumerator.h"
#include "Library/Array/ES5ArrayIndexStaticEnumerator.h"
#include "Library/Array/TypedArrayIndexEnumerator.h"
#include "Library/String/JavascriptStringEnumerator.h"
#include "Library/Regex/JavascriptRegExpEnumerator.h"

#include "Library/Iterators/JavascriptIterator.h"
#include "Library/Iterators/JavascriptArrayIterator.h"
#include "Library/Iterators/JavascriptMapIterator.h"
#include "Library/Iterators/JavascriptSetIterator.h"
#include "Library/String/JavascriptStringIterator.h"
#include "Library/Iterators/JavascriptListIterator.h"

#include "Library/UriHelper.h"
#include "Library/HostObjectBase.h"

#include "Library/DateImplementation.h"
#include "Library/JavascriptDate.h"

#include "Library/ModuleRoot.h"
#include "Library/Functions/ArgumentsObject.h"
// SIMD
#include "Language/SimdOps.h"

#include "Library/WASM/WebAssemblyInstance.h"

#include "Language/JavascriptStackWalker.h"
#include "Language/CacheOperators.h"
#include "Types/TypePropertyCache.h"
// .inl files
#include "Library/String/JavascriptString.inl"
#include "Library/String/ConcatString.inl"
#include "Language/CacheOperators.inl"

#endif // !IsJsDiag

#ifdef IsJsDiag
#define JS_DIAG_INLINE inline
#else
#define JS_DIAG_INLINE
#endif
