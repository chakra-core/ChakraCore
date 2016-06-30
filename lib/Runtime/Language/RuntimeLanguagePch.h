//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Parser.h"

#include "Runtime.h"

#include "Language/AsmJsTypes.h"
#include "Language/AsmJsUtils.h"
#include "Language/AsmJsLink.h"
#include "Language/AsmJsModule.h"
#include "Language/AsmJs.h"
#ifdef ASMJS_PLAT
#include "Language/AsmJsJitTemplate.h"
#include "Language/AsmJsEncoder.h"
#include "Language/AsmJsCodeGenerator.h"
#endif
#include "Language/ObjTypeSpecFldInfo.h"
#include "Language/FunctionCodeGenJitTimeData.h"

#include "Language/ProfilingHelpers.h"
#include "Language/CacheOperators.h"

#include "Language/JavascriptMathOperators.h"
#include "Language/JavascriptStackWalker.h"
#ifdef DYNAMIC_PROFILE_STORAGE
#include "Language/DynamicProfileStorage.h"
#endif
#include "Language/SourceDynamicProfileManager.h"

#include "Base/EtwTrace.h"

#include "Library/ArgumentsObject.h"

#include "Types/TypePropertyCache.h"
#include "Library/JavascriptVariantDate.h"
#include "Library/JavascriptProxy.h"
#include "Library/JavascriptSymbol.h"
#include "Library/JavascriptSymbolObject.h"
#include "Library/JavascriptGenerator.h"
#include "Library/StackScriptFunction.h"
#include "Library/HostObjectBase.h"

#ifdef ENABLE_MUTATION_BREAKPOINT
// REVIEW: ChakraCore Dependency
#include "activdbg_private.h"
#include "Debug/MutationBreakpoint.h"
#endif

// SIMD_JS
#include "Library/SimdLib.h"
#include "Language/SimdOps.h"
#include "Language/SimdUtils.h"

#include "Debug/DebuggingFlags.h"
#include "Debug/DiagProbe.h"
#include "Debug/DebugManager.h"
#include "Debug/ProbeContainer.h"
#include "Debug/DebugContext.h"

#ifdef ENABLE_BASIC_TELEMETRY
#include "ScriptContextTelemetry.h"
#endif

// .inl files
#include "Language/CacheOperators.inl"
#include "Language/JavascriptMathOperators.inl"
