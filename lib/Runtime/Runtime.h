//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Common.h"

//========================
// Parser includes
//========================
#include "Parser.h"
#include "ParserCommon.h"
#include "ParseFlags.h"
#include "rterror.h"

// Parser forward decl
class FuncInfo;
class Scope;
class Symbol;
struct Ident;
typedef Ident *IdentPtr;

enum SymbolType : byte;

// Regex forward decl
namespace UnifiedRegex
{
    struct RegexPattern;
    template <typename T> class StandardChars;      // Used by ThreadContext.h
    struct TrigramAlphabet;
    struct RegexStacks;
#if ENABLE_REGEX_CONFIG_OPTIONS
    class DebugWriter;
    struct RegexStats;
    class RegexStatsDatabase;
#endif
};

//========================

#include "RuntimeCommon.h"

#include <intsafe.h>

#if !defined(UNREFERENCED_PARAMETER)
#define UNREFERENCED_PARAMETER(x) (x)
#endif

class SRCINFO;
class Lowerer;
class LowererMD;
class LowererMDArch;
class ByteCodeGenerator;
interface IActiveScriptDataCache;
class ActiveScriptProfilerHeapEnum;
class JITJavascriptString;

////////

#include "Debug/TTSupport.h"
#include "Debug/TTSerialize.h"

////////

namespace Js
{
    //
    // Forward declarations
    //
    class CharClassifier;
    typedef int32 MessageId;
    /* enum */ struct PropertyIds;
    class DebugDocument;
    struct Utf8SourceInfo;
    struct CallInfo;
    struct InlineeCallInfo;
    struct InlineCache;
    class PolymorphicInlineCache;
    struct Arguments;
    class StringDictionaryWrapper;
    struct ByteCodeDumper;
    struct ByteCodeReader;
    struct ByteCodeWriter;
    enum class EnumeratorFlags : byte;
    struct EnumeratorCache;
    class JavascriptStaticEnumerator;
    class ForInObjectEnumerator;
    class JavascriptConversion;
    class JavascriptDate;
    class DateImplementation;
    class BufferString;
    class BufferStringBuilder;
    class ConcatString;
    class CompoundString;
    class JavascriptBoolean;
    class JavascriptBooleanObject;
    class JavascriptSymbol;
    class JavascriptSymbolObject;
#ifdef _CHAKRACOREBUILD
    class CustomExternalWrapperObject;
#endif
    class JavascriptProxy;
    class JavascriptReflect;
    class JavascriptEnumeratorIterator;
    class JavascriptArrayIterator;
    enum class JavascriptArrayIteratorKind;
    class JavascriptAsyncFromSyncIterator;
    class JavascriptMapIterator;
    enum class JavascriptMapIteratorKind;
    class JavascriptSetIterator;
    enum class JavascriptSetIteratorKind;
    class JavascriptStringIterator;
    class JavascriptListIterator;
    class JavascriptPromise;
    class JavascriptPromiseCapability;
    class JavascriptPromiseReaction;
    class JavascriptPromiseCapabilitiesExecutorFunction;
    class JavascriptPromiseResolveOrRejectFunction;
    class JavascriptPromiseReactionTaskFunction;
    class JavascriptPromiseThenFinallyFunction;
    class JavascriptPromiseThunkFinallyFunction;
    class JavascriptPromiseResolveThenableTaskFunction;
    class JavascriptPromiseAllResolveElementFunction;
    class JavascriptPromiseAllSettledResolveOrRejectElementFunction;
    class JavascriptPromiseAnyRejectElementFunction;
    struct JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper;
    struct JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper;
    class JavascriptAsyncSpawnExecutorFunction;
    class JavascriptAsyncSpawnStepFunction;
    class JavascriptGenerator;
    class LiteralString;
    class JavascriptStringObject;
    class JavascriptBigIntObject;
    struct PropertyDescriptor;
    class Type;
    class DynamicType;
    class ScriptFunctionType;
    class DynamicTypeHandler;
    class DeferredTypeHandlerBase;
    template <bool IsPrototype> class NullTypeHandler;
    template<size_t size> class SimpleTypeHandler;
    class PathTypeHandlerBase;
    class IndexPropertyDescriptor;
    class DynamicObject;
    class ArrayObject;
    class UnscopablesWrapperObject;
    class SpreadArgument;
    class JavascriptString;
    class JavascriptBigInt;
    class StringCopyInfo;
    class StringCopyInfoStack;
    class ObjectPrototypeObject;
    class PropertyString;
    class PropertyRecordUsageCache;
    class ArgumentsObject;
    class HeapArgumentsObject;
    class ActivationObject;
    class JavascriptNumber;
    class JavascriptNumberObject;

    class ScriptContextProfiler;

    struct RestrictedErrorStrings;
    class JavascriptError;
    class JavascriptAggregateError;
    class RecyclableObject;
    class JavascriptRegExp;
    class JavascriptRegularExpressionResult;
    template<typename T> class SparseArraySegment;
    enum class DynamicObjectFlags : uint16;
    class JavascriptArray;
    class JavascriptNativeIntArray;
#if ENABLE_COPYONACCESS_ARRAY
    class JavascriptCopyOnAccessNativeIntArray;
#endif
    class JavascriptNativeFloatArray;
    class ES5Array;
    class JavascriptFunction;
    class ScriptFunction;
    class ScriptFunctionWithInlineCache;
    class StackScriptFunction;
    class GeneratorVirtualScriptFunction;
    class JavascriptGeneratorFunction;
    class JavascriptAsyncGeneratorFunction;
    class JavascriptAsyncFunction;
    class AsmJsScriptFunction;
    class WasmScriptFunction;
    class JavascriptRegExpConstructor;
    class JavascriptRegExpEnumerator;
    class BoundFunction;
    class JavascriptMap;
    class JavascriptSet;
    class JavascriptWeakMap;
    class JavascriptWeakSet;
    class DynamicObject;
    class HostObjectBase;
    class RootObjectBase;
    class ModuleRoot;
    class GlobalObject;
    class Math;
    class JavascriptOperators;
    class JavascriptLibrary;
    class JavascriptEncodeURI;
    class JavascriptEncodeURIComponent;
    class JavascriptDecodeURI;
    class JavascriptDecodeURIComponent;
    class DataView;
    struct ConstructorCache;
    enum class OpCode : ushort;
    enum class OpCodeAsmJs : ushort;
    /* enum */ struct OpLayoutType;
    /* enum */ struct OpLayoutTypeAsmJs;
    class ExceptionBase;
    class OutOfMemoryException;
    class ScriptDebug;
    class ScriptContext;
    struct NativeModule;
    template <class T> class RcRef;
    class TaggedInt;
    class TaggedNumber;
    struct InterpreterStackFrame;
    struct ScriptEntryExitRecord;
    class JavascriptStackWalker;
    struct AsmJsCallStackLayout;
    class JavascriptCallStackLayout;
    class Throw;
    struct Tick;
    struct TickDelta;
    class ByteBlock;
    class FunctionInfo;
    class FunctionProxy;
    class FunctionBody;
    class ParseableFunctionInfo;
    struct StatementLocation;
    class EntryPointInfo;
    struct LoopHeader;
    class InternalString;
    enum class JavascriptHint;
    /* enum */ struct BuiltinFunction;
    class EnterScriptObject;
    class PropertyRecord;
    struct IsInstInlineCache;
    class EntryPointInfo;
    class PolymorphicInlineCacheInfo;
    class PropertyGuard;

    class DetachedStateBase;

    // asm.js
    namespace ArrayBufferView
    {
        enum ViewType: uint8;
    }
    struct EmitExpressionInfo;
    struct AsmJsModuleMemory;
    namespace AsmJsLookupSource
    {
        enum Source: int;
    }
    struct AsmJsByteCodeWriter;
    class AsmJsArrayView;
    class AsmJsType;
    class AsmJsRetType;
    class AsmJsVarType;
    class AsmJsSymbol;
    class AsmJsVarBase;
    class AsmJsVar;
    class AsmJsConstantImport;
    class AsmJsArgument;
    class AsmJsFunc;
    class AsmJsFunctionDeclaration;
    class AsmJsFunctionInfo;
    class AsmJsModuleInfo;
    class AsmJsGlobals;
    class AsmJsImportFunction;
    class AsmJsTypedArrayFunction;
    class AsmJsMathFunction;
    class AsmJsMathConst;
#ifdef ASMJS_PLAT
    Var AsmJsExternalEntryPoint(Js::RecyclableObject* entryObject, Js::CallInfo callInfo, ...);
    class AsmJsCodeGenerator;
    class AsmJsEncoder;
#endif
    struct MathBuiltin;
    struct ExclusiveContext;
    class AsmJsModuleCompiler;
    class AsmJSCompiler;
    class AsmJSByteCodeGenerator;
    enum AsmJSMathBuiltinFunction: int;
    //////////////////////////////////////////////////////////////////////////
#if ENABLE_WEAK_REFERENCE_REGIONS
    template <typename T> using WeakPropertyIdMap = JsUtil::WeakReferenceRegionDictionary<PropertyId, T*, PrimeSizePolicy>;
#else
    template <typename T> using WeakPropertyIdMap = JsUtil::WeakReferenceDictionary<PropertyId, T, PrimeSizePolicy>;
#endif
    typedef WeakPropertyIdMap<PropertyString> PropertyStringCacheMap;
    typedef WeakPropertyIdMap<JavascriptSymbol> SymbolCacheMap;

    extern const FrameDisplay NullFrameDisplay;
    extern const FrameDisplay StrictNullFrameDisplay;

    enum ImplicitCallFlags : BYTE
    {
        ImplicitCall_HasNoInfo = 0x00,
        ImplicitCall_None = 0x01,
        ImplicitCall_ToPrimitive = 0x02 | ImplicitCall_None,
        ImplicitCall_Accessor = 0x04 | ImplicitCall_None,
        ImplicitCall_NonProfiledAccessor = 0x08 | ImplicitCall_None,
        ImplicitCall_External = 0x10 | ImplicitCall_None,
        ImplicitCall_Exception = 0x20 | ImplicitCall_None,
        ImplicitCall_NoOpSet = 0x40 | ImplicitCall_None,
        ImplicitCall_All = 0x7F,

        // Implicit call that is not caused by operations for the instruction (e.g. QC and GC dispose)
        // where we left script and enter script again. (Also see BEGIN_LEAVE_SCRIPT_INTERNAL)
        // This doesn't count as an implicit call on the recorded profile, but if it happens on JIT'ed code
        // it will still cause a bailout. Should happen very rarely.
        ImplicitCall_AsyncHostOperation = 0x80
    };
}

namespace TTD
{
    //typedef for a pin set (ensure that objects are kept live).
    typedef JsUtil::BaseHashSet<Js::PropertyRecord*, Recycler> PropertyRecordPinSet;
    typedef JsUtil::BaseHashSet<Js::FunctionBody*, Recycler> FunctionBodyPinSet;
    typedef JsUtil::BaseHashSet<Js::RecyclableObject*, Recycler> ObjectPinSet;
    typedef JsUtil::BaseHashSet<Js::FrameDisplay*, Recycler> EnvironmentPinSet;
    typedef JsUtil::BaseHashSet<Js::Var, Recycler> SlotArrayPinSet;
}

#include "PlatformAgnostic/ChakraPlatform.h"

bool IsMathLibraryId(Js::PropertyId propertyId);
#include "ByteCode/PropertyIdArray.h"
#include "ByteCode/AuxArray.h"
#include "ByteCode/VarArrayVarCount.h"

// module id
const Js::ModuleID kmodGlobal = 0;

class SourceContextInfo;

#if defined(ENABLE_SCRIPT_DEBUGGING) && defined(_WIN32)
#include "activdbg100.h"
#else
#define SCRIPT_E_RECORDED                _HRESULT_TYPEDEF_(0x86664004L)
#define NEED_DEBUG_EVENT_INFO_TYPE
#endif

#ifndef NTDDI_WIN10
// These are only defined for the Win10 SDK and above
#define NEED_DEBUG_EVENT_INFO_TYPE
#define SDO_ENABLE_LIBRARY_STACK_FRAME ((SCRIPT_DEBUGGER_OPTIONS)0x8)
#define DBGPROP_ATTRIB_VALUE_IS_RETURN_VALUE 0x8000000
#define DBGPROP_ATTRIB_VALUE_PENDING_MUTATION 0x10000000
#endif

#ifdef NEED_DEBUG_EVENT_INFO_TYPE
// Consider: Refactor to avoid needing these?
typedef
enum tagDEBUG_EVENT_INFO_TYPE
{
    DEIT_GENERAL = 0,
    DEIT_ASMJS_IN_DEBUGGING = (DEIT_GENERAL + 1),
    DEIT_ASMJS_SUCCEEDED = (DEIT_ASMJS_IN_DEBUGGING + 1),
    DEIT_ASMJS_FAILED = (DEIT_ASMJS_SUCCEEDED + 1)
} DEBUG_EVENT_INFO_TYPE;
#endif

#include "../JITIDL/JITTypes.h"
#include "../JITClient/JITManager.h"

#include "Base/SourceHolder.h"
#include "Base/LineOffsetCache.h"
#include "Base/Utf8SourceInfo.h"
#include "Base/PropertyRecord.h"
#ifdef ENABLE_GLOBALIZATION
#include "Base/DelayLoadLibrary.h"
#endif
#include "Base/CallInfo.h"
#include "Language/ExecutionMode.h"
#include "Types/TypeId.h"

#include "Base/Constants.h"
#include "BackendApi.h"
#include "Language/PropertyGuard.h"
#include "Language/ConstructorCache.h"
#include "ByteCode/OpLayoutsCommon.h"
#include "ByteCode/OpLayouts.h"
#include "ByteCode/OpLayoutsAsmJs.h"
#include "ByteCode/OpCodeUtil.h"
#include "Language/Arguments.h"

#include "Types/RecyclableObject.h"
#include "Base/ExpirableObject.h"
#include "Types/Type.h"
#include "Types/StaticType.h"
#include "Base/CrossSite.h"
#include "Base/CrossSiteObject.h"
#include "Types/JavascriptEnumerator.h"
#include "Types/DynamicObject.h"
#include "Types/ArrayObject.h"

#include "Types/TypePath.h"
#include "Types/TypeHandler.h"
#include "Types/SimplePropertyDescriptor.h"

#include "Types/DynamicType.h"

#include "Language/StackTraceArguments.h"
#include "Types/PropertyDescriptor.h"
#include "Types/ActivationObject.h"
#include "Base/TempArenaAllocatorObject.h"
#include "Language/ValueType.h"
#include "Language/DynamicProfileInfo.h"
#include "Base/SourceContextInfo.h"
#include "Language/InlineCache.h"
#include "Language/InlineCachePointerArray.h"
#include "Base/FunctionInfo.h"
#include "Base/FunctionBody.h"
#include "Language/JavascriptExceptionContext.h"
#include "Language/JavascriptExceptionObject.h"
#include "Base/PerfHint.h"

#include "ByteCode/ByteBlock.h"

#include "Library/JavascriptBuiltInFunctions.h"
#include "Library/String/JavascriptString.h"
#include "Library/String/StringCopyInfo.h"


#include "Library/JavascriptNumber.h"
#include "Library/Functions/JavascriptFunction.h"
#include "Library/Functions/BoundFunction.h"
#include "Library/Functions/RuntimeFunction.h"
#include "Library/Functions/JavascriptExternalFunction.h"
#include "Library/Iterators/CustomExternalIterator.h"

#include "Base/CharStringCache.h"

#include "Language/PrototypeChainCache.h"
#include "Library/JavascriptObject.h"
#include "Library/BuiltInFlags.h"
#include "Types/DynamicObjectPropertyEnumerator.h"
#include "Types/JavascriptStaticEnumerator.h"
#include "Library/ExternalLibraryBase.h"
#include "Library/JavascriptLibraryBase.h"
#include "Library/MathLibrary.h"
#include "Base/ThreadContextInfo.h"
#include "DataStructures/EvalMapString.h"
#include "Language/EvalMapRecord.h"
#include "Base/RegexPatternMruMap.h"
#include "Library/JavascriptLibrary.h"

#include "Language/JavascriptExceptionOperators.h"
#include "Language/JavascriptOperators.h"

#include "Library/WASM/WasmLibrary.h"
#include "Library/WASM/WabtInterface.h"
// xplat-todo: We should get rid of this altogether and move the functionality it
// encapsulates to the Platform Agnostic Interface
#ifdef _WIN32
#if defined(ENABLE_GLOBALIZATION) || ENABLE_UNICODE_API
#include "Base/WindowsGlobalizationAdapter.h"
#include "Base/WindowsFoundationAdapter.h"
#endif
#endif
#include "Base/Debug.h"

#ifdef _M_X64
#include "Language/amd64/StackFrame.h"
#endif

#include "Base/Entropy.h"
#ifdef ENABLE_BASIC_TELEMETRY
#include "DirectCall.h"
#include "ScriptContext/ScriptContextTelemetry.h"
#else
#define CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(builtin)
#define CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(esVersion, feature, m_scriptContext)
#endif

#include "Library/DelayFreeArrayBufferHelper.h"
#include "Base/ThreadContext.h"

#include "Base/StackProber.h"
#include "Base/ScriptContextProfiler.h"

#include "Language/JavascriptConversion.h"

#include "Base/ScriptContextOptimizationOverrideInfo.h"
#include "Base/ScriptContextBase.h"
#include "Base/ScriptContextInfo.h"
#include "Base/ScriptContext.h"
#include "Base/LeaveScriptObject.h"
#include "Base/PropertyRecord.h"

#include "ByteCode/ByteCodeReader.h"
#include "Language/TaggedInt.h"

#include "Library/RootObjectBase.h"
#include "Library/GlobalObject.h"

#include "Library/String/LiteralString.h"
#include "Library/String/ConcatString.h"
#include "Library/String/CompoundString.h"
#include "Library/PropertyRecordUsageCache.h"
#include "Library/String/PropertyString.h"
#include "Library/String/SingleCharString.h"

#include "Library/JavascriptTypedNumber.h"
#include "Library/Array/SparseArraySegment.h"
#include "Library/JavascriptError.h"
#include "Library/Array/JavascriptArray.h"
#include "Library/JavascriptSymbol.h"

#include "Library/AtomicsObject.h"
#include "DetachedStateBase.h"
#include "Library/ArrayBuffer.h"
#include "Library/SharedArrayBuffer.h"
#include "Library/Array/TypedArray.h"
#include "Library/JavascriptBoolean.h"
#include "Library/JavascriptBigInt.h"
#include "Library/WASM/WebAssemblyEnvironment.h"
#include "Library/WASM/WebAssemblyTable.h"
#include "Library/WASM/WebAssemblyMemory.h"
#include "Library/WASM/WebAssemblyModule.h"
#include "Library/WASM/WebAssembly.h"

#include "Language/ModuleRecordBase.h"
#include "Language/SourceTextModuleRecord.h"
//#include "Language/ModuleNamespace.h"
#include "Types/ScriptFunctionType.h"
#include "Library/Functions/ScriptFunction.h"
#include "Library/Functions/StackScriptFunction.h"

#ifdef _CHAKRACOREBUILD
#include "Library/CustomExternalWrapperObject.h"
#endif

#include "Library/JavascriptProxy.h"

#if ENABLE_TTD
#include "screrror.h"

#include "Debug/TTRuntimeInfoTracker.h"
#include "Debug/TTExecutionInfo.h"
#include "Debug/TTInflateMap.h"
#include "Debug/TTSnapTypes.h"
#include "Debug/TTSnapValues.h"
#include "Debug/TTSnapObjects.h"
#include "Debug/TTSnapshot.h"
#include "Debug/TTSnapshotExtractor.h"
#include "Debug/TTEvents.h"
#include "Debug/TTActionEvents.h"
#include "Debug/TTEventLog.h"
#endif

#include "Library/Generators/JavascriptGeneratorFunction.h"
#include "Library/Generators/JavascriptAsyncFunction.h"
#include "Library/Generators/JavascriptAsyncGeneratorFunction.h"

#include "../WasmReader/WasmReader.h"

#include "Language/AsmJsTypes.h"
#include "Language/AsmJsModule.h"
#include "Language/AsmJs.h"

#include "Core/JitHelperUtils.h"

//
// .inl files
//

#include "CommonInl.h"

#include "Language/JavascriptConversion.inl"
#include "Types/RecyclableObject.inl"
#include "Types/DynamicObject.inl"
#include "Library/Array/JavascriptArray.inl"
#include "Library/Array/SparseArraySegment.inl"
#include "Library/JavascriptNumber.inl"
#include "Library/JavascriptLibrary.inl"
#include "Language/InlineCache.inl"
#include "Language/InlineCachePointerArray.inl"
#include "Language/JavascriptOperators.inl"
#include "Language/TaggedInt.inl"

#ifndef USED_IN_STATIC_LIB
#ifdef ENABLE_INTL_OBJECT
#ifdef INTL_WINGLOB

//The "helper" methods below are to resolve external symbol references to our delay-loaded libraries.
inline HRESULT WindowsCreateString(_In_reads_opt_(length) const WCHAR * sourceString, UINT32 length, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string)
{
    return ThreadContext::GetContextForCurrentThread()->GetWindowsGlobalizationLibrary()->WindowsCreateString(sourceString, length, string);
}

inline HRESULT WindowsCreateStringReference(_In_reads_opt_(length + 1) const WCHAR * sourceString, UINT32 length, _Out_ HSTRING_HEADER * header, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string)
{
    return ThreadContext::GetContextForCurrentThread()->GetWindowsGlobalizationLibrary()->WindowsCreateStringReference(sourceString, length, header, string);
}

inline HRESULT WindowsDeleteString(_In_opt_ HSTRING string)
{
    return ThreadContext::GetContextForCurrentThread()->GetWindowsGlobalizationLibrary()->WindowsDeleteString(string);
}

inline PCWSTR WindowsGetStringRawBuffer(_In_opt_ HSTRING string, _Out_opt_ UINT32 * length)
{
    return ThreadContext::GetContextForCurrentThread()->GetWindowsGlobalizationLibrary()->WindowsGetStringRawBuffer(string, length);
}

inline HRESULT WindowsCompareStringOrdinal(_In_opt_ HSTRING string1, _In_opt_ HSTRING string2, _Out_ INT32 * result)
{
    return ThreadContext::GetContextForCurrentThread()->GetWindowsGlobalizationLibrary()->WindowsCompareStringOrdinal(string1, string2, result);
}

inline HRESULT WindowsDuplicateString(_In_opt_ HSTRING original, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *newString)
{
    return ThreadContext::GetContextForCurrentThread()->GetWindowsGlobalizationLibrary()->WindowsDuplicateString(original, newString);
}

#endif // INTL_WINGLOB
#endif // ENABLE_INTL_OBJECT
#endif // #ifndef USED_IN_STATIC_LIB
