//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef __midl
import "wtypes.idl";
#endif

#if defined(_M_IX86) || defined(_M_ARM)
#define CHAKRA_PTR int
#define BV_SHIFT 5
#elif defined(_M_X64) || defined(_M_ARM64)
#define CHAKRA_PTR __int64
#define BV_SHIFT 6
#endif

#ifdef __midl
#define IDL_DEF(def) def
#else
#define IDL_DEF(def)
#endif

#if defined(_M_X64) && defined(__midl)
#define IDL_PAD1(num) byte struct_pad_##num;
#define IDL_PAD2(num) short struct_pad_##num;
#define IDL_PAD4(num) int struct_pad_##num;
#else
#define IDL_PAD1(num)
#define IDL_PAD2(num)
#define IDL_PAD4(num)
#endif

#ifndef __midl
#ifndef _MSC_VER
typedef unsigned char boolean;
#endif
#endif

// TODO: OOP JIT, how do we make this better?
const int VTABLE_COUNT = 47;
const int EQUIVALENT_TYPE_CACHE_SIZE_IDL = 8;

typedef struct TypeHandlerIDL
{
    boolean isObjectHeaderInlinedTypeHandler;
    boolean isLocked;

    unsigned short inlineSlotCapacity;
    unsigned short offsetOfInlineSlots;
    IDL_PAD2(0)
    IDL_PAD4(1)
    int slotCapacity;
} TypeHandlerIDL;

typedef struct TypeIDL
{
    unsigned char flags;
    boolean isShared;
    IDL_PAD2(0)
    int typeId;

    CHAKRA_PTR libAddr;
    CHAKRA_PTR protoAddr;
    CHAKRA_PTR entrypointAddr;
    CHAKRA_PTR propertyCacheAddr;
    CHAKRA_PTR addr;

    TypeHandlerIDL handler;
} TypeIDL;

typedef struct EquivalentTypeSetIDL
{
    boolean sortedAndDuplicatesRemoved;
    IDL_PAD1(0)
    unsigned short count;
    IDL_PAD4(1)
    IDL_DEF([size_is(count)]) TypeIDL ** types;
} EquivalentTypeSetIDL;

typedef struct FixedFieldIDL
{
    boolean nextHasSameFixedField;
    boolean isClassCtor;
    unsigned short valueType;
    unsigned int localFuncId;
    TypeIDL * type;
    CHAKRA_PTR fieldValue;
    CHAKRA_PTR funcInfoAddr;
    CHAKRA_PTR environmentAddr;
} FixedFieldIDL;

typedef struct JITTimeConstructorCacheIDL
{
    boolean skipNewScObject;
    boolean ctorHasNoExplicitReturnValue;
    boolean typeIsFinal;
    boolean isUsed;

    short inlineSlotCount;

    IDL_PAD2(0)
    int slotCount;

    IDL_PAD4(1)
    TypeIDL type;

    CHAKRA_PTR runtimeCacheAddr;
    CHAKRA_PTR runtimeCacheGuardAddr;
    CHAKRA_PTR guardedPropOps;
} JITTimeConstructorCacheIDL;

typedef struct ObjTypeSpecFldIDL
{
    boolean inUse;
    IDL_PAD1(0)
    unsigned short flags;
    unsigned short slotIndex;
    unsigned short fixedFieldCount;
    unsigned short fixedFieldInfoArraySize; // 1 (when fixedFieldCount is 0) or fixedFieldCount
    IDL_PAD2(1)
    int propertyId;
    int typeId;
    unsigned int id;
    CHAKRA_PTR protoObjectAddr;
    CHAKRA_PTR propertyGuardValueAddr;
    EquivalentTypeSetIDL * typeSet;
    TypeIDL * initialType;
    JITTimeConstructorCacheIDL * ctorCache;
    IDL_DEF([size_is(fixedFieldInfoArraySize)]) FixedFieldIDL * fixedFieldInfoArray;
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
    IDL_PAD4(0)
    IDL_DEF([size_is(((len - 1) >> BV_SHIFT) + 1)]) BVUnitIDL data[IDL_DEF(*)];
} BVFixedIDL;

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
} LdElemIDL;

typedef struct StElemIDL
{
    unsigned short arrayType;
    byte bits;
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

    IDL_PAD2(1)
    IDL_PAD4(2)
    CHAKRA_PTR processHandle;
    CHAKRA_PTR chakraBaseAddress;
    CHAKRA_PTR crtBaseAddress;
    CHAKRA_PTR threadStackLimitAddr;
    CHAKRA_PTR scriptStackLimit;
    CHAKRA_PTR bailOutRegisterSaveSpaceAddr;
    CHAKRA_PTR disableImplicitFlagsAddr;
    CHAKRA_PTR implicitCallFlagsAddr;
    CHAKRA_PTR debuggingFlagsAddr;
    CHAKRA_PTR debugStepTypeAddr;
    CHAKRA_PTR debugFrameAddressAddr;
    CHAKRA_PTR debugScriptIdWhenSetAddr;
    CHAKRA_PTR stringReplaceNameAddr;
    CHAKRA_PTR stringMatchNameAddr;
    CHAKRA_PTR simdTempAreaBaseAddr;
} ThreadContextDataIDL;

typedef struct ScriptContextDataIDL
{
    boolean isRecyclerVerifyEnabled;
    boolean recyclerAllowNativeCodeBumpAllocation;
    boolean isSIMDEnabled;
    IDL_PAD1(0)
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
    CHAKRA_PTR globalObjectThisAddr;
    CHAKRA_PTR sideEffectsAddr;
    CHAKRA_PTR arraySetElementFastPathVtableAddr;
    CHAKRA_PTR intArraySetElementFastPathVtableAddr;
    CHAKRA_PTR floatArraySetElementFastPathVtableAddr;
    CHAKRA_PTR numberAllocatorAddr;
    CHAKRA_PTR recyclerAddr;
    CHAKRA_PTR builtinFunctionsBaseAddr;
} ScriptContextDataIDL;

typedef struct SmallSpanSequenceIDL
{
    int baseValue;
    unsigned int statementLength;
    IDL_DEF([size_is(statementLength)]) unsigned int * statementBuffer;
    IDL_PAD4(1)
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

typedef struct AsmJsDataIDL
{
    boolean isHeapBufferConst;
    boolean usesHeapBuffer;
    unsigned short argByteSize;
    unsigned short argCount;
    IDL_PAD2(0)
    int retType;
    int intConstCount;
    int doubleConstCount;
    int floatConstCount;
    int simdConstCount;
    int intTmpCount;
    int doubleTmpCount;
    int floatTmpCount;
    int simdTmpCount;
    int intVarCount;
    int doubleVarCount;
    int floatVarCount;
    int simdVarCount;
    int intByteOffset;
    int doubleByteOffset;
    int floatByteOffset;
    int simdByteOffset;
    int totalSizeInBytes;
    IDL_DEF([size_is(argCount)]) byte * argTypeArray;
} AsmJsDataIDL;

typedef struct PropertyRecordIDL
{
    CHAKRA_PTR vTable;
    int pid;
    unsigned int hash;
    boolean isNumeric;
    boolean isBound;
    boolean isSymbol;
    IDL_PAD1(0)
    unsigned int byteCount;
    IDL_DEF([size_is(byteCount + sizeof(wchar_t) + (isNumeric ? sizeof(unsigned int) : 0))]) byte buffer[IDL_DEF(*)];
} PropertyRecordIDL;

typedef struct FunctionJITRuntimeIDL
{
    unsigned int clonedCacheCount;
    IDL_PAD4(0)
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
    IDL_DEF([size_is(m_charLength + 1)]) wchar_t* m_pszValue;
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
    IDL_PAD4(0)
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

    unsigned short envDepth;
    unsigned short inParamCount;
    unsigned short profiledIterations;
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

    unsigned int byteCodeLength;
    unsigned int constCount;
    unsigned int inlineCacheCount;
    unsigned int loopHeaderArrayLength;
    unsigned int referencedPropertyIdCount;
    unsigned int nameLength;
    unsigned int literalRegexCount;
    unsigned int auxDataCount;
    unsigned int auxContextDataCount;

    unsigned int fullStatementMapCount;
    unsigned int propertyIdsForRegSlotsCount;

    IDL_PAD4(1)

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

    IDL_DEF([size_is(nameLength)]) wchar_t * displayName;

    IDL_DEF([size_is(literalRegexCount)]) CHAKRA_PTR * literalRegexes;

    IDL_DEF([size_is(auxDataCount)]) byte * auxData;

    IDL_DEF([size_is(auxContextDataCount)]) byte * auxContextData;

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

    IDL_DEF([size_is(globalObjTypeSpecFldInfoCount)]) ObjTypeSpecFldIDL * globalObjTypeSpecFldInfoArray;

    unsigned int inlineeCount;
    unsigned int ldFldInlineeCount;
    IDL_DEF([size_is(inlineeCount)]) struct FunctionJITTimeDataIDL ** inlinees;
    IDL_DEF([size_is(inlineeCount)]) boolean * inlineesRecursionFlags;

    IDL_DEF([size_is(ldFldInlineeCount)]) struct FunctionJITTimeDataIDL ** ldFldInlinees;

    IDL_PAD4(1)
    unsigned int objTypeSpecFldInfoCount;
    IDL_DEF([size_is(objTypeSpecFldInfoCount)]) ObjTypeSpecFldIDL * objTypeSpecFldInfoArray;

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
    CHAKRA_PTR chunkAllocator;
} XProcNumberPageSegment;

typedef struct PolymorphicInlineCacheIDL
{
    unsigned short size;
    IDL_PAD2(0)
    IDL_PAD4(1)
    CHAKRA_PTR addr;
    CHAKRA_PTR inlineCachesAddr;
} PolymorphicInlineCacheIDL;

typedef struct PolymorphicInlineCacheInfoIDL
{
    unsigned int polymorphicInlineCacheCount;
    unsigned int bogus1;
    IDL_DEF([size_is(polymorphicInlineCacheCount)]) byte * polymorphicCacheUtilizationArray;
    IDL_DEF([size_is(polymorphicInlineCacheCount)]) PolymorphicInlineCacheIDL * polymorphicInlineCaches;

    CHAKRA_PTR functionBodyAddr;
} PolymorphicInlineCacheInfoIDL;

// CodeGenWorkItem fields, read only in JIT
typedef struct CodeGenWorkItemIDL
{
    boolean hasSharedPropGuards;
    boolean isJitInDebugMode;  // Whether JIT is in debug mode for this work item.
    byte type;
    char jitMode;

    unsigned int loopNumber;
    unsigned int inlineeInfoCount;
    unsigned int symIdToValueTypeMapCount;
    XProcNumberPageSegment * xProcNumberPageSegment;

    PolymorphicInlineCacheInfoIDL * selfInfo;

    IDL_DEF([size_is(inlineeInfoCount)]) PolymorphicInlineCacheInfoIDL * inlineeInfo;

    // TODO: OOP JIT, move loop body data to separate struct
    IDL_DEF([size_is(symIdToValueTypeMapCount)]) unsigned short * symIdToValueTypeMap;

    FunctionJITTimeDataIDL * jitData;
    CHAKRA_PTR jittedLoopIterationsSinceLastBailoutAddr;
    CHAKRA_PTR functionBodyAddr;
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
    IDL_PAD4(0)
    struct NativeDataFixupEntry* updateList;
} NativeDataFixupRecord;

typedef struct NativeDataFixupTable
{
    unsigned int count;
    IDL_PAD4(0)
    IDL_DEF([size_is(count)]) NativeDataFixupRecord fixupRecords[IDL_DEF(*)];
} NativeDataFixupTable;


typedef struct TypeEquivalenceRecordIDL
{
    unsigned int propertyCount;
    unsigned int propertyOffset;
} TypeEquivalenceRecord;

typedef struct EquivlentTypeCacheIDL
{
    CHAKRA_PTR types[EQUIVALENT_TYPE_CACHE_SIZE_IDL];
    CHAKRA_PTR guardOffset;
    struct TypeEquivalenceRecordIDL record;
    unsigned int nextEvictionVictim;
    boolean isLoadedFromProto;
    boolean hasFixedValue;
} EquivlentTypeCacheIDL;

typedef struct EquivalentTypeGuardIDL
{
    EquivlentTypeCacheIDL cache;
    unsigned int offset;

} EquivalentTypeGuardIDL;

typedef struct EquivalentTypeGuardOffsets
{
    unsigned int count;
    IDL_PAD4(0)
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
    unsigned int unused;
    IDL_DEF([size_is(len)]) byte data[IDL_DEF(*)];
} NativeDataBuffer;

// Fields that JIT modifies
typedef struct JITOutputIDL
{
    boolean disableAggressiveIntTypeSpec;
    boolean disableInlineApply;
    boolean disableInlineSpread;
    boolean disableStackArgOpt;
    boolean disableSwitchOpt;
    boolean disableTrackCompoundedIntOverflow;
    boolean isInPrereservedRegion;

    boolean hasBailoutInstr;

    boolean hasJittedStackClosure;

    IDL_PAD1(0)

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

    CHAKRA_PTR codeAddress;
    CHAKRA_PTR xdataAddr;
    TypeGuardTransferEntryIDL* typeGuardEntries;

    IDL_DEF([size_is(ctorCachesCount)]) CtorCacheTransferEntryIDL ** ctorCacheEntries;
    PinnedTypeRefsIDL* pinnedTypeRefs;

    NativeDataFixupTable* nativeDataFixupTable;
    NativeDataBuffer* buffer;
    EquivalentTypeGuardOffsets* equivalentTypeGuardOffsets;
    XProcNumberPageSegment* numberPageSegments;

    __int64 startTime;
} JITOutputIDL;
