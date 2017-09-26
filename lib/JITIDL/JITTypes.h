//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "CommonDefines.h"

#ifdef __midl
import "wtypes.idl";
#include "sdkddkver.h"
#endif

#if defined(WINVER) && WINVER >= _WIN32_WINNT_WINBLUE // on 8.1+, RPC can marshal process handle for us
#ifdef __midl
cpp_quote("#define USE_RPC_HANDLE_MARSHALLING 1")
#endif
#define USE_RPC_HANDLE_MARSHALLING 1
#endif

#if defined(_M_IX86) || defined(_M_ARM)
#ifdef __midl
#define CHAKRA_WB_PTR int
#else
#define CHAKRA_WB_PTR void*
#endif
#define CHAKRA_PTR int
#define BV_SHIFT 5
#elif defined(_M_X64) || defined(_M_ARM64)
#ifdef __midl
#define CHAKRA_WB_PTR __int64
#else
#define CHAKRA_WB_PTR void*
#endif
#define CHAKRA_PTR __int64
#define BV_SHIFT 6
#endif

#ifdef __midl
#define IDL_DEF(def) def
#else
#define IDL_DEF(def)
#endif

#define IDL_PAD1(num) IDL_Field(byte) struct_pad_##num;
#define IDL_PAD2(num) IDL_Field(short) struct_pad_##num;
#define IDL_PAD4(num) IDL_Field(int) struct_pad_##num;

#if defined(_M_X64) || defined(_M_ARM64)
#define X64_PAD4(num) IDL_Field(int) struct_pad_##num;
#else
#define X64_PAD4(num)
#endif

#if defined(_M_IX86) || defined(_M_ARM)
#define X86_PAD4(num) IDL_Field(int) struct_pad_##num;
#else
#define X86_PAD4(num)
#endif

#ifndef __midl
#ifndef _MSC_VER
typedef unsigned char boolean;
#endif
#endif

#ifdef __midl
#define IDL_Field(type)             type
#define IDL_FieldNoBarrier(type)    type
#else
#define IDL_Field(type)             Field(type)
#define IDL_FieldNoBarrier(type)    FieldNoBarrier(type)
#endif

#ifndef __JITTypes_h__
#define __JITTypes_h__

// TODO: OOP JIT, how do we make this better?
const int VTABLE_COUNT = 48;
const int EQUIVALENT_TYPE_CACHE_SIZE = 8;

typedef IDL_DEF([context_handle]) void * PTHREADCONTEXT_HANDLE;
typedef IDL_DEF([ref]) PTHREADCONTEXT_HANDLE * PPTHREADCONTEXT_HANDLE;

typedef IDL_DEF([context_handle]) void * PSCRIPTCONTEXT_HANDLE;
typedef IDL_DEF([ref]) PSCRIPTCONTEXT_HANDLE * PPSCRIPTCONTEXT_HANDLE;

typedef struct TypeHandlerIDL
{
    IDL_Field(boolean) isObjectHeaderInlinedTypeHandler;
    IDL_Field(boolean) isLocked;

    IDL_Field(unsigned short) inlineSlotCapacity;
    IDL_Field(unsigned short) offsetOfInlineSlots;
    IDL_PAD2(0)
    X64_PAD4(1)
    IDL_Field(int) slotCapacity;
} TypeHandlerIDL;

typedef struct TypeIDL
{
    IDL_Field(boolean) exists;
    IDL_Field(unsigned char) flags;
    IDL_Field(boolean) isShared;
    IDL_PAD1(0)
    IDL_Field(int) typeId;

    IDL_Field(CHAKRA_WB_PTR) libAddr;
    IDL_Field(CHAKRA_WB_PTR) protoAddr;
    IDL_Field(CHAKRA_PTR) entrypointAddr;
    IDL_Field(CHAKRA_WB_PTR) propertyCacheAddr;
    IDL_Field(CHAKRA_WB_PTR) addr;

    IDL_Field(TypeHandlerIDL) handler;
} TypeIDL;

typedef struct EquivalentTypeSetIDL
{
    IDL_Field(boolean) sortedAndDuplicatesRemoved;
    IDL_PAD1(0)
    IDL_Field(unsigned short) count;
    X64_PAD4(1)
    IDL_DEF([size_is(count)]) IDL_Field(TypeIDL *)* types;
} EquivalentTypeSetIDL;

typedef struct FixedFieldIDL
{
    IDL_Field(boolean) nextHasSameFixedField;
    IDL_Field(boolean) isClassCtor;
    IDL_Field(unsigned short) valueType;
    IDL_Field(unsigned int) localFuncId;
    IDL_Field(TypeIDL) type;
    IDL_Field(CHAKRA_WB_PTR) fieldValue;
    IDL_Field(CHAKRA_WB_PTR) funcInfoAddr;
    IDL_Field(CHAKRA_WB_PTR) environmentAddr;
} FixedFieldIDL;

typedef struct JITTimeConstructorCacheIDL
{
    IDL_Field(boolean) skipNewScObject;
    IDL_Field(boolean) ctorHasNoExplicitReturnValue;
    IDL_Field(boolean) typeIsFinal;
    IDL_Field(boolean) isUsed;

    IDL_Field(short) inlineSlotCount;

    IDL_PAD2(0)
    IDL_Field(int) slotCount;

    X64_PAD4(1)
    IDL_Field(TypeIDL) type;

    IDL_Field(CHAKRA_WB_PTR) runtimeCacheAddr;
    IDL_Field(CHAKRA_WB_PTR) runtimeCacheGuardAddr;
    IDL_Field(CHAKRA_PTR) guardedPropOps;
} JITTimeConstructorCacheIDL;

typedef struct ObjTypeSpecFldIDL
{
    IDL_Field(unsigned short) flags;
    IDL_Field(unsigned short) slotIndex;
    IDL_Field(unsigned short) fixedFieldCount;
    IDL_Field(unsigned short) fixedFieldInfoArraySize; // 1 (when fixedFieldCount is 0) or fixedFieldCount
    IDL_Field(int) propertyId;
    IDL_Field(int) typeId;
    IDL_Field(unsigned int) id;
    X64_PAD4(0)
    IDL_Field(CHAKRA_WB_PTR) protoObjectAddr;
    IDL_Field(CHAKRA_WB_PTR) propertyGuardValueAddr;
    IDL_Field(EquivalentTypeSetIDL *) typeSet;
    IDL_Field(TypeIDL *) initialType;
    IDL_Field(JITTimeConstructorCacheIDL *) ctorCache;
    IDL_DEF([size_is(fixedFieldInfoArraySize)]) IDL_Field(FixedFieldIDL *) fixedFieldInfoArray;
} ObjTypeSpecFldIDL;

typedef struct PinnedTypeRefsIDL
{
    boolean isOOPJIT;// REVIEW: remove this
    IDL_PAD1(0)
    IDL_PAD2(1)
    unsigned int count;
    IDL_DEF([size_is(count)]) CHAKRA_PTR typeRefs[IDL_DEF(*)];

} PinnedTypeRefsIDL;

typedef struct BVUnitIDL
{
    unsigned CHAKRA_PTR word;
} BVUnitIDL;

typedef struct BVFixedIDL
{
    unsigned int len;
    X64_PAD4(0)
    IDL_DEF([size_is(((len - 1) >> BV_SHIFT) + 1)]) BVUnitIDL data[IDL_DEF(*)];
} BVFixedIDL;

typedef struct BVSparseNodeIDL
{
    struct BVSparseNodeIDL * next;
    unsigned int startIndex;
    X64_PAD4(0)
    __int64 data;
} BVSparseNodeIDL;

typedef struct CallSiteIDL
{
    unsigned short bitFields;
    unsigned short returnType;
    unsigned int ldFldInlineCacheId;
    unsigned int sourceId;
    unsigned int functionId;
} CallSiteIDL;

typedef struct ThisIDL
{
    unsigned short valueType;
    byte thisType;
    IDL_PAD1(0)
} ThisIDL;

typedef struct FldIDL
{
    unsigned short valueType;
    byte flags;
    byte polymorphicInlineCacheUtilization;
} FldIDL;

typedef struct ArrayCallSiteIDL
{
    byte bits;
#if DBG
    IDL_PAD1(0)
    IDL_PAD2(1)
    unsigned int functionNumber;
    unsigned short callSiteNumber;
    IDL_PAD2(2)
#endif
} ArrayCallSiteIDL;

typedef struct LdElemIDL
{
    unsigned short arrayType;
    unsigned short elemType;
    byte bits;
    IDL_PAD1(0)
} LdElemIDL;

typedef struct StElemIDL
{
    unsigned short arrayType;
    byte bits;
    IDL_PAD1(0)
} StElemIDL;

typedef struct ProfileDataIDL
{
    byte implicitCallFlags;
    IDL_PAD1(0)

    ThisIDL thisData;

    unsigned short profiledLdElemCount;
    unsigned short profiledStElemCount;
    unsigned short profiledArrayCallSiteCount;

    unsigned short profiledSlotCount;
    unsigned short profiledCallSiteCount;

    unsigned short profiledReturnTypeCount;
    unsigned short profiledDivOrRemCount;
    unsigned short profiledSwitchCount;
    unsigned short profiledInParamsCount;

    unsigned int inlineCacheCount;
    unsigned int loopCount;

    BVFixedIDL * loopFlags;

    IDL_DEF([size_is(profiledLdElemCount)]) LdElemIDL * ldElemData;

    IDL_DEF([size_is(profiledStElemCount)]) StElemIDL * stElemData;

    IDL_DEF([size_is(profiledArrayCallSiteCount)]) ArrayCallSiteIDL * arrayCallSiteData;

    // TODO: michhol OOP JIT, share counts with body
    IDL_DEF([size_is(inlineCacheCount)]) FldIDL * fldData;

    IDL_DEF([size_is(profiledSlotCount)]) unsigned short * slotData;

    IDL_DEF([size_is(profiledCallSiteCount)]) CallSiteIDL * callSiteData;

    IDL_DEF([size_is(profiledReturnTypeCount)]) unsigned short * returnTypeData;

    IDL_DEF([size_is(profiledDivOrRemCount)]) unsigned short * divideTypeInfo;

    IDL_DEF([size_is(profiledSwitchCount)]) unsigned short * switchTypeInfo;

    IDL_DEF([size_is(profiledInParamsCount)]) unsigned short * parameterInfo;

    IDL_DEF([size_is(loopCount)]) byte * loopImplicitCallFlags;

    CHAKRA_PTR arrayCallSiteDataAddr;
    CHAKRA_PTR fldDataAddr;
    __int64 flags;
} ProfileDataIDL;

typedef struct ThreadContextDataIDL
{
    boolean isThreadBound;
    boolean allowPrereserveAlloc;

    IDL_PAD2(0)
    X64_PAD4(1)
    CHAKRA_PTR chakraBaseAddress;
    CHAKRA_PTR crtBaseAddress;
    CHAKRA_PTR threadStackLimitAddr;
    CHAKRA_PTR scriptStackLimit;
    CHAKRA_PTR bailOutRegisterSaveSpaceAddr;
    CHAKRA_PTR disableImplicitFlagsAddr;
    CHAKRA_PTR implicitCallFlagsAddr;
    CHAKRA_PTR stringReplaceNameAddr;
    CHAKRA_PTR stringMatchNameAddr;
    CHAKRA_PTR simdTempAreaBaseAddr;
} ThreadContextDataIDL;

typedef struct ScriptContextDataIDL
{
    boolean isRecyclerVerifyEnabled;
    boolean recyclerAllowNativeCodeBumpAllocation;
#ifdef ENABLE_SIMDJS
    boolean isSIMDEnabled;
#else
    IDL_PAD1(0)
#endif
    IDL_PAD1(1)
    unsigned int recyclerVerifyPad;
    CHAKRA_PTR vtableAddresses[VTABLE_COUNT];

    CHAKRA_PTR nullAddr;
    CHAKRA_PTR undefinedAddr;
    CHAKRA_PTR trueAddr;
    CHAKRA_PTR falseAddr;
    CHAKRA_PTR undeclBlockVarAddr;
    CHAKRA_PTR scriptContextAddr;
    CHAKRA_PTR emptyStringAddr;
    CHAKRA_PTR negativeZeroAddr;
    CHAKRA_PTR numberTypeStaticAddr;
    CHAKRA_PTR stringTypeStaticAddr;
    CHAKRA_PTR objectTypeAddr;
    CHAKRA_PTR objectHeaderInlinedTypeAddr;
    CHAKRA_PTR regexTypeAddr;
    CHAKRA_PTR arrayTypeAddr;
    CHAKRA_PTR nativeIntArrayTypeAddr;
    CHAKRA_PTR nativeFloatArrayTypeAddr;
    CHAKRA_PTR arrayConstructorAddr;
    CHAKRA_PTR charStringCacheAddr;
    CHAKRA_PTR libraryAddr;
    CHAKRA_PTR globalObjectAddr;
    CHAKRA_PTR sideEffectsAddr;
    CHAKRA_PTR arraySetElementFastPathVtableAddr;
    CHAKRA_PTR intArraySetElementFastPathVtableAddr;
    CHAKRA_PTR floatArraySetElementFastPathVtableAddr;
    CHAKRA_PTR numberAllocatorAddr;
    CHAKRA_PTR recyclerAddr;
    CHAKRA_PTR builtinFunctionsBaseAddr;
#ifdef ENABLE_SCRIPT_DEBUGGING
    CHAKRA_PTR debuggingFlagsAddr;
    CHAKRA_PTR debugStepTypeAddr;
    CHAKRA_PTR debugFrameAddressAddr;
    CHAKRA_PTR debugScriptIdWhenSetAddr;
#endif
} ScriptContextDataIDL;

typedef struct SmallSpanSequenceIDL
{
    int baseValue;
    unsigned int statementLength;
    IDL_DEF([size_is(statementLength)]) unsigned int * statementBuffer;
    X64_PAD4(1)
    unsigned int actualOffsetLength; // REVIEW: are lengths the same?
    IDL_DEF([size_is(actualOffsetLength)]) unsigned int * actualOffsetList;
} SmallSpanSequenceIDL;

typedef struct JITLoopHeaderIDL
{
    boolean isNested;
    boolean isInTry;
    IDL_PAD2(0)
    unsigned int interpretCount;
    unsigned int startOffset;
    unsigned int endOffset;
} JITLoopHeaderIDL;


typedef struct StatementMapIDL
{
    int sourceSpanBegin;
    int sourceSpanEnd;
    int byteCodeSpanBegin;
    int byteCodeSpanEnd;
    boolean isSubExpression;
    IDL_PAD1(1)
    IDL_PAD2(0)
} StatementMapIDL;

typedef struct WasmSignatureIDL
{
    int resultType;
    unsigned int id;
    unsigned short paramSize;
    unsigned short paramsCount;
    X64_PAD4(0)
    CHAKRA_PTR shortSig;
    IDL_DEF([size_is(paramsCount)]) int * params;
} WasmSignatureIDL;

typedef struct TypedSlotInfo
{
    boolean isValidType;
    IDL_PAD1(0)
    IDL_PAD2(1)
    unsigned int constCount;
    unsigned int varCount;
    unsigned int tmpCount;
    unsigned int byteOffset;
    unsigned int constSrcByteOffset;
} TypedSlotInfo;

typedef struct AsmJsDataIDL
{
    boolean usesHeapBuffer;
    IDL_PAD1(0)
    unsigned short argByteSize;
    unsigned short argCount;
    IDL_PAD2(1)
    int retType;
    int totalSizeInBytes;
    unsigned int wasmSignatureCount;
    X64_PAD4(2)
    TypedSlotInfo typedSlotInfos[5];
    CHAKRA_PTR wasmSignaturesBaseAddr;
    IDL_DEF([size_is(wasmSignatureCount)]) WasmSignatureIDL *  wasmSignatures;
    IDL_DEF([size_is(argCount)]) byte * argTypeArray;
} AsmJsDataIDL;

typedef struct FunctionJITRuntimeIDL
{
    unsigned int clonedCacheCount;
    X64_PAD4(0)
    IDL_DEF([size_is(clonedCacheCount)]) CHAKRA_PTR * clonedInlineCaches;
} FunctionJITRuntimeIDL;

typedef struct PropertyIdArrayIDL
{
    unsigned int count;
    byte extraSlotCount;
    boolean hadDuplicates;
    boolean has__proto__;
    boolean hasNonSimpleParams;
    IDL_DEF([size_is(count + extraSlotCount)]) int elements[IDL_DEF(*)];
} PropertyIdArrayIDL;


typedef struct JavascriptStringIDL
{
    IDL_DEF([size_is(m_charLength + 1)]) WCHAR* m_pszValue;
    unsigned int m_charLength;
} JavascriptStringIDL;

typedef IDL_DEF([switch_type(int)]) union RecyclableObjectContent
{
    // todo: add more interesting types
    IDL_DEF([case(4)]) //TypeIds_Number
    double value;
    IDL_DEF([case(7)]) //TypeIds_String
    JavascriptStringIDL str;
    IDL_DEF([default])
    ;
} RecyclableObjectContent;


typedef struct RecyclableObjectIDL
{
    CHAKRA_PTR vtbl; // todo: this can be saved
    int* typeId; // first field of Js::Type is typeId, so this should work
    IDL_DEF([switch_is(*typeId)]) RecyclableObjectContent x;
} RecyclableObjectIDL;

// to avoid rpc considering FunctionBodyDataIDL complex, move this to its own struct
typedef struct ConstTableContentIDL
{
    unsigned int count;
    X64_PAD4(0)
    IDL_DEF([size_is(count)]) RecyclableObjectIDL** content;
} ConstTableContentIDL;

// FunctionBody fields, read only in JIT, gathered in foreground
typedef struct FunctionBodyDataIDL
{
    byte flags;

    // TODO: compress booleans into flags
    boolean doBackendArgumentsOptimization;
    boolean isLibraryCode;
    boolean isAsmJsMode;
    boolean isWasmFunction;
    boolean hasImplicitArgIns;
    boolean isStrictMode;
    boolean isEval;
    boolean hasScopeObject;
    boolean hasCachedScopePropIds;
    boolean inlineCachesOnFunctionObject;
    boolean doInterruptProbe;
    boolean isGlobalFunc;
    boolean isInlineApplyDisabled;
    boolean doJITLoopBody;
    boolean disableInlineSpread;
    boolean hasNestedLoop;
    boolean hasNonBuiltInCallee;
    boolean isParamAndBodyScopeMerged;
    boolean hasFinally;
    boolean usesArgumentsObject;
    boolean doScopeObjectCreation;

    unsigned short envDepth;
    unsigned short inParamCount;
    unsigned short argUsedForBranch;
    unsigned short profiledCallSiteCount;
    IDL_PAD2(0)
    unsigned int funcNumber;
    unsigned int sourceContextId;
    unsigned int nestedCount;
    unsigned int scopeSlotArraySize;
    unsigned int paramScopeSlotArraySize;
    unsigned int attributes;
    unsigned int byteCodeCount;
    unsigned int byteCodeInLoopCount;
    unsigned int nonLoadByteCodeCount;
    unsigned int localFrameDisplayReg;
    unsigned int paramClosureReg;
    unsigned int localClosureReg;
    unsigned int envReg;
    unsigned int firstTmpReg;
    unsigned int firstInnerScopeReg;
    unsigned int varCount;
    unsigned int innerScopeCount;
    unsigned int thisRegisterForEventHandler;
    unsigned int funcExprScopeRegister;
    unsigned int loopCount;
    unsigned int recursiveCallSiteCount;
    unsigned int isInstInlineCacheCount; // TODO: only used in Assert
    unsigned int forInLoopDepth;
    unsigned int byteCodeLength;
    unsigned int constCount;
    unsigned int inlineCacheCount;
    unsigned int loopHeaderArrayLength;
    unsigned int referencedPropertyIdCount;
    unsigned int nameLength;
    unsigned int literalRegexCount;
    unsigned int auxDataCount;
    unsigned int auxContextDataCount;
    unsigned int functionSlotsInCachedScopeCount;

    unsigned int fullStatementMapCount;
    unsigned int propertyIdsForRegSlotsCount;
    X64_PAD4(1)
    IDL_DEF([size_is(propertyIdsForRegSlotsCount)]) int * propertyIdsForRegSlots;

    SmallSpanSequenceIDL * statementMap;

    IDL_DEF([size_is(fullStatementMapCount)]) StatementMapIDL * fullStatementMaps;

    IDL_DEF([size_is(byteCodeLength)]) byte * byteCodeBuffer;

    IDL_DEF([size_is(constCount)]) CHAKRA_PTR * constTable;
    ConstTableContentIDL * constTableContent;

    IDL_DEF([size_is(inlineCacheCount)]) int * cacheIdToPropertyIdMap;
    IDL_DEF([size_is(inlineCacheCount + isInstInlineCacheCount)]) CHAKRA_PTR * inlineCaches;

    IDL_DEF([size_is(loopHeaderArrayLength)]) JITLoopHeaderIDL * loopHeaders;

    IDL_DEF([size_is(referencedPropertyIdCount)]) int * referencedPropertyIdMap;

    IDL_DEF([size_is(nameLength)]) WCHAR * displayName;

    IDL_DEF([size_is(literalRegexCount)]) CHAKRA_PTR * literalRegexes;

    IDL_DEF([size_is(auxDataCount)]) byte * auxData;

    IDL_DEF([size_is(auxContextDataCount)]) byte * auxContextData;

    IDL_DEF([size_is(functionSlotsInCachedScopeCount)]) unsigned int * slotIdInCachedScopeToNestedIndexArray;

    ProfileDataIDL * profileData;

    AsmJsDataIDL * asmJsData;

    PropertyIdArrayIDL * formalsPropIdArray;

    CHAKRA_PTR loopHeaderArrayAddr;
    CHAKRA_PTR functionBodyAddr;
    CHAKRA_PTR scriptIdAddr;
    CHAKRA_PTR probeCountAddr;
    CHAKRA_PTR flagsAddr;
    CHAKRA_PTR regAllocStoreCountAddr;
    CHAKRA_PTR regAllocLoadCountAddr;
    CHAKRA_PTR callCountStatsAddr;
    CHAKRA_PTR nestedFuncArrayAddr;
    CHAKRA_PTR auxDataBufferAddr;
    CHAKRA_PTR objectLiteralTypesAddr;
    CHAKRA_PTR formalsPropIdArrayAddr;
    CHAKRA_PTR forInCacheArrayAddr;
} FunctionBodyDataIDL;

typedef struct FunctionJITTimeDataIDL
{
    boolean isAggressiveInliningEnabled;
    boolean isInlined;
    IDL_PAD2(0)
    unsigned int localFuncId;
    FunctionBodyDataIDL * bodyData; // TODO: oop jit, can these repeat, should we share?

    BVFixedIDL * inlineesBv;

    unsigned int sharedPropGuardCount;
    unsigned int globalObjTypeSpecFldInfoCount;
    IDL_DEF([size_is(sharedPropGuardCount)]) int * sharedPropertyGuards;

    IDL_DEF([size_is(globalObjTypeSpecFldInfoCount)]) ObjTypeSpecFldIDL ** globalObjTypeSpecFldInfoArray;

    unsigned int inlineeCount;
    unsigned int ldFldInlineeCount;
    IDL_DEF([size_is(inlineeCount)]) struct FunctionJITTimeDataIDL ** inlinees;
    IDL_DEF([size_is(inlineeCount)]) boolean * inlineesRecursionFlags;

    IDL_DEF([size_is(ldFldInlineeCount)]) struct FunctionJITTimeDataIDL ** ldFldInlinees;

    X64_PAD4(1)
    unsigned int objTypeSpecFldInfoCount;
    IDL_DEF([size_is(objTypeSpecFldInfoCount)]) ObjTypeSpecFldIDL ** objTypeSpecFldInfoArray;

    FunctionJITRuntimeIDL * profiledRuntimeData;

    struct FunctionJITTimeDataIDL * next;

    CHAKRA_PTR functionInfoAddr;
    CHAKRA_PTR callsCountAddress;
    CHAKRA_PTR weakFuncRef;
} FunctionJITTimeDataIDL;

typedef struct XProcNumberPageSegment
{
    struct XProcNumberPageSegment* nextSegment;

    unsigned int committedEnd;
    unsigned int blockIntegratedSize;
    CHAKRA_PTR pageAddress;
    CHAKRA_PTR allocStartAddress;
    CHAKRA_PTR allocEndAddress;
    CHAKRA_PTR pageSegment;
} XProcNumberPageSegment;

typedef struct PolymorphicInlineCacheIDL
{
    IDL_Field(unsigned short) size;
    IDL_PAD2(0)
    X64_PAD4(1)
    IDL_Field(CHAKRA_WB_PTR) addr;
    IDL_Field(CHAKRA_PTR) inlineCachesAddr;
} PolymorphicInlineCacheIDL;

typedef struct PolymorphicInlineCacheInfoIDL
{
    IDL_Field(unsigned int) polymorphicInlineCacheCount;
    IDL_Field(unsigned int) bogus1;
    IDL_DEF([size_is(polymorphicInlineCacheCount)]) IDL_Field(byte *) polymorphicCacheUtilizationArray;
    IDL_DEF([size_is(polymorphicInlineCacheCount)]) IDL_Field(PolymorphicInlineCacheIDL *) polymorphicInlineCaches;

    IDL_Field(CHAKRA_WB_PTR) functionBodyAddr;
} PolymorphicInlineCacheInfoIDL;

// CodeGenWorkItem fields, read only in JIT
typedef struct CodeGenWorkItemIDL
{
    boolean hasSharedPropGuards;
    boolean isJitInDebugMode;  // Whether JIT is in debug mode for this work item.
    byte type;
    char jitMode;

    unsigned short profiledIterations;
    IDL_PAD2(0)
    unsigned int loopNumber;
    unsigned int inlineeInfoCount;
    unsigned int symIdToValueTypeMapCount;
    X64_PAD4(1)
    XProcNumberPageSegment * xProcNumberPageSegment;

    PolymorphicInlineCacheInfoIDL * selfInfo;

    IDL_DEF([size_is(inlineeInfoCount)]) PolymorphicInlineCacheInfoIDL * inlineeInfo;

    // TODO: OOP JIT, move loop body data to separate struct
    IDL_DEF([size_is(symIdToValueTypeMapCount)]) unsigned short * symIdToValueTypeMap;

    FunctionJITTimeDataIDL * jitData;
    CHAKRA_PTR jittedLoopIterationsSinceLastBailoutAddr;
    CHAKRA_PTR functionBodyAddr;
    CHAKRA_PTR globalThisAddr;
    CHAKRA_PTR nativeDataAddr;
    __int64 startTime;
} CodeGenWorkItemIDL;

typedef struct NativeDataFixupEntry
{
    struct NativeDataFixupEntry* next;
    unsigned int addrOffset;
    unsigned int targetTotalOffset;
} NativeDataFixupEntry;

typedef struct NativeDataFixupRecord
{
    unsigned int index;
    unsigned int length;
    unsigned int startOffset;
    X64_PAD4(0)
    struct NativeDataFixupEntry* updateList;
} NativeDataFixupRecord;

typedef struct NativeDataFixupTable
{
    unsigned int count;
    X64_PAD4(0)
    IDL_DEF([size_is(count)]) NativeDataFixupRecord fixupRecords[IDL_DEF(*)];
} NativeDataFixupTable;


typedef struct TypeEquivalenceRecordIDL
{
    unsigned int propertyCount;
    unsigned int propertyOffset;
} TypeEquivalenceRecord;

typedef struct EquivlentTypeCacheIDL
{
    CHAKRA_PTR types[EQUIVALENT_TYPE_CACHE_SIZE];
    CHAKRA_PTR guardOffset;
    struct TypeEquivalenceRecordIDL record;
    unsigned int nextEvictionVictim;
    boolean isLoadedFromProto;
    boolean hasFixedValue;
    IDL_PAD2(0)
} EquivlentTypeCacheIDL;

typedef struct EquivalentTypeGuardIDL
{
    EquivlentTypeCacheIDL cache;
    unsigned int offset;

} EquivalentTypeGuardIDL;

typedef struct EquivalentTypeGuardOffsets
{
    unsigned int count;
    X64_PAD4(0)
    IDL_DEF([size_is(count)]) EquivalentTypeGuardIDL guards[IDL_DEF(*)];

} EquivalentTypeGuardOffsets;

typedef struct TypeGuardTransferEntryIDL
{
    unsigned int propId;
    unsigned int guardsCount;
    struct TypeGuardTransferEntryIDL* next;
    IDL_DEF([size_is(guardsCount)]) int guardOffsets[IDL_DEF(*)];
} TypeGuardTransferEntryIDL;

typedef struct  CtorCacheTransferEntryIDL
{
    unsigned int propId;
    unsigned int cacheCount;
    IDL_DEF([size_is(cacheCount)]) CHAKRA_PTR caches[IDL_DEF(*)];
}  CtorCacheTransferEntryIDL;

typedef struct NativeDataBuffer
{
    unsigned int len;
    // pad so that buffer is always 8 byte aligned
    IDL_PAD4(0)
    IDL_DEF([size_is(len)]) byte data[IDL_DEF(*)];
} NativeDataBuffer;

// Fields that JIT modifies
typedef struct JITOutputIDL
{
    boolean disableArrayCheckHoist;
    boolean disableAggressiveIntTypeSpec;
    boolean disableInlineApply;
    boolean disableInlineSpread;
    boolean disableStackArgOpt;
    boolean disableSwitchOpt;
    boolean disableTrackCompoundedIntOverflow;
    boolean isInPrereservedRegion;

    boolean hasBailoutInstr;

    boolean hasJittedStackClosure;

    unsigned short pdataCount;
    unsigned short xdataSize;

    unsigned short argUsedForBranch;

    int localVarSlotsOffset; // FunctionEntryPointInfo only
    int localVarChangedOffset; // FunctionEntryPointInfo only
    unsigned int frameHeight;


    unsigned int codeSize;
    unsigned int throwMapOffset;
    unsigned int throwMapCount;
    unsigned int inlineeFrameOffsetArrayOffset;
    unsigned int inlineeFrameOffsetArrayCount;

    unsigned int propertyGuardCount;
    unsigned int ctorCachesCount;

#if defined(_M_X64)
    CHAKRA_PTR xdataAddr;
#elif defined(_M_ARM) || defined(_M_ARM64)
    unsigned int xdataOffset;
#else
    X86_PAD4(0)
#endif
    CHAKRA_PTR codeAddress;
    CHAKRA_PTR thunkAddress;
    TypeGuardTransferEntryIDL* typeGuardEntries;

    IDL_DEF([size_is(ctorCachesCount)]) CtorCacheTransferEntryIDL ** ctorCacheEntries;
    PinnedTypeRefsIDL* pinnedTypeRefs;

    NativeDataFixupTable* nativeDataFixupTable;
    NativeDataBuffer* buffer;
    EquivalentTypeGuardOffsets* equivalentTypeGuardOffsets;
    XProcNumberPageSegment* numberPageSegments;
    __int64 startTime;
} JITOutputIDL;

typedef struct InterpreterThunkInputIDL
{
    boolean asmJsThunk;
} InterpreterThunkInputIDL;

typedef struct InterpreterThunkOutputIDL
{
    unsigned int thunkCount;
    X64_PAD4(0)
    CHAKRA_PTR mappedBaseAddr;
    CHAKRA_PTR pdataTableStart;
    CHAKRA_PTR epilogEndAddr;
} InterpreterThunkOutputIDL;

#endif //__JITTypes_h__
