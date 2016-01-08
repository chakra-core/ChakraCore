//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Common.h"

//========================
// Parser includes
//========================
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

////////
//The default number of elements per arrayblock, the size of "small" arrays and the block size we use when extracting
#define TTD_ARRAY_BLOCK_SIZE 0x200
#define TTD_ARRAY_SMALL_ARRAY 0x100

//The ids for objects in a snapshot
typedef uint64 TTD_PTR_ID;
#define TTD_INVALID_PTR_ID 0ul

#define TTD_CONVERT_VAR_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_TYPEINFO_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_ENV_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_SLOTARRAY_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)
#define TTD_CONVERT_SCOPE_TO_PTR_ID(X) reinterpret_cast<TTD_PTR_ID>(X)

#define TTD_EXTRACT_CTX_LOG_TAG(X) ((X)->ScriptContextLogTag)

#define TTD_COERCE_PTR_ID_TO_VAR(X) (reinterpret_cast<Js::Var*>(X))

typedef uint64 TTD_LOG_TAG;
#define TTD_INVALID_LOG_TAG 0l
#define TTD_INITIAL_LOG_TAG 1l

#define TTD_INCREMENT_LOG_TAG(X) ((X)++)

typedef int64 TTD_IDENTITY_TAG;
#define TTD_INVALID_IDENTITY_TAG 0l
#define TTD_INITIAL_IDENTITY_TAG 1l

#define TTD_INCREMENT_IDENTITY_TAG(X) ((X)++)

//The representation of an identifier (currently access path) for a well known object/primitive/function body/etc. in the JS engine or HOST
typedef LPCWSTR TTD_WELLKNOWN_TOKEN;
#define TTD_INVALID_WELLKNOWN_TOKEN nullptr

#include "Debug\TTSupport.h"

namespace TTD
{
    //typedef for a pin set (ensure that objects are kept live).
    typedef JsUtil::BaseHashSet<FinalizableObject*, Recycler> ReferencePinSet;

    class RuntimeThreadInfo;
    class RuntimeContextInfo;

    //Mode of the time-travel debugger
    enum class TTDMode
    {
        Invalid = 0x0,
        Disabled = 0x1,  //The system is being run in normal mode (with no TTD features)
        Detached = 0x2,  //The system has completed running (e.g. contexts have been detached) and we are done
        RecordEnabled = 0x4,     //The system is being run in Record mode
        DebuggingEnabled = 0x8,  //The system is being run in Debug Replay mode
        TTDActive = (RecordEnabled | DebuggingEnabled),

        ExcludedExecution = 0x20,  //Set when the system is executing code on behalf of the TTD/debugger (so we don't want to record/replay things for it)
        TTDTagActive = (RecordEnabled | DebuggingEnabled) & (~ExcludedExecution)
    };
    DEFINE_ENUM_FLAG_OPERATORS(TTDMode)

        //We typedef Js::Var into a TTD version that has the same bit layout but we want to avoid confusion  
        //if this bit layout is for the "live" state or potentially only for the snapshot state or the representations change later
        typedef Js::Var TTDVar;

    namespace NSSnapType
    {
        enum class SnapTypeHandlerTag : int;

        struct SnapPropertyRecord;
        struct SnapHandlerPropertyEntry;
        struct SnapHandler;
        struct SnapType;
    }

    namespace NSSnapValues
    {
        struct SnapPrimitiveValue;
        struct SlotArrayInfo;
        struct ScriptFunctionScopeInfo;
    }

    namespace NSSnapObjects
    {
        enum class SnapObjectType : int;

        struct SnapObject;
    }

    class SnapShot;
    class SnapshotExtractor;

    class EventTime;
    class TTDExceptionFramePopper;
    struct SingleCallCounter;
    class EventLogEntry;
    class JsRTActionLogEntry;
    class EventLog;

    class TTDebuggerAbortException;
}
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
    struct PolymorphicInlineCache;
    struct Arguments;
    class StringDictionaryWrapper;
    struct ByteCodeDumper;
    struct ByteCodeReader;
    struct ByteCodeWriter;
    class JavascriptConversion;
    class JavascriptDate;
    class JavascriptVariantDate;
    class DateImplementation;
    class BufferString;
    class BufferStringBuilder;
    class ConcatString;
    class CompoundString;
    class JavascriptBoolean;
    class JavascriptBooleanObject;
    class JavascriptSymbol;
    class JavascriptSymbolObject;
    class JavascriptProxy;
    class JavascriptReflect;
    class JavascriptEnumeratorIterator;
    class JavascriptArrayIterator;
    enum class JavascriptArrayIteratorKind;
    class JavascriptMapIterator;
    enum class JavascriptMapIteratorKind;
    class JavascriptSetIterator;
    enum class JavascriptSetIteratorKind;
    class JavascriptStringIterator;
    class JavascriptPromise;
    class JavascriptPromiseCapability;
    class JavascriptPromiseReaction;
    class JavascriptPromiseAsyncSpawnExecutorFunction;
    class JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction;
    class JavascriptPromiseCapabilitiesExecutorFunction;
    class JavascriptPromiseResolveOrRejectFunction;
    class JavascriptPromiseReactionTaskFunction;
    class JavascriptPromiseResolveThenableTaskFunction;
    class JavascriptPromiseAllResolveElementFunction;
    struct JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper;
    struct JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper;
    class JavascriptGenerator;
    class LiteralString;
    class ArenaLiteralString;
    class JavascriptStringObject;
    struct PropertyDescriptor;
    class Type;
    class DynamicType;
    class ScriptFunctionType;
    class DynamicTypeHandler;
    class DeferredTypeHandlerBase;
    template <bool IsPrototype> class NullTypeHandler;
    template<size_t size> class SimpleTypeHandler;
    class PathTypeHandler;
    class IndexPropertyDescriptor;
    class DynamicObject;
    class ArrayObject;
    class WithScopeObject;
    class SpreadArgument;
    class JavascriptString;
    class StringCopyInfo;
    class StringCopyInfoStack;
    class ObjectPrototypeObject;
    class PropertyString;
    class ArgumentsObject;
    class HeapArgumentsObject;
    class ActivationObject;
    class JavascriptNumber;
    class JavascriptNumberObject;

    class ES5ArgumentsObjectEnumerator;
    class ScriptContextProfiler;

    struct RestrictedErrorStrings;
    class JavascriptError;
    class NullEnumerator;
//SIMD_JS
    // SIMD
    class SIMDFloat32x4Lib;
    class JavascriptSIMDFloat32x4;
    class SIMDFloat64x2Lib;
    class JavascriptSIMDFloat64x2;
    class SIMDInt32x4Lib;
    class JavascriptSIMDInt32x4;
    class SIMDInt8x16Lib;
    class JavascriptSIMDInt8x16;

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
    class AsmJsScriptFunction;
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
    class FunctionBody;
    class ParseableFunctionInfo;
    struct StatementLocation;
    class EntryPointInfo;
    struct LoopHeader;
    class InternalString;
    /* enum */ struct JavascriptHint;
    /* enum */ struct BuiltinFunction;
    class EnterScriptObject;
    class PropertyRecord;
    struct IsInstInlineCache;
    class EntryPointInfo;
    class PolymorphicInlineCacheInfo;
    class PropertyGuard;

    // asm.js
    namespace ArrayBufferView
    {
        enum ViewType;
    }
    struct EmitExpressionInfo;
    struct AsmJsModuleMemory;
    namespace AsmJsLookupSource
    {
        enum Source;
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
    class AsmJsCodeGenerator;
    class AsmJsEncoder;
#endif
    struct MathBuiltin;
    struct ExclusiveContext;
    class AsmJsModuleCompiler;
    class AsmJSCompiler;
    class AsmJSByteCodeGenerator;
    enum AsmJSMathBuiltinFunction;
    //////////////////////////////////////////////////////////////////////////
    typedef JsUtil::WeakReferenceDictionary<PropertyId, PropertyString, PowerOf2SizePolicy> PropertyStringCacheMap;

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

#include "DataStructures\EvalMapString.h"

bool IsMathLibraryId(Js::PropertyId propertyId);
#include "ByteCode\PropertyIdArray.h"
#include "ByteCode\AuxArray.h"
#include "ByteCode\VarArrayVarCount.h"

// module id
const Js::ModuleID kmodGlobal = 0;

class SourceContextInfo;


#include "activdbg100.h"
#ifndef NTDDI_WIN10
// These are only defined for the Win10 SDK and above
// Consider: Refactor to avoid needing these?
typedef
enum tagDEBUG_EVENT_INFO_TYPE
{
    DEIT_GENERAL = 0,
    DEIT_ASMJS_IN_DEBUGGING = (DEIT_GENERAL + 1),
    DEIT_ASMJS_SUCCEEDED = (DEIT_ASMJS_IN_DEBUGGING + 1),
    DEIT_ASMJS_FAILED = (DEIT_ASMJS_SUCCEEDED + 1)
} DEBUG_EVENT_INFO_TYPE;

#define SDO_ENABLE_LIBRARY_STACK_FRAME ((SCRIPT_DEBUGGER_OPTIONS)0x8)
#endif

#include "Base\SourceHolder.h"
#include "Base\Utf8SourceInfo.h"
#include "Base\PropertyRecord.h"
#include "Base\DelayLoadLibrary.h"
#include "Base\CallInfo.h"
#include "Language\ExecutionMode.h"
#include "BackEndAPI.h"
#include "DetachedStateBase.h"

#include "Base\Constants.h"
#include "ByteCode\OpLayoutsCommon.h"
#include "ByteCode\OpLayouts.h"
#include "ByteCode\OpLayoutsAsmJs.h"
#include "ByteCode\OpCodeUtil.h"
#include "Language\Arguments.h"

#include "Types\TypeId.h"
#include "Types\RecyclableObject.h"
#include "Base\ExpirableObject.h"
#include "Types\Type.h"
#include "Types\StaticType.h"
#include "Base\CrossSite.h"
#include "Base\CrossSiteObject.h"
#include "Base\CrossSiteEnumerator.h"
#include "Types\JavascriptEnumerator.h"
#include "Types\DynamicObject.h"
#include "Types\ArrayObject.h"

#include "Types\TypePath.h"
#include "Types\TypeHandler.h"
#include "Types\SimplePropertyDescriptor.h"

#include "Types\DynamicType.h"

#include "Language\StackTraceArguments.h"
#include "Types\PropertyDescriptor.h"
#include "Types\ActivationObject.h"
#include "Base\TempArenaAllocatorObject.h"
#include "Language\ValueType.h"
#include "Language\DynamicProfileInfo.h"
#include "Debug\SourceContextInfo.h"
#include "Language\InlineCache.h"
#include "Language\InlineCachePointerArray.h"
#include "Base\FunctionInfo.h"
#include "Base\FunctionBody.h"
#include "Language\JavascriptExceptionContext.h"
#include "Language\JavascriptExceptionObject.h"
#include "Base\PerfHint.h"

#include "ByteCode\ByteBlock.h"

#include "Library\JavascriptBuiltInFunctions.h"
#include "Library\JavascriptString.h"
#include "Library\StringCopyInfo.h"


#include "Library\JavascriptNumber.h"
#include "Library\JavascriptFunction.h"
#include "Library\RuntimeFunction.h"
#include "Library\JavascriptExternalFunction.h"

#include "Base\CharStringCache.h"

#include "Library\JavascriptObject.h"
#include "Library\BuiltInFlags.h"
#include "Library\ExternalLibraryBase.h"
#include "Library\JavascriptLibraryBase.h"
#include "Library\JavascriptLibrary.h"

#include "Language\JavascriptExceptionOperators.h"
#include "Language\JavascriptOperators.h"

#include "Library\MathLibrary.h"


#include "Base\HiResTimer.h"
#include "Base\WindowsGlobalizationAdapter.h"
#include "Base\WindowsFoundationAdapter.h"
#include "Base\Debug.h"

#ifdef _M_X64
#include "Language\amd64\stackframe.h"
#endif

#include "Base\Entropy.h"
#ifdef ENABLE_BASIC_TELEMETRY
#include "DirectCall.h"
#include "LanguageTelemetry.h"
#else
#define CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(builtin)
#define CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(feature, m_scriptContext)
#endif
#include "Base\ThreadContext.h"

#include "Base\StackProber.h"

#include "Language\EvalMapRecord.h"
#include "Base\RegexPatternMruMap.h"
#include "Language\JavascriptConversion.h"

#include "Base\ScriptContextOptimizationOverrideInfo.h"
#include "Base\scriptContextbase.h"
#include "Base\ScriptContext.h"
#include "Base\LeaveScriptObject.h"
#include "Base\PropertyRecord.h"

#include "ByteCode\ByteCodeReader.h"
#include "Language\TaggedInt.h"

#include "Library\RootObjectBase.h"
#include "Library\GlobalObject.h"

#include "Library\LiteralString.h"
#include "Library\ConcatString.h"
#include "Library\CompoundString.h"
#include "Library\PropertyString.h"

#include "Library\JavascriptTypedNumber.h"
#include "Library\SparseArraySegment.h"
#include "Library\JavascriptError.h"
#include "Library\JavascriptArray.h"

#include "Library\ArrayBuffer.h"
#include "Library\TypedArray.h"
#include "Library\JavascriptBoolean.h"

#include "Types\ScriptFunctionType.h"
#include "Library\ScriptFunction.h"

#if ENABLE_TTD
#include "screrror.h"

#include "Debug\TTRuntimeInfoTracker.h"
#include "Debug\TTInflateMap.h"
#include "Debug\TTSerialize.h"
#include "Debug\TTSnapTypes.h"
#include "Debug\TTSnapValues.h"
#include "Debug\TTSnapObjects.h"
#include "Debug\TTSnapshot.h"
#include "Debug\TTSnapshotExtractor.h"
#include "Debug\TTEvents.h"
#include "Debug\TTActionEvents.h"
#include "Debug\TTEventLog.h"
#endif

//
// .inl files
//

#include "commoninl.h"

#include "Language\JavascriptConversion.inl"
#include "Types\RecyclableObject.inl"
#include "Types\DynamicObject.inl"
#include "Library\JavascriptBoolean.inl"
#include "Library\JavascriptArray.inl"
#include "Library\SparseArraySegment.inl"
#include "Library\JavascriptNumber.inl"
#include "Library\JavascriptLibrary.inl"
#include "Language\InlineCache.inl"
#include "Language\InlineCachePointerArray.inl"
#include "Language\JavascriptOperators.inl"
#include "Language\TaggedInt.inl"


#ifndef USED_IN_STATIC_LIB
#ifdef ENABLE_INTL_OBJECT

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
#endif
#endif