//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define InlineSlotCountIncrement (HeapConstants::ObjectGranularity / sizeof(Var))

#define MaxPreInitializedObjectTypeInlineSlotCount 16
#define MaxPreInitializedObjectHeaderInlinedTypeInlineSlotCount \
    (Js::DynamicTypeHandler::GetObjectHeaderInlinableSlotCapacity() + MaxPreInitializedObjectTypeInlineSlotCount)
#define PreInitializedObjectTypeCount ((MaxPreInitializedObjectTypeInlineSlotCount / InlineSlotCountIncrement) + 1)
CompileAssert(MaxPreInitializedObjectTypeInlineSlotCount <= USHRT_MAX);

#include "StringCache.h"
#include "Library/JavascriptGenerator.h"

class ScriptSite;
class ActiveScriptExternalLibrary;
class ProjectionExternalLibrary;
class EditAndContinue;
class ChakraHostScriptContext;

#ifdef ENABLE_PROJECTION
namespace Projection
{
    class ProjectionContext;
    class WinRTPromiseEngineInterfaceExtensionObject;
}
#endif

namespace Js
{
    class RefCountedBuffer;

    static const unsigned int EvalMRUSize = 15;
    typedef JsUtil::BaseDictionary<DWORD_PTR, SourceContextInfo *, Recycler, PowerOf2SizePolicy> SourceContextInfoMap;
    typedef JsUtil::BaseDictionary<uint, SourceContextInfo *, Recycler, PowerOf2SizePolicy> DynamicSourceContextInfoMap;

    typedef JsUtil::BaseDictionary<EvalMapString, ScriptFunction*, RecyclerNonLeafAllocator, PrimeSizePolicy> SecondLevelEvalCache;
    typedef TwoLevelHashRecord<FastEvalMapString, ScriptFunction*, SecondLevelEvalCache, EvalMapString> EvalMapRecord;
    typedef JsUtil::Cache<FastEvalMapString, EvalMapRecord*, RecyclerNonLeafAllocator, PrimeSizePolicy, JsUtil::MRURetentionPolicy<FastEvalMapString, EvalMRUSize>, FastEvalMapStringComparer> EvalCacheTopLevelDictionary;
    typedef JsUtil::Cache<EvalMapString, FunctionInfo*, RecyclerNonLeafAllocator, PrimeSizePolicy, JsUtil::MRURetentionPolicy<EvalMapString, EvalMRUSize>> NewFunctionCache;
    typedef JsUtil::BaseDictionary<ParseableFunctionInfo*, ParseableFunctionInfo*, Recycler, PrimeSizePolicy, RecyclerPointerComparer> ParseableFunctionInfoMap;
    // This is the dictionary used by script context to cache the eval.
    typedef TwoLevelHashDictionary<FastEvalMapString, ScriptFunction*, EvalMapRecord, EvalCacheTopLevelDictionary, EvalMapString> EvalCacheDictionary;

    typedef JsUtil::BaseDictionary<JavascriptMethod, JavascriptFunction*, Recycler, PrimeSizePolicy> BuiltInLibraryFunctionMap;
    typedef JsUtil::BaseDictionary<uint, JavascriptString *, Recycler> StringMap;

    // valid if object!= NULL
    struct EnumeratedObjectCache
    {
        static const int kMaxCachedPropStrings = 16;
        Field(DynamicObject*) object;
        Field(DynamicType*) type;
        Field(PropertyString*) propertyStrings[kMaxCachedPropStrings];
        Field(int) validPropStrings;
    };

    struct PropertyStringMap
    {
        Field(PropertyString*) strLen2[80];

        inline static uint PStrMapIndex(char16 ch)
        {
            Assert(ch >= '0' && ch <= 'z');
            return ch - '0';
        }
    };

    struct Cache
    {
        static const uint AssignCacheSize = 16;
        static const uint StringifyCacheSize = 16;

        Field(PropertyStringMap*) propertyStrings[80];
        Field(JavascriptString *) lastNumberToStringRadix10String;
        Field(EnumeratedObjectCache) enumObjCache;
        Field(JavascriptString *) lastUtcTimeFromStrString;
        Field(EvalCacheDictionary*) evalCacheDictionary;
        Field(EvalCacheDictionary*) indirectEvalCacheDictionary;
        Field(NewFunctionCache*) newFunctionCache;
        Field(StringMap *) integerStringMap;
        Field(RegexPatternMruMap *) dynamicRegexMap;
        Field(SourceContextInfoMap*) sourceContextInfoMap;   // maps host provided context cookie to the URL of the script buffer passed.
        Field(DynamicSourceContextInfoMap*) dynamicSourceContextInfoMap;
        Field(SourceContextInfo*) noContextSourceContextInfo;
        Field(SRCINFO*) noContextGlobalSourceInfo;
        Field(Field(SRCINFO const *)*) moduleSrcInfo;
        Field(BuiltInLibraryFunctionMap*) builtInLibraryFunctions;
        Field(ScriptContextPolymorphicInlineCache*) toStringTagCache;
        Field(ScriptContextPolymorphicInlineCache*) toJSONCache;
        Field(EnumeratorCache*) assignCache;
        Field(EnumeratorCache*) stringifyCache;
#if ENABLE_PROFILE_INFO
#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)
        Field(DynamicProfileInfoList*) profileInfoList;
#endif
#endif
        Cache() : toStringTagCache(nullptr), toJSONCache(nullptr), assignCache(nullptr), stringifyCache(nullptr) { }
    };

    class MissingPropertyTypeHandler;
    class SourceTextModuleRecord;
    class ArrayBufferBase;
    class SharedContents;
    typedef RecyclerFastAllocator<JavascriptNumber, LeafBit> RecyclerJavascriptNumberAllocator;
    typedef JsUtil::List<Var, Recycler> ListForListIterator;

    class UndeclaredBlockVariable : public RecyclableObject
    {
        friend class JavascriptLibrary;
        UndeclaredBlockVariable(Type* type) : RecyclableObject(type) { }
    };

    class SourceTextModuleRecord;
    typedef JsUtil::List<SourceTextModuleRecord*> ModuleRecordList;

#if ENABLE_COPYONACCESS_ARRAY
    struct CacheForCopyOnAccessArraySegments
    {
        static const uint32 MAX_SIZE = 31;
        Field(SparseArraySegment<int32> *) cache[MAX_SIZE];
        Field(uint32) count;

        uint32 AddSegment(SparseArraySegment<int32> *segment)
        {
            cache[count++] = segment;
            return count;
        }

        SparseArraySegment<int32> *GetSegmentByIndex(byte index)
        {
            Assert(index <= MAX_SIZE);
            return cache[index - 1];
        }

        bool IsNotOverHardLimit()
        {
            return count < MAX_SIZE;
        }

        bool IsNotFull()
        {
            return count < (uint32) CONFIG_FLAG(CopyOnAccessArraySegmentCacheSize);
        }

        bool IsValidIndex(uint32 index)
        {
            return count && index && index <= count;
        }

#if ENABLE_DEBUG_CONFIG_OPTIONS
        uint32 GetCount()
        {
            return count;
        }
#endif
    };
#endif

    class JavascriptLibrary : public JavascriptLibraryBase
    {
        friend class EditAndContinue;
        friend class ScriptSite;
        friend class GlobalObject;
        friend class ScriptContext;
        friend class EngineInterfaceObject;
        friend class ExternalLibraryBase;
        friend class ActiveScriptExternalLibrary;
        friend class IntlEngineInterfaceExtensionObject;
#ifdef ENABLE_JS_BUILTINS
        friend class JsBuiltInEngineInterfaceExtensionObject;
#endif
        friend class ChakraHostScriptContext;
#ifdef ENABLE_PROJECTION
        friend class ProjectionExternalLibrary;
        friend class Projection::WinRTPromiseEngineInterfaceExtensionObject;
        friend class Projection::ProjectionContext;
#endif
        static const char16* domBuiltinPropertyNames[];

    public:
#if ENABLE_COPYONACCESS_ARRAY
        Field(CacheForCopyOnAccessArraySegments *) cacheForCopyOnAccessArraySegments;
#endif

        static DWORD GetScriptContextOffset() { return offsetof(JavascriptLibrary, scriptContext); }
        static DWORD GetUndeclBlockVarOffset() { return offsetof(JavascriptLibrary, undeclBlockVarSentinel); }
        static DWORD GetEmptyStringOffset() { return offsetof(JavascriptLibrary, emptyString); }
        static DWORD GetUndefinedValueOffset() { return offsetof(JavascriptLibrary, undefinedValue); }
        static DWORD GetNullValueOffset() { return offsetof(JavascriptLibrary, nullValue); }
        static DWORD GetBooleanTrueOffset() { return offsetof(JavascriptLibrary, booleanTrue); }
        static DWORD GetBooleanFalseOffset() { return offsetof(JavascriptLibrary, booleanFalse); }
        static DWORD GetNegativeZeroOffset() { return offsetof(JavascriptLibrary, negativeZero); }
        static DWORD GetNumberTypeStaticOffset() { return offsetof(JavascriptLibrary, numberTypeStatic); }
        static DWORD GetBigIntTypeStaticOffset() { return offsetof(JavascriptLibrary, bigintTypeStatic); }
        static DWORD GetObjectTypesOffset() { return offsetof(JavascriptLibrary, objectTypes); }
        static DWORD GetObjectHeaderInlinedTypesOffset() { return offsetof(JavascriptLibrary, objectHeaderInlinedTypes); }
        static DWORD GetRegexTypeOffset() { return offsetof(JavascriptLibrary, regexType); }
        static DWORD GetArrayConstructorOffset() { return offsetof(JavascriptLibrary, arrayConstructor); }
        static DWORD GetPositiveInfinityOffset() { return offsetof(JavascriptLibrary, positiveInfinite); }
        static DWORD GetNaNOffset() { return offsetof(JavascriptLibrary, nan); }
        static DWORD GetNativeIntArrayTypeOffset() { return offsetof(JavascriptLibrary, nativeIntArrayType); }
#if ENABLE_COPYONACCESS_ARRAY
        static DWORD GetCopyOnAccessNativeIntArrayTypeOffset() { return offsetof(JavascriptLibrary, copyOnAccessNativeIntArrayType); }
#endif
        static DWORD GetNativeFloatArrayTypeOffset() { return offsetof(JavascriptLibrary, nativeFloatArrayType); }
        static DWORD GetVTableAddressesOffset() { return offsetof(JavascriptLibrary, vtableAddresses); }
        static DWORD GetConstructorCacheDefaultInstanceOffset() { return offsetof(JavascriptLibrary, constructorCacheDefaultInstance); }
        static DWORD GetAbsDoubleCstOffset() { return offsetof(JavascriptLibrary, absDoubleCst); }
        static DWORD GetUintConvertConstOffset() { return offsetof(JavascriptLibrary, uintConvertConst); }
        static DWORD GetBuiltinFunctionsOffset() { return offsetof(JavascriptLibrary, builtinFunctions); }
        static DWORD GetCharStringCacheOffset() { return offsetof(JavascriptLibrary, charStringCache); }
        static DWORD GetCharStringCacheAOffset() { return GetCharStringCacheOffset() + CharStringCache::GetCharStringCacheAOffset(); }
        PolymorphicInlineCache *GetToStringTagCache() const { return cache.toStringTagCache; }
        const  JavascriptLibraryBase* GetLibraryBase() const { return static_cast<const JavascriptLibraryBase*>(this); }
        void SetGlobalObject(GlobalObject* globalObject) {this->globalObject = globalObject; }
        static DWORD GetRandSeed0Offset() { return offsetof(JavascriptLibrary, randSeed0); }
        static DWORD GetRandSeed1Offset() { return offsetof(JavascriptLibrary, randSeed1); }
        static DWORD GetTypeDisplayStringsOffset() { return offsetof(JavascriptLibrary, typeDisplayStrings); }
        typedef bool (CALLBACK *PromiseContinuationCallback)(Var task, void *callbackState);

        Var GetUndeclBlockVar() const { return undeclBlockVarSentinel; }
        bool IsUndeclBlockVar(Var var) const { return var == undeclBlockVarSentinel; }

        static bool IsTypedArrayConstructor(Var constructor, ScriptContext* scriptContext);

    private:
        FieldNoBarrier(Recycler *) recycler;
        Field(ExternalLibraryBase*) externalLibraryList;

        Field(UndeclaredBlockVariable*) undeclBlockVarSentinel;

        Field(DynamicType *) generatorConstructorPrototypeObjectType;
        Field(DynamicType *) constructorPrototypeObjectType;
        Field(DynamicType *) heapArgumentsType;
        Field(DynamicType *) strictHeapArgumentsType;
        Field(DynamicType *) activationObjectType;
        Field(DynamicType *) arrayType;
        Field(DynamicType *) nativeIntArrayType;
#if ENABLE_COPYONACCESS_ARRAY
        Field(DynamicType *) copyOnAccessNativeIntArrayType;
#endif
        Field(DynamicType *) nativeFloatArrayType;
        Field(DynamicType *) arrayBufferType;
        Field(DynamicType *) sharedArrayBufferType;
        Field(DynamicType *) dataViewType;
        Field(DynamicType *) int8ArrayType;
        Field(DynamicType *) uint8ArrayType;
        Field(DynamicType *) uint8ClampedArrayType;
        Field(DynamicType *) int16ArrayType;
        Field(DynamicType *) uint16ArrayType;
        Field(DynamicType *) int32ArrayType;
        Field(DynamicType *) uint32ArrayType;
        Field(DynamicType *) float32ArrayType;
        Field(DynamicType *) float64ArrayType;
        Field(DynamicType *) int64ArrayType;
        Field(DynamicType *) uint64ArrayType;
        Field(DynamicType *) boolArrayType;
        Field(DynamicType *) charArrayType;
        Field(StaticType *) booleanTypeStatic;
        Field(DynamicType *) booleanTypeDynamic;
        Field(DynamicType *) bigintTypeDynamic;
        Field(StaticType *) bigintTypeStatic;
        Field(DynamicType *) dateType;
        Field(StaticType *) variantDateType;
        Field(DynamicType *) symbolTypeDynamic;
        Field(StaticType *) symbolTypeStatic;
        Field(DynamicType *) iteratorResultType;
        Field(DynamicType *) arrayIteratorType;
        Field(DynamicType *) mapIteratorType;
        Field(DynamicType *) setIteratorType;
        Field(DynamicType *) stringIteratorType;
        Field(DynamicType *) promiseType;
        Field(DynamicType *) listIteratorType;

        Field(JavascriptFunction*) builtinFunctions[BuiltinFunction::Count];

        Field(INT_PTR) vtableAddresses[VTableValue::Count];
        Field(JavascriptString*) typeDisplayStrings[TypeIds_Limit];
        Field(ConstructorCache *) constructorCacheDefaultInstance;
        __declspec(align(16)) Field(const BYTE *) absDoubleCst;
        Field(double const *) uintConvertConst;

        // Function Types
        Field(DynamicTypeHandler *) anonymousFunctionTypeHandler;
        Field(DynamicTypeHandler *) anonymousFunctionWithPrototypeTypeHandler;
        Field(DynamicTypeHandler *) functionTypeHandler;
        Field(DynamicTypeHandler *) functionTypeHandlerWithLength;
        Field(DynamicTypeHandler *) functionWithPrototypeAndLengthTypeHandler;
        Field(DynamicTypeHandler *) functionWithPrototypeTypeHandler;
        Field(DynamicType *) externalFunctionWithDeferredPrototypeType;
        Field(DynamicType *) externalFunctionWithLengthAndDeferredPrototypeType;
        Field(DynamicType *) wrappedFunctionWithDeferredPrototypeType;
        Field(DynamicType *) stdCallFunctionWithDeferredPrototypeType;
        Field(DynamicType *) idMappedFunctionWithPrototypeType;
        Field(DynamicType *) externalConstructorFunctionWithDeferredPrototypeType;
        Field(DynamicType *) defaultExternalConstructorFunctionWithDeferredPrototypeType;
        Field(DynamicType *) boundFunctionType;
        Field(DynamicType *) regexConstructorType;
        Field(DynamicType *) crossSiteDeferredPrototypeFunctionType;
        Field(DynamicType *) crossSiteIdMappedFunctionWithPrototypeType;
        Field(DynamicType *) crossSiteExternalConstructFunctionWithPrototypeType;

        Field(StaticType  *) enumeratorType;
        Field(DynamicType *) errorType;
        Field(DynamicType *) evalErrorType;
        Field(DynamicType *) rangeErrorType;
        Field(DynamicType *) referenceErrorType;
        Field(DynamicType *) syntaxErrorType;
        Field(DynamicType *) typeErrorType;
        Field(DynamicType *) uriErrorType;
        Field(DynamicType *) webAssemblyCompileErrorType;
        Field(DynamicType *) webAssemblyRuntimeErrorType;
        Field(DynamicType *) webAssemblyLinkErrorType;
        Field(StaticType  *) numberTypeStatic;
        Field(StaticType  *) int64NumberTypeStatic;
        Field(StaticType  *) uint64NumberTypeStatic;

        Field(DynamicType *) webAssemblyModuleType;
        Field(DynamicType *) webAssemblyInstanceType;
        Field(DynamicType *) webAssemblyMemoryType;
        Field(DynamicType *) webAssemblyTableType;

        Field(DynamicType *) numberTypeDynamic;
        Field(DynamicType *) objectTypes[PreInitializedObjectTypeCount];
        Field(DynamicType *) objectHeaderInlinedTypes[PreInitializedObjectTypeCount];
        Field(DynamicType *) nullPrototypeObjectType;
        Field(DynamicType *) regexPrototypeType;
        Field(DynamicType *) regexType;
        Field(DynamicType *) regexResultType;
        Field(DynamicType *) stringTypeDynamic;
        Field(DynamicType *) mapType;
        Field(DynamicType *) setType;
        Field(DynamicType *) weakMapType;
        Field(DynamicType *) weakSetType;
        Field(DynamicType *) proxyType;
        Field(StaticType  *) withType;
        Field(DynamicType *) SpreadArgumentType;
        Field(DynamicType *) moduleNamespaceType;
        Field(PropertyDescriptor) defaultPropertyDescriptor;

        Field(StringCache) stringCache; // cache string variables once they are used

        // no late caching is needed
        Field(JavascriptString*) nullString;
        Field(JavascriptString*) emptyString;

        // used implicitly by ChakraFull - don't late cache
        Field(JavascriptString*) symbolTypeDisplayString;
        Field(JavascriptString*) debuggerDeadZoneBlockVariableString;

        Field(DynamicObject*) missingPropertyHolder;
        Field(StaticType*) throwErrorObjectType;
        Field(PropertyStringCacheMap*) propertyStringMap;
        Field(SymbolCacheMap*) symbolMap;
        Field(ConstructorCache*) builtInConstructorCache;

        Field(DynamicObject*) chakraLibraryObject;

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        Field(JavascriptFunction*) debugObjectFaultInjectionCookieGetterFunction;
        Field(JavascriptFunction*) debugObjectFaultInjectionCookieSetterFunction;
#endif

        Field(JavascriptFunction*) evalFunctionObject;
        Field(JavascriptFunction*) arrayPrototypeValuesFunction;
        Field(JavascriptFunction*) parseIntFunctionObject;
        Field(JavascriptFunction*) parseFloatFunctionObject;
        Field(JavascriptFunction*) arrayPrototypeToStringFunction;
        Field(JavascriptFunction*) arrayPrototypeToLocaleStringFunction;
        Field(JavascriptFunction*) identityFunction;
        Field(JavascriptFunction*) throwerFunction;
        Field(JavascriptFunction*) generatorReturnFunction;
        Field(JavascriptFunction*) generatorNextFunction;
        Field(JavascriptFunction*) generatorThrowFunction;

        Field(JavascriptFunction*) objectValueOfFunction;
        Field(JavascriptFunction*) objectToStringFunction;

#ifdef ENABLE_JS_BUILTINS
        Field(JavascriptFunction*) isArrayFunction;
#endif

#ifdef ENABLE_WASM
        Field(DynamicObject*) webAssemblyObject;
        Field(JavascriptFunction*) webAssemblyQueryResponseFunction;
        Field(JavascriptFunction*) webAssemblyCompileFunction;
        Field(JavascriptFunction*) webAssemblyInstantiateBoundFunction;
#endif

        Field(JavascriptSymbol*) symbolMatch;
        Field(JavascriptSymbol*) symbolReplace;
        Field(JavascriptSymbol*) symbolSearch;
        Field(JavascriptSymbol*) symbolSplit;

        Field(UnifiedRegex::RegexPattern *) emptyRegexPattern;
        Field(JavascriptFunction*) regexExecFunction;
        Field(JavascriptFunction*) regexFlagsGetterFunction;
        Field(JavascriptFunction*) regexGlobalGetterFunction;
        Field(JavascriptFunction*) regexStickyGetterFunction;
        Field(JavascriptFunction*) regexDotAllGetterFunction;
        Field(JavascriptFunction*) regexUnicodeGetterFunction;

        Field(RuntimeFunction*) sharedArrayBufferConstructor;
        Field(DynamicObject*) sharedArrayBufferPrototype;
        Field(DynamicObject*) atomicsObject;

        Field(DynamicObject*) webAssemblyCompileErrorPrototype;
        Field(RuntimeFunction*) webAssemblyCompileErrorConstructor;
        Field(DynamicObject*) webAssemblyRuntimeErrorPrototype;
        Field(RuntimeFunction*) webAssemblyRuntimeErrorConstructor;
        Field(DynamicObject*) webAssemblyLinkErrorPrototype;
        Field(RuntimeFunction*) webAssemblyLinkErrorConstructor;

        Field(DynamicObject*) webAssemblyMemoryPrototype;
        Field(RuntimeFunction*) webAssemblyMemoryConstructor;
        Field(DynamicObject*) webAssemblyModulePrototype;
        Field(RuntimeFunction*) webAssemblyModuleConstructor;
        Field(DynamicObject*) webAssemblyInstancePrototype;
        Field(RuntimeFunction*) webAssemblyInstanceConstructor;
        Field(DynamicObject*) webAssemblyTablePrototype;
        Field(RuntimeFunction*) webAssemblyTableConstructor;

        Field(int) regexConstructorSlotIndex;
        Field(int) regexExecSlotIndex;
        Field(int) regexFlagsGetterSlotIndex;
        Field(int) regexGlobalGetterSlotIndex;
        Field(int) regexStickyGetterSlotIndex;
        Field(int) regexDotAllGetterSlotIndex;
        Field(int) regexUnicodeGetterSlotIndex;

        mutable Field(CharStringCache) charStringCache;

        FieldNoBarrier(PromiseContinuationCallback) nativeHostPromiseContinuationFunction;
        Field(void *) nativeHostPromiseContinuationFunctionState;

        typedef SList<Js::FunctionProxy*, Recycler> FunctionReferenceList;
        typedef JsUtil::WeakReferenceDictionary<uintptr_t, DynamicType, DictionarySizePolicy<PowerOf2Policy, 1>> JsrtExternalTypesCache;

        Field(void *) bindRefChunkBegin;
        Field(Field(void*)*) bindRefChunkCurrent;
        Field(Field(void*)*) bindRefChunkEnd;
        Field(TypePath*) rootPath;         // this should be in library instead of ScriptContext::Cache
        Field(Js::Cache) cache;
        Field(FunctionReferenceList*) dynamicFunctionReference;
        Field(uint) dynamicFunctionReferenceDepth;
        Field(FinalizableObject*) jsrtContextObject;
        Field(JsrtExternalTypesCache*) jsrtExternalTypesCache;
        Field(FunctionBody*) fakeGlobalFuncForUndefer;

        Field(ModuleRecordList*) moduleRecordList;

        Field(OnlyWritablePropertyProtoChainCache) typesWithOnlyWritablePropertyProtoChain;
        Field(NoSpecialPropertyProtoChainCache) typesWithNoSpecialPropertyProtoChain;

        Field(uint64) randSeed0, randSeed1;
        Field(bool) isPRNGSeeded;
        Field(bool) inProfileMode;
        Field(bool) inDispatchProfileMode;
        Field(bool) arrayObjectHasUserDefinedSpecies;

        JavascriptFunction * AddFunctionToLibraryObjectWithPrototype(DynamicObject * object, PropertyId propertyId, FunctionInfo * functionInfo, int length, DynamicObject * prototype = nullptr, DynamicType * functionType = nullptr);
        JavascriptFunction * AddFunctionToLibraryObject(DynamicObject* object, PropertyId propertyId, FunctionInfo * functionInfo, int length, PropertyAttributes attributes = PropertyBuiltInMethodDefaults);

        JavascriptFunction * AddFunctionToLibraryObjectWithName(DynamicObject* object, PropertyId propertyId, PropertyId nameId, FunctionInfo * functionInfo, int length);
        RuntimeFunction* AddGetterToLibraryObject(DynamicObject* object, PropertyId propertyId, FunctionInfo* functionInfo);
        void AddAccessorsToLibraryObject(DynamicObject* object, PropertyId propertyId, FunctionInfo * getterFunctionInfo, FunctionInfo * setterFunctionInfo);
        void AddAccessorsToLibraryObject(DynamicObject* object, PropertyId propertyId, RecyclableObject * getterFunction, RecyclableObject * setterFunction);
        void AddAccessorsToLibraryObjectWithName(DynamicObject* object, PropertyId propertyId, PropertyId nameId, FunctionInfo * getterFunctionInfo, FunctionInfo * setterFunction);
        void AddSpeciesAccessorsToLibraryObject(DynamicObject* object, FunctionInfo * getterFunctionInfo);
        RuntimeFunction * CreateGetterFunction(PropertyId nameId, FunctionInfo* functionInfo);
        RuntimeFunction * CreateSetterFunction(PropertyId nameId, FunctionInfo* functionInfo);

        template <size_t N>
        JavascriptFunction * AddFunctionToLibraryObjectWithPropertyName(DynamicObject* object, const char16(&propertyName)[N], FunctionInfo * functionInfo, int length);

        static SimpleTypeHandler<1> SharedPrototypeTypeHandler;
        static SimpleTypeHandler<1> SharedFunctionWithoutPrototypeTypeHandler;
        static SimpleTypeHandler<1> SharedFunctionWithPrototypeTypeHandlerV11;
        static SimpleTypeHandler<2> SharedFunctionWithPrototypeTypeHandler;
        static SimpleTypeHandler<1> SharedFunctionWithConfigurableLengthTypeHandler;
        static SimpleTypeHandler<1> SharedFunctionWithLengthTypeHandler;
        static SimpleTypeHandler<2> SharedFunctionWithLengthAndNameTypeHandler;
        static SimpleTypeHandler<2> SharedIdMappedFunctionWithPrototypeTypeHandler;
        static SimpleTypeHandler<1> SharedNamespaceSymbolTypeHandler;
        static SimpleTypeHandler<3> SharedFunctionWithPrototypeLengthAndNameTypeHandler;
        static SimpleTypeHandler<2> SharedFunctionWithPrototypeAndLengthTypeHandler;
        static MissingPropertyTypeHandler MissingPropertyHolderTypeHandler;

        static SimplePropertyDescriptor const SharedFunctionPropertyDescriptors[2];
        static SimplePropertyDescriptor const SharedIdMappedFunctionPropertyDescriptors[2];
        static SimplePropertyDescriptor const HeapArgumentsPropertyDescriptors[3];
        static SimplePropertyDescriptor const FunctionWithLengthAndPrototypeTypeDescriptors[2];
        static SimplePropertyDescriptor const FunctionWithLengthAndNameTypeDescriptors[2];
        static SimplePropertyDescriptor const FunctionWithPrototypeLengthAndNameTypeDescriptors[3];
        static SimplePropertyDescriptor const FunctionWithPrototypeAndLengthTypeDescriptors[2];
        static SimplePropertyDescriptor const ModuleNamespaceTypeDescriptors[1];

    public:


        static const ObjectInfoBits EnumFunctionClass = EnumClass_1_Bit;

        static void InitializeProperties(ThreadContext * threadContext);

        JavascriptLibrary(GlobalObject* globalObject, Recycler * recycler) :
            JavascriptLibraryBase(globalObject),
            inProfileMode(false),
            inDispatchProfileMode(false),
            propertyStringMap(nullptr),
            symbolMap(nullptr),
            parseIntFunctionObject(nullptr),
            evalFunctionObject(nullptr),
            parseFloatFunctionObject(nullptr),
            arrayPrototypeToLocaleStringFunction(nullptr),
            arrayPrototypeToStringFunction(nullptr),
            identityFunction(nullptr),
            throwerFunction(nullptr),
            jsrtContextObject(nullptr),
            jsrtExternalTypesCache(nullptr),
            fakeGlobalFuncForUndefer(nullptr),
            externalLibraryList(nullptr),
#if ENABLE_COPYONACCESS_ARRAY
            cacheForCopyOnAccessArraySegments(nullptr),
#endif
            referencedPropertyRecords(nullptr),
            moduleRecordList(nullptr),
            rootPath(nullptr),
            bindRefChunkBegin(nullptr),
            bindRefChunkCurrent(nullptr),
            bindRefChunkEnd(nullptr),
            dynamicFunctionReference(nullptr),
            typesWithOnlyWritablePropertyProtoChain(recycler),
            typesWithNoSpecialPropertyProtoChain(recycler)
        {
            this->globalObject = globalObject;
        }

        void Initialize(ScriptContext* scriptContext, GlobalObject * globalObject);
        void Uninitialize();
        GlobalObject* GetGlobalObject() const { return globalObject; }
        ScriptContext* GetScriptContext() const { return scriptContext; }

        Recycler * GetRecycler() const { return recycler; }
        Var GetTrueOrFalse(BOOL value) { return value ? booleanTrue : booleanFalse; }
        JavascriptSymbol* GetSymbolMatch() { return symbolMatch; }
        JavascriptSymbol* GetSymbolReplace() { return symbolReplace; }
        JavascriptSymbol* GetSymbolSearch() { return symbolSearch; }
        JavascriptSymbol* GetSymbolSplit() { return symbolSplit; }
        JavascriptSymbol* GetSymbolSpecies() { return symbolSpecies; }
        JavascriptString* GetNullString() { return nullString; }
        JavascriptString* GetEmptyString() const;

#define STRING(name, str) JavascriptString* Get##name##String() { return stringCache.Get##name(); }
#define PROPERTY_STRING(name, str) STRING(name, str)
#include "StringCacheList.h"
#undef PROPERTY_STRING
#undef STRING

        JavascriptString* GetSymbolTypeDisplayString() const { return symbolTypeDisplayString; }
        JavascriptString* GetDebuggerDeadZoneBlockVariableString() { Assert(debuggerDeadZoneBlockVariableString); return debuggerDeadZoneBlockVariableString; }
        JavascriptRegExp* CreateEmptyRegExp();
        JavascriptFunction* GetEvalFunctionObject() { return evalFunctionObject; }
        JavascriptFunction* GetArrayIteratorPrototypeBuiltinNextFunction() { return arrayIteratorPrototypeBuiltinNextFunction; }
        DynamicObject* GetReflectObject() const { return reflectObject; }
        const PropertyDescriptor* GetDefaultPropertyDescriptor() const { return &defaultPropertyDescriptor; }
        DynamicObject* GetMissingPropertyHolder() const { return missingPropertyHolder; }

        JavascriptFunction* GetSharedArrayBufferConstructor() { return sharedArrayBufferConstructor; }
        DynamicObject* GetAtomicsObject() { return atomicsObject; }

        DynamicObject* GetWebAssemblyCompileErrorPrototype() const { return webAssemblyCompileErrorPrototype; }
        DynamicObject* GetWebAssemblyCompileErrorConstructor() const { return webAssemblyCompileErrorConstructor; }
        DynamicObject* GetWebAssemblyRuntimeErrorPrototype() const { return webAssemblyRuntimeErrorPrototype; }
        DynamicObject* GetWebAssemblyRuntimeErrorConstructor() const { return webAssemblyRuntimeErrorConstructor; }
        DynamicObject* GetWebAssemblyLinkErrorPrototype() const { return webAssemblyLinkErrorPrototype; }
        DynamicObject* GetWebAssemblyLinkErrorConstructor() const { return webAssemblyLinkErrorConstructor; }

        DynamicObject* GetChakraLib() const { return chakraLibraryObject; }

#if ENABLE_TTD
        Js::PropertyId ExtractPrimitveSymbolId_TTD(Var value);
        Js::RecyclableObject* CreatePrimitveSymbol_TTD(Js::PropertyId pid);
        Js::RecyclableObject* CreatePrimitveSymbol_TTD(Js::JavascriptString* str);

        Js::RecyclableObject* CreateDefaultBoxedObject_TTD(Js::TypeId kind);
        void SetBoxedObjectValue_TTD(Js::RecyclableObject* obj, Js::Var value);

        Js::RecyclableObject* CreateDate_TTD(double value);
        Js::RecyclableObject* CreateRegex_TTD(const char16* patternSource, uint32 patternLength, UnifiedRegex::RegexFlags flags, CharCount lastIndex, Js::Var lastVar);
        Js::RecyclableObject* CreateError_TTD();

        Js::RecyclableObject* CreateES5Array_TTD();
        static void SetLengthWritableES5Array_TTD(Js::RecyclableObject* es5Array, bool isLengthWritable);

        Js::RecyclableObject* CreateSet_TTD();
        Js::RecyclableObject* CreateWeakSet_TTD();
        static void AddSetElementInflate_TTD(Js::JavascriptSet* set, Var value);
        static void AddWeakSetElementInflate_TTD(Js::JavascriptWeakSet* set, Var value);

        Js::RecyclableObject* CreateMap_TTD();
        Js::RecyclableObject* CreateWeakMap_TTD();
        static void AddMapElementInflate_TTD(Js::JavascriptMap* map, Var key, Var value);
        static void AddWeakMapElementInflate_TTD(Js::JavascriptWeakMap* map, Var key, Var value);

        Js::RecyclableObject* CreateExternalFunction_TTD(Js::Var fname);
        Js::RecyclableObject* CreateBoundFunction_TTD(
                RecyclableObject* function, Var bThis, uint32 ct, Field(Var)* args);

        Js::RecyclableObject* CreateProxy_TTD(RecyclableObject* handler, RecyclableObject* target);
        Js::RecyclableObject* CreateRevokeFunction_TTD(RecyclableObject* proxy);

        Js::RecyclableObject* CreateHeapArguments_TTD(uint32 numOfArguments, uint32 formalCount, ActivationObject* frameObject, byte* deletedArray);
        Js::RecyclableObject* CreateES5HeapArguments_TTD(uint32 numOfArguments, uint32 formalCount, ActivationObject* frameObject, byte* deletedArray);

        Js::JavascriptPromiseCapability* CreatePromiseCapability_TTD(Var promise, Var resolve, Var reject);
        Js::JavascriptPromiseReaction* CreatePromiseReaction_TTD(RecyclableObject* handler, JavascriptPromiseCapability* capabilities);

        Js::RecyclableObject* CreatePromise_TTD(uint32 status, bool isHandled, Var result, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& resolveReactions, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& rejectReactions);
        JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* CreateAlreadyDefinedWrapper_TTD(bool alreadyDefined);
        Js::RecyclableObject* CreatePromiseResolveOrRejectFunction_TTD(RecyclableObject* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolved);
        Js::RecyclableObject* CreatePromiseReactionTaskFunction_TTD(JavascriptPromiseReaction* reaction, Var argument);

        Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* CreateRemainingElementsWrapper_TTD(Js::ScriptContext* ctx, uint32 value);
        Js::RecyclableObject* CreatePromiseAllResolveElementFunction_TTD(Js::JavascriptPromiseCapability* capabilities, uint32 index, Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* wrapper, Js::RecyclableObject* values, bool alreadyCalled);
        Js::RecyclableObject* CreateJavascriptGenerator_TTD(Js::ScriptContext *ctx,
                                                            Js::RecyclableObject *prototype, Js::Arguments &arguments,
                                                            Js::JavascriptGenerator::GeneratorState generatorState);
#endif

#ifdef ENABLE_INTL_OBJECT
        void ResetIntlObject();
        void EnsureIntlObjectReady();
        template <class Fn>
        void InitializeIntlForPrototypes(Fn fn);
        void InitializeIntlForStringPrototype();
        void InitializeIntlForDatePrototype();
        void InitializeIntlForNumberPrototype();
#endif

#ifdef ENABLE_JS_BUILTINS
        template <class Fn>
        void InitializeBuiltInForPrototypes(Fn fn);

        void EnsureBuiltInEngineIsReady();

        static bool __cdecl InitializeChakraLibraryObject(DynamicObject* chakraLibraryObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeBuiltInObject(DynamicObject* builtInEngineObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

#endif

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        DynamicType * GetDebugDisposableObjectType() { return debugDisposableObjectType; }
        DynamicType * GetDebugFuncExecutorInDisposeObjectType() { return debugFuncExecutorInDisposeObjectType; }
#endif

        DynamicType* GetErrorType(ErrorTypeEnum typeToCreate) const;
        StaticType  * GetBooleanTypeStatic() const { return booleanTypeStatic; }
        DynamicType * GetBooleanTypeDynamic() const { return booleanTypeDynamic; }
        DynamicType * GetDateType() const { return dateType; }
        StaticType * GetBigIntTypeStatic() const { return bigintTypeStatic; }
        DynamicType * GetBigIntTypeDynamic() const { return bigintTypeDynamic; }
        DynamicType * GetBoundFunctionType() const { return boundFunctionType; }
        DynamicType * GetRegExpConstructorType() const { return regexConstructorType; }
        StaticType  * GetEnumeratorType() const { return enumeratorType; }
        DynamicType * GetSpreadArgumentType() const { return SpreadArgumentType; }
        StaticType  * GetWithType() const { return withType; }
        DynamicType * GetErrorType() const { return errorType; }
        DynamicType * GetEvalErrorType() const { return evalErrorType; }
        DynamicType * GetRangeErrorType() const { return rangeErrorType; }
        DynamicType * GetReferenceErrorType() const { return referenceErrorType; }
        DynamicType * GetSyntaxErrorType() const { return syntaxErrorType; }
        DynamicType * GetTypeErrorType() const { return typeErrorType; }
        DynamicType * GetURIErrorType() const { return uriErrorType; }
        DynamicType * GetWebAssemblyCompileErrorType() const { return webAssemblyCompileErrorType; }
        DynamicType * GetWebAssemblyRuntimeErrorType() const { return webAssemblyRuntimeErrorType; }
        DynamicType * GetWebAssemblyLinkErrorType() const { return webAssemblyLinkErrorType; }
        StaticType  * GetNumberTypeStatic() const { return numberTypeStatic; }
        StaticType  * GetInt64TypeStatic() const { return int64NumberTypeStatic; }
        StaticType  * GetUInt64TypeStatic() const { return uint64NumberTypeStatic; }

        DynamicType * GetNumberTypeDynamic() const { return numberTypeDynamic; }
        DynamicType * GetPromiseType() const { return promiseType; }

        DynamicType * GetWebAssemblyModuleType()  const { return webAssemblyModuleType; }
        DynamicType * GetWebAssemblyInstanceType()  const { return webAssemblyInstanceType; }
        DynamicType * GetWebAssemblyMemoryType() const { return webAssemblyMemoryType; }
        DynamicType * GetWebAssemblyTableType() const { return webAssemblyTableType; }
        DynamicType * GetGeneratorConstructorPrototypeObjectType() const { return generatorConstructorPrototypeObjectType; }

#ifdef ENABLE_WASM
        JavascriptFunction* GetWebAssemblyQueryResponseFunction() const { return webAssemblyQueryResponseFunction; }
        JavascriptFunction* GetWebAssemblyCompileFunction() const { return webAssemblyCompileFunction; }
        JavascriptFunction* GetWebAssemblyInstantiateBoundFunction() const { return webAssemblyInstantiateBoundFunction; }
#endif
        
        DynamicType * GetObjectLiteralType(uint16 requestedInlineSlotCapacity);
        DynamicType * GetObjectHeaderInlinedLiteralType(uint16 requestedInlineSlotCapacity);
        DynamicType * GetObjectType() const { return objectTypes[0]; }
        DynamicType * GetNullPrototypeObjectType() const { return nullPrototypeObjectType; }
        DynamicType * GetObjectHeaderInlinedType() const { return objectHeaderInlinedTypes[0]; }
        StaticType  * GetSymbolTypeStatic() const { return symbolTypeStatic; }
        DynamicType * GetSymbolTypeDynamic() const { return symbolTypeDynamic; }
        DynamicType * GetProxyType() const { return proxyType; }
        DynamicType * GetHeapArgumentsObjectType() const { return heapArgumentsType; }
        DynamicType * GetActivationObjectType() const { return activationObjectType; }
        DynamicType * GetModuleNamespaceType() const { return moduleNamespaceType; }
        DynamicType * GetArrayType() const { return arrayType; }
        DynamicType * GetNativeIntArrayType() const { return nativeIntArrayType; }
#if ENABLE_COPYONACCESS_ARRAY
        DynamicType * GetCopyOnAccessNativeIntArrayType() const { return copyOnAccessNativeIntArrayType; }
#endif
        DynamicType * GetNativeFloatArrayType() const { return nativeFloatArrayType; }
        DynamicType * GetRegexPrototypeType() const { return regexPrototypeType; }
        DynamicType * GetRegexType() const { return regexType; }
        DynamicType * GetRegexResultType() const { return regexResultType; }
        DynamicType * GetArrayBufferType() const { return arrayBufferType; }
        StaticType  * GetStringTypeStatic() const { return stringCache.GetStringTypeStatic(); }
        DynamicType * GetStringTypeDynamic() const { return stringTypeDynamic; }
        StaticType  * GetVariantDateType() const { return variantDateType; }
        void EnsureDebugObject(DynamicObject* newDebugObject);
        DynamicObject* GetDebugObject() const { Assert(debugObject != nullptr); return debugObject; }
        DynamicType * GetMapType() const { return mapType; }
        DynamicType * GetSetType() const { return setType; }
        DynamicType * GetWeakMapType() const { return weakMapType; }
        DynamicType * GetWeakSetType() const { return weakSetType; }
        DynamicType * GetArrayIteratorType() const { return arrayIteratorType; }
        DynamicType * GetMapIteratorType() const { return mapIteratorType; }
        DynamicType * GetSetIteratorType() const { return setIteratorType; }
        DynamicType * GetStringIteratorType() const { return stringIteratorType; }
        DynamicType * GetListIteratorType() const { return listIteratorType; }
        JavascriptFunction* GetDefaultAccessorFunction() const { return defaultAccessorFunction; }
        JavascriptFunction* GetStackTraceAccessorFunction() const { return stackTraceAccessorFunction; }
        JavascriptFunction* GetThrowTypeErrorRestrictedPropertyAccessorFunction() const { return throwTypeErrorRestrictedPropertyAccessorFunction; }
        JavascriptFunction* Get__proto__getterFunction() const { return __proto__getterFunction; }
        JavascriptFunction* Get__proto__setterFunction() const { return __proto__setterFunction; }

        JavascriptFunction* GetObjectValueOfFunction() const { return objectValueOfFunction; }
        JavascriptFunction* GetObjectToStringFunction() const { return objectToStringFunction; }

        JavascriptFunction* GetDebugObjectNonUserGetterFunction() const { return debugObjectNonUserGetterFunction; }
        JavascriptFunction* GetDebugObjectNonUserSetterFunction() const { return debugObjectNonUserSetterFunction; }

        UnifiedRegex::RegexPattern * GetEmptyRegexPattern() const { return emptyRegexPattern; }
        JavascriptFunction* GetRegexExecFunction() const { return regexExecFunction; }
        JavascriptFunction* GetRegexFlagsGetterFunction() const { return regexFlagsGetterFunction; }
        JavascriptFunction* GetRegexGlobalGetterFunction() const { return regexGlobalGetterFunction; }
        JavascriptFunction* GetRegexStickyGetterFunction() const { return regexStickyGetterFunction; }
        JavascriptFunction* GetRegexDotAllGetterFunction() const { return regexDotAllGetterFunction; }
        JavascriptFunction* GetRegexUnicodeGetterFunction() const { return regexUnicodeGetterFunction; }

        int GetRegexConstructorSlotIndex() const { return regexConstructorSlotIndex;  }
        int GetRegexExecSlotIndex() const { return regexExecSlotIndex;  }
        int GetRegexFlagsGetterSlotIndex() const { return regexFlagsGetterSlotIndex;  }
        int GetRegexGlobalGetterSlotIndex() const { return regexGlobalGetterSlotIndex;  }
        int GetRegexStickyGetterSlotIndex() const { return regexStickyGetterSlotIndex;  }
        int GetRegexDotAllGetterSlotIndex() const { return regexDotAllGetterSlotIndex;  }
        int GetRegexUnicodeGetterSlotIndex() const { return regexUnicodeGetterSlotIndex;  }

        TypePath* GetRootPath() const { return rootPath; }
        void BindReference(void * addr);
        void CleanupForClose();
        void BeginDynamicFunctionReferences();
        void EndDynamicFunctionReferences();
        void RegisterDynamicFunctionReference(FunctionProxy* func);

        void SetDebugObjectNonUserAccessor(FunctionInfo *funcGetter, FunctionInfo *funcSetter);

        JavascriptFunction* GetDebugObjectDebugModeGetterFunction() const { return debugObjectDebugModeGetterFunction; }
        void SetDebugObjectDebugModeAccessor(FunctionInfo *funcGetter);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        JavascriptFunction* GetDebugObjectFaultInjectionCookieGetterFunction() const { return debugObjectFaultInjectionCookieGetterFunction; }
        JavascriptFunction* GetDebugObjectFaultInjectionCookieSetterFunction() const { return debugObjectFaultInjectionCookieSetterFunction; }
        void SetDebugObjectFaultInjectionCookieGetterAccessor(FunctionInfo *funcGetter, FunctionInfo *funcSetter);
#endif

        JavascriptFunction* GetArrayPrototypeToStringFunction() const { return arrayPrototypeToStringFunction; }
        JavascriptFunction* GetArrayPrototypeToLocaleStringFunction() const { return arrayPrototypeToLocaleStringFunction; }
        JavascriptFunction* GetIdentityFunction() const { return identityFunction; }
        JavascriptFunction* GetThrowerFunction() const { return throwerFunction; }

        void SetNativeHostPromiseContinuationFunction(PromiseContinuationCallback function, void *state);

        void CallNativeHostPromiseRejectionTracker(Var promise, Var reason, bool handled);

        void SetJsrtContext(FinalizableObject* jsrtContext);
        FinalizableObject* GetJsrtContext();
        void EnqueueTask(Var taskVar);

        HeapArgumentsObject* CreateHeapArguments(Var frameObj, uint formalCount, bool isStrictMode = false);
        JavascriptArray* CreateArray();
        JavascriptArray* CreateArray(uint32 length);
        JavascriptArray *CreateArrayOnStack(void *const stackAllocationPointer);
        JavascriptNativeIntArray* CreateNativeIntArray();
        JavascriptNativeIntArray* CreateNativeIntArray(uint32 length);
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptCopyOnAccessNativeIntArray* CreateCopyOnAccessNativeIntArray();
        JavascriptCopyOnAccessNativeIntArray* CreateCopyOnAccessNativeIntArray(uint32 length);
#endif
        JavascriptNativeFloatArray* CreateNativeFloatArray();
        JavascriptNativeFloatArray* CreateNativeFloatArray(uint32 length);
        JavascriptArray* CreateArray(uint32 length, uint32 size);
        ArrayBuffer* CreateArrayBuffer(uint32 length);
        ArrayBuffer* CreateArrayBuffer(byte* buffer, uint32 length);
        ArrayBuffer* CreateArrayBuffer(RefCountedBuffer* buffer, uint32 length);
#ifdef ENABLE_WASM
        class WebAssemblyArrayBuffer* CreateWebAssemblyArrayBuffer(uint32 length);
        class WebAssemblyArrayBuffer* CreateWebAssemblyArrayBuffer(byte* buffer, uint32 length);
#ifdef ENABLE_WASM_THREADS
        class WebAssemblySharedArrayBuffer* CreateWebAssemblySharedArrayBuffer(uint32 length, uint32 maxLength);
        class WebAssemblySharedArrayBuffer* CreateWebAssemblySharedArrayBuffer(SharedContents *contents);
#endif
#endif
        SharedArrayBuffer* CreateSharedArrayBuffer(uint32 length);
        SharedArrayBuffer* CreateSharedArrayBuffer(SharedContents *contents);
        ArrayBuffer* CreateProjectionArraybuffer(uint32 length);
        ArrayBuffer* CreateProjectionArraybuffer(byte* buffer, uint32 length);
        ArrayBuffer* CreateProjectionArraybuffer(RefCountedBuffer* buffer, uint32 length);
        ArrayBuffer* CreateExternalArrayBuffer(RefCountedBuffer* buffer, uint32 length);
        DataView* CreateDataView(ArrayBufferBase* arrayBuffer, uint32 offSet, uint32 mappedLength);

        template <typename TypeName, bool clamped>
        inline DynamicType* GetTypedArrayType(TypeName);

        template<> inline DynamicType* GetTypedArrayType<int8,false>(int8) { return int8ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<uint8,false>(uint8) { return uint8ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<uint8,true>(uint8) { return uint8ClampedArrayType; };
        template<> inline DynamicType* GetTypedArrayType<int16,false>(int16) { return int16ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<uint16,false>(uint16) { return uint16ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<int32,false>(int32) { return int32ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<uint32,false>(uint32) { return uint32ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<float,false>(float) { return float32ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<double,false>(double) { return float64ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<int64,false>(int64) { return int64ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<uint64,false>(uint64) { return uint64ArrayType; };
        template<> inline DynamicType* GetTypedArrayType<bool,false>(bool) { return boolArrayType; };

        DynamicType* GetCharArrayType() { return charArrayType; };

        //
        // This method would be used for creating array literals, when we really need to create a huge array
        // Avoids checks at runtime.
        //
        JavascriptArray*            CreateArrayLiteral(uint32 length);
        JavascriptNativeIntArray*   CreateNativeIntArrayLiteral(uint32 length);

#if ENABLE_PROFILE_INFO
        JavascriptNativeIntArray*   CreateCopyOnAccessNativeIntArrayLiteral(ArrayCallSiteInfo *arrayInfo, FunctionBody *functionBody, const Js::AuxArray<int32> *ints);
#endif

        JavascriptNativeFloatArray* CreateNativeFloatArrayLiteral(uint32 length);

        JavascriptBoolean* CreateBoolean(BOOL value);
        JavascriptDate* CreateDate();
        JavascriptDate* CreateDate(double value);
        JavascriptDate* CreateDate(SYSTEMTIME* pst);
        JavascriptMap* CreateMap();
        JavascriptSet* CreateSet();
        JavascriptWeakMap* CreateWeakMap();
        JavascriptWeakSet* CreateWeakSet();
        JavascriptError* CreateError();
        JavascriptError* CreateError(DynamicType* errorType, BOOL isExternal = FALSE);
        JavascriptError* CreateExternalError(ErrorTypeEnum errorTypeEnum);
        JavascriptError* CreateEvalError();
        JavascriptError* CreateRangeError();
        JavascriptError* CreateReferenceError();
        JavascriptError* CreateSyntaxError();
        JavascriptError* CreateTypeError();
        JavascriptError* CreateURIError();
        JavascriptError* CreateStackOverflowError();
        JavascriptError* CreateOutOfMemoryError();
        JavascriptError* CreateWebAssemblyCompileError();
        JavascriptError* CreateWebAssemblyRuntimeError();
        JavascriptError* CreateWebAssemblyLinkError();
        JavascriptSymbol* CreateSymbol(JavascriptString* description);
        JavascriptSymbol* CreateSymbol(const char16* description, int descriptionLength);
        JavascriptSymbol* CreateSymbol(const PropertyRecord* propertyRecord);
        JavascriptPromise* CreatePromise();
        JavascriptGenerator* CreateGenerator(Arguments& args, ScriptFunction* scriptFunction, RecyclableObject* prototype);
        JavascriptFunction* CreateNonProfiledFunction(FunctionInfo * functionInfo);
        template <class MethodType>
        JavascriptExternalFunction* CreateIdMappedExternalFunction(MethodType entryPoint, DynamicType *pPrototypeType);
        JavascriptExternalFunction* CreateExternalConstructor(Js::ExternalMethod entryPoint, PropertyId nameId, RecyclableObject * prototype);
        JavascriptExternalFunction* CreateExternalConstructor(Js::ExternalMethod entryPoint, PropertyId nameId, InitializeMethod method, unsigned short deferredTypeSlots, bool hasAccessors);
        DynamicType* GetCachedJsrtExternalType(uintptr_t finalizeCallback);
        void CacheJsrtExternalType(uintptr_t finalizeCallback, DynamicType* dynamicType);
        static DynamicTypeHandler * GetDeferredPrototypeGeneratorFunctionTypeHandler(ScriptContext* scriptContext);
        static DynamicTypeHandler * GetDeferredPrototypeAsyncFunctionTypeHandler(ScriptContext* scriptContext);
        DynamicType * CreateDeferredPrototypeGeneratorFunctionType(JavascriptMethod entrypoint, bool isAnonymousFunction, bool isShared = false);
        DynamicType * CreateDeferredPrototypeAsyncFunctionType(JavascriptMethod entrypoint, bool isAnonymousFunction, bool isShared = false);

        static DynamicTypeHandler * GetDeferredPrototypeFunctionTypeHandler(ScriptContext* scriptContext);
        static DynamicTypeHandler * GetDeferredPrototypeFunctionWithLengthTypeHandler(ScriptContext* scriptContext);
        static DynamicTypeHandler * GetDeferredAnonymousPrototypeFunctionWithLengthTypeHandler();
        static DynamicTypeHandler * GetDeferredAnonymousPrototypeGeneratorFunctionTypeHandler();
        static DynamicTypeHandler * GetDeferredAnonymousPrototypeAsyncFunctionTypeHandler();

        DynamicTypeHandler * GetDeferredFunctionTypeHandler();
        DynamicTypeHandler * GetDeferredFunctionWithLengthTypeHandler();
        DynamicTypeHandler * GetDeferredPrototypeFunctionWithNameAndLengthTypeHandler();
        DynamicTypeHandler * ScriptFunctionTypeHandler(bool noPrototypeProperty, bool isAnonymousFunction);
        DynamicTypeHandler * GetDeferredAnonymousFunctionWithLengthTypeHandler();
        DynamicTypeHandler * GetDeferredAnonymousFunctionTypeHandler();
        template<bool isNameAvailable, bool isPrototypeAvailable = true, bool isLengthAvailable = false>
        static DynamicTypeHandler * GetDeferredFunctionTypeHandlerBase();
        template<bool isNameAvailable, bool isPrototypeAvailable = true>
        static DynamicTypeHandler * GetDeferredGeneratorFunctionTypeHandlerBase();
        template<bool isNameAvailable>
        static DynamicTypeHandler * GetDeferredAsyncFunctionTypeHandlerBase();

        DynamicType * CreateDeferredPrototypeFunctionType(JavascriptMethod entrypoint);
        DynamicType * CreateDeferredPrototypeFunctionTypeNoProfileThunk(JavascriptMethod entrypoint, bool isShared = false, bool isLengthAvailable = false);
        DynamicType * CreateFunctionType(JavascriptMethod entrypoint, RecyclableObject* prototype = nullptr);
        DynamicType * CreateFunctionWithConfigurableLengthType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndNameType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndPrototypeType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithConfigurableLengthType(DynamicObject * prototype, FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthType(DynamicObject * prototype, FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndNameType(DynamicObject * prototype, FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndPrototypeType(DynamicObject * prototype, FunctionInfo * functionInfo);
        ScriptFunction * CreateScriptFunction(FunctionProxy* proxy);
        AsmJsScriptFunction * CreateAsmJsScriptFunction(FunctionProxy* proxy);
#ifdef ENABLE_WASM
        WasmScriptFunction * CreateWasmScriptFunction(FunctionProxy* proxy);
#endif
        ScriptFunctionWithInlineCache * CreateScriptFunctionWithInlineCache(FunctionProxy* proxy);
        GeneratorVirtualScriptFunction * CreateGeneratorVirtualScriptFunction(FunctionProxy* proxy);

        DynamicType * CreateGeneratorType(RecyclableObject* prototype);

#if 0
        JavascriptNumber* CreateNumber(double value);
#endif
        JavascriptNumber* CreateNumber(double value, RecyclerJavascriptNumberAllocator * numberAllocator);
        JavascriptGeneratorFunction* CreateGeneratorFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction);
        JavascriptGeneratorFunction* CreateGeneratorFunction(JavascriptMethod entryPoint, bool isAnonymousFunction);
        JavascriptAsyncFunction* CreateAsyncFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction);
        JavascriptAsyncFunction* CreateAsyncFunction(JavascriptMethod entryPoint, bool isAnonymousFunction);
        JavascriptExternalFunction* CreateExternalFunction(ExternalMethod entryPointer, PropertyId nameId, Var signature, UINT64 flags, bool isLengthAvailable = false);
        JavascriptExternalFunction* CreateExternalFunction(ExternalMethod entryPointer, Var nameId, Var signature, UINT64 flags, bool isLengthAvailable = false);
        JavascriptExternalFunction* CreateStdCallExternalFunction(StdCallJavascriptMethod entryPointer, Var name, void *callbackState);
        JavascriptPromiseAsyncSpawnExecutorFunction* CreatePromiseAsyncSpawnExecutorFunction(JavascriptGenerator* generator, Var target);
        JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction* CreatePromiseAsyncSpawnStepArgumentExecutorFunction(JavascriptMethod entryPoint, JavascriptGenerator* generator, Var argument, Var resolve = nullptr, Var reject = nullptr, bool isReject = false);
        JavascriptPromiseCapabilitiesExecutorFunction* CreatePromiseCapabilitiesExecutorFunction(JavascriptMethod entryPoint, JavascriptPromiseCapability* capability);
        JavascriptPromiseResolveOrRejectFunction* CreatePromiseResolveOrRejectFunction(JavascriptMethod entryPoint, JavascriptPromise* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord);
        JavascriptPromiseReactionTaskFunction* CreatePromiseReactionTaskFunction(JavascriptMethod entryPoint, JavascriptPromiseReaction* reaction, Var argument);
        JavascriptPromiseResolveThenableTaskFunction* CreatePromiseResolveThenableTaskFunction(JavascriptMethod entryPoint, JavascriptPromise* promise, RecyclableObject* thenable, RecyclableObject* thenFunction);
        JavascriptPromiseAllResolveElementFunction* CreatePromiseAllResolveElementFunction(JavascriptMethod entryPoint, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElements);
        JavascriptPromiseThenFinallyFunction* CreatePromiseThenFinallyFunction(JavascriptMethod entryPoint, RecyclableObject* OnFinally, RecyclableObject* Constructor, bool shouldThrow);
        JavascriptPromiseThunkFinallyFunction* CreatePromiseThunkFinallyFunction(JavascriptMethod entryPoint, Var value, bool shouldThrow);
        JavascriptExternalFunction* CreateWrappedExternalFunction(JavascriptExternalFunction* wrappedFunction);

#if ENABLE_NATIVE_CODEGEN
#if !FLOATVAR
        JavascriptNumber* CreateCodeGenNumber(CodeGenNumberAllocator *alloc, double value);
#endif
#endif

        DynamicObject* CreateGeneratorConstructorPrototypeObject();
        DynamicObject* CreateConstructorPrototypeObject(JavascriptFunction * constructor);
        DynamicObject* CreateObject(const bool allowObjectHeaderInlining = false, const PropertyIndex requestedInlineSlotCapacity = 0);
        DynamicObject* CreateObject(DynamicTypeHandler * typeHandler);
        DynamicObject* CreateActivationObject();
        DynamicObject* CreatePseudoActivationObject();
        DynamicObject* CreateBlockActivationObject();
        DynamicObject* CreateConsoleScopeActivationObject();
        DynamicType* CreateObjectType(RecyclableObject* prototype, Js::TypeId typeId, uint16 requestedInlineSlotCapacity);
        DynamicType* CreateObjectTypeNoCache(RecyclableObject* prototype, Js::TypeId typeId);
        DynamicType* CreateObjectType(RecyclableObject* prototype, uint16 requestedInlineSlotCapacity);
        DynamicObject* CreateObject(RecyclableObject* prototype, uint16 requestedInlineSlotCapacity = 0);

        typedef JavascriptString* LibStringType; // used by diagnostics template
        template< size_t N > JavascriptString* CreateStringFromCppLiteral(const char16 (&value)[N]) const;
        template<> JavascriptString* CreateStringFromCppLiteral(const char16 (&value)[1]) const; // Specialization for empty string
        template<> JavascriptString* CreateStringFromCppLiteral(const char16 (&value)[2]) const; // Specialization for single-char strings
        PropertyString* CreatePropertyString(const Js::PropertyRecord* propertyRecord);

        JavascriptVariantDate* CreateVariantDate(const double value);

        JavascriptBooleanObject* CreateBooleanObject(BOOL value);
        JavascriptBooleanObject* CreateBooleanObject();
        JavascriptNumberObject* CreateNumberObjectWithCheck(double value);
        JavascriptNumberObject* CreateNumberObject(Var number);
        JavascriptStringObject* CreateStringObject(JavascriptString* value);
        JavascriptStringObject* CreateStringObject(const char16* value, charcount_t length);
        JavascriptSymbolObject* CreateSymbolObject(JavascriptSymbol* value);
        JavascriptArrayIterator* CreateArrayIterator(Var iterable, JavascriptArrayIteratorKind kind);
        JavascriptMapIterator* CreateMapIterator(JavascriptMap* map, JavascriptMapIteratorKind kind);
        JavascriptSetIterator* CreateSetIterator(JavascriptSet* set, JavascriptSetIteratorKind kind);
        JavascriptStringIterator* CreateStringIterator(JavascriptString* string);
        JavascriptListIterator* CreateListIterator(ListForListIterator* list);

        JavascriptRegExp* CreateRegExp(UnifiedRegex::RegexPattern* pattern);

        DynamicObject* CreateIteratorResultObject(Var value, Var done);
        DynamicObject* CreateIteratorResultObjectValueFalse(Var value);
        DynamicObject* CreateIteratorResultObjectUndefinedTrue();

        RecyclableObject* CreateThrowErrorObject(JavascriptError* error);

        JavascriptFunction* EnsurePromiseResolveFunction();
        JavascriptFunction* EnsurePromiseThenFunction();
        JavascriptFunction* EnsureGeneratorReturnFunction();
        JavascriptFunction* EnsureGeneratorNextFunction();
        JavascriptFunction* EnsureGeneratorThrowFunction();
        JavascriptFunction* EnsureArrayPrototypeForEachFunction();
        JavascriptFunction* EnsureArrayPrototypeKeysFunction();
        JavascriptFunction* EnsureArrayPrototypeEntriesFunction();
        JavascriptFunction* EnsureArrayPrototypeValuesFunction();
        JavascriptFunction* EnsureJSONStringifyFunction();
        JavascriptFunction* EnsureObjectFreezeFunction();

        void SetCrossSiteForLockedFunctionType(JavascriptFunction * function);
        void SetCrossSiteForLockedNonBuiltInFunctionType(JavascriptFunction * function);

        bool IsPRNGSeeded() { return isPRNGSeeded; }
        uint64 GetRandSeed0() { return randSeed0; }
        uint64 GetRandSeed1() { return randSeed1; }
        void SetIsPRNGSeeded(bool val);
        void SetRandSeed0(uint64 rs) { randSeed0 = rs;}
        void SetRandSeed1(uint64 rs) { randSeed1 = rs; }

        void SetProfileMode(bool fSet);
        void SetDispatchProfile(bool fSet, JavascriptMethod dispatchInvoke);
        HRESULT ProfilerRegisterBuiltIns();

#if ENABLE_COPYONACCESS_ARRAY
        static bool IsCopyOnAccessArrayCallSite(JavascriptLibrary *lib, ArrayCallSiteInfo *arrayInfo, uint32 length);
        static bool IsCachedCopyOnAccessArrayCallSite(const JavascriptLibrary *lib, ArrayCallSiteInfo *arrayInfo);
        template <typename T>
        static void CheckAndConvertCopyOnAccessNativeIntArray(const T instance);
#endif    

        static void CheckAndInvalidateIsConcatSpreadableCache(PropertyId propertyId, ScriptContext *scriptContext);

#if DBG_DUMP
        static const char16* GetStringTemplateCallsiteObjectKey(Var callsite);
#endif

        Field(JavascriptFunction*)* GetBuiltinFunctions();
        INT_PTR* GetVTableAddresses();
        static BuiltinFunction GetBuiltinFunctionForPropId(PropertyId id);
        static BuiltinFunction GetBuiltInForFuncInfo(LocalFunctionId localFuncId);
#if DBG
        static void CheckRegisteredBuiltIns(Field(JavascriptFunction*)* builtInFuncs, ScriptContext *scriptContext);
#endif
        static BOOL CanFloatPreferenceFunc(BuiltinFunction index);
        static BOOL IsFltFunc(BuiltinFunction index);
        static bool IsFloatFunctionCallsite(BuiltinFunction index, size_t argc);
        static bool IsFltBuiltInConst(PropertyId id);
        static size_t GetArgCForBuiltIn(BuiltinFunction index)
        {
            Assert(index < _countof(JavascriptLibrary::LibraryFunctionArgC));
            return JavascriptLibrary::LibraryFunctionArgC[index];
        }
        static BuiltInFlags GetFlagsForBuiltIn(BuiltinFunction index)
        {
            Assert(index < _countof(JavascriptLibrary::LibraryFunctionFlags));
            return (BuiltInFlags)JavascriptLibrary::LibraryFunctionFlags[index];
        }
        static BuiltinFunction GetBuiltInInlineCandidateId(Js::OpCode opCode);
        static BuiltInArgSpecializationType GetBuiltInArgType(BuiltInFlags flags, BuiltInArgShift argGroup);
        static bool IsTypeSpecRequired(BuiltInFlags flags)
        {
            return GetBuiltInArgType(flags, BuiltInArgShift::BIAS_Src1) || GetBuiltInArgType(flags, BuiltInArgShift::BIAS_Src2) || GetBuiltInArgType(flags, BuiltInArgShift::BIAS_Dst);
        }
#if ENABLE_DEBUG_CONFIG_OPTIONS
        static char16 const * GetNameForBuiltIn(BuiltinFunction index)
        {
            Assert(index < _countof(JavascriptLibrary::LibraryFunctionName));
            return JavascriptLibrary::LibraryFunctionName[index];
        }
#endif

        PropertyStringCacheMap* EnsurePropertyStringMap();
        SymbolCacheMap* EnsureSymbolMap();

        template <typename TProperty> WeakPropertyIdMap<TProperty>* GetPropertyMap();
        template <> PropertyStringCacheMap* GetPropertyMap<PropertyString>() { return this->propertyStringMap; }
        template <> SymbolCacheMap* GetPropertyMap<JavascriptSymbol>() { return this->symbolMap; }

        Field(OnlyWritablePropertyProtoChainCache*) GetTypesWithOnlyWritablePropertyProtoChainCache() { return &this->typesWithOnlyWritablePropertyProtoChain; }
        Field(NoSpecialPropertyProtoChainCache*) GetTypesWithNoSpecialPropertyProtoChainCache() { return &this->typesWithNoSpecialPropertyProtoChain; }

        static bool IsDefaultArrayValuesFunction(RecyclableObject * function, ScriptContext *scriptContext);
        static bool ArrayIteratorPrototypeHasUserDefinedNext(ScriptContext *scriptContext);

        CharStringCache& GetCharStringCache() { return charStringCache;  }
        static JavascriptLibrary * FromCharStringCache(CharStringCache * cache)
        {
            return (JavascriptLibrary *)((uintptr_t)cache - offsetof(JavascriptLibrary, charStringCache));
        }

        EnumeratorCache* GetObjectAssignCache(Type* type);
        EnumeratorCache* GetStringifyCache(Type* type);

        bool GetArrayObjectHasUserDefinedSpecies() const { return arrayObjectHasUserDefinedSpecies; }
        void SetArrayObjectHasUserDefinedSpecies(bool val) { arrayObjectHasUserDefinedSpecies = val; }

        FunctionBody* GetFakeGlobalFuncForUndefer()const { return this->fakeGlobalFuncForUndefer; }
        void SetFakeGlobalFuncForUndefer(FunctionBody* functionBody) { this->fakeGlobalFuncForUndefer = functionBody; }

        ModuleRecordList* EnsureModuleRecordList();
        SourceTextModuleRecord* GetModuleRecord(uint moduleId);

    private:
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        // Declare fretest/debug properties here since otherwise it can cause
        // a mismatch between fre mshtml and fretest jscript9 causing undefined behavior

        Field(DynamicType *) debugDisposableObjectType;
        Field(DynamicType *) debugFuncExecutorInDisposeObjectType;
#endif

        void InitializePrototypes();
        void InitializeTypes();
        void InitializeGlobal(GlobalObject * globalObject);
        static void PrecalculateArrayAllocationBuckets();

#define STANDARD_INIT(name) \
        static bool __cdecl Initialize##name##Constructor(DynamicObject* arrayConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode); \
        static bool __cdecl Initialize##name##Prototype(DynamicObject* arrayPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        STANDARD_INIT(Array);
        STANDARD_INIT(SharedArrayBuffer);
        STANDARD_INIT(ArrayBuffer);
        STANDARD_INIT(DataView);
        STANDARD_INIT(Error);
        STANDARD_INIT(EvalError);
        STANDARD_INIT(RangeError);
        STANDARD_INIT(ReferenceError);
        STANDARD_INIT(SyntaxError);
        STANDARD_INIT(TypeError);
        STANDARD_INIT(URIError);
        STANDARD_INIT(RuntimeError);
        STANDARD_INIT(TypedArray);
        STANDARD_INIT(Int8Array);
        STANDARD_INIT(Uint8Array);
        STANDARD_INIT(Uint8ClampedArray);
        STANDARD_INIT(Int16Array);
        STANDARD_INIT(Uint16Array);
        STANDARD_INIT(Int32Array);
        STANDARD_INIT(Uint32Array);
        STANDARD_INIT(Float32Array);
        STANDARD_INIT(Float64Array);
        STANDARD_INIT(Boolean);
        STANDARD_INIT(Symbol);
        STANDARD_INIT(Date);
        STANDARD_INIT(Proxy);
        STANDARD_INIT(Function);
        STANDARD_INIT(Number);
        STANDARD_INIT(BigInt);
        STANDARD_INIT(Object);
        STANDARD_INIT(Regex);
        STANDARD_INIT(String);
        STANDARD_INIT(Map);
        STANDARD_INIT(Set);
        STANDARD_INIT(WeakMap);
        STANDARD_INIT(WeakSet);
        STANDARD_INIT(Promise);
        STANDARD_INIT(GeneratorFunction);
        STANDARD_INIT(AsyncFunction);
        STANDARD_INIT(WebAssemblyCompileError);
        STANDARD_INIT(WebAssemblyRuntimeError);
        STANDARD_INIT(WebAssemblyLinkError);
        STANDARD_INIT(WebAssemblyMemory);
        STANDARD_INIT(WebAssemblyModule);
        STANDARD_INIT(WebAssemblyInstance);
        STANDARD_INIT(WebAssemblyTable);

#undef STANDARD_INIT

        static bool __cdecl InitializeAtomicsObject(DynamicObject* atomicsObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        static bool __cdecl InitializeInt64ArrayPrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeUint64ArrayPrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeBoolArrayPrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeCharArrayPrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        void InitializeComplexThings();
        void InitializeStaticValues();
        static bool __cdecl InitializeMathObject(DynamicObject* mathObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
#ifdef ENABLE_WASM
        static bool __cdecl InitializeWebAssemblyObject(DynamicObject* WasmObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
#endif

        static bool __cdecl InitializeJSONObject(DynamicObject* JSONObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeEngineInterfaceObject(DynamicObject* engineInterface, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeReflectObject(DynamicObject* reflectObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
#ifdef ENABLE_INTL_OBJECT
        static bool __cdecl InitializeIntlObject(DynamicObject* IntlEngineObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
#endif

#ifdef ENABLE_PROJECTION
        void InitializeWinRTPromiseConstructor();
#endif

        static bool __cdecl InitializeIteratorPrototype(DynamicObject* iteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeArrayIteratorPrototype(DynamicObject* arrayIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeMapIteratorPrototype(DynamicObject* mapIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSetIteratorPrototype(DynamicObject* setIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeStringIteratorPrototype(DynamicObject* stringIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        static bool __cdecl InitializeGeneratorPrototype(DynamicObject* generatorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        static bool __cdecl InitializeAsyncFunction(DynamicObject *function, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        RuntimeFunction* CreateBuiltinConstructor(FunctionInfo * functionInfo, DynamicTypeHandler * typeHandler, DynamicObject* prototype = nullptr);
        void DefaultCreateFunction(ParseableFunctionInfo * functionInfo, int length, DynamicObject * prototype, PropertyId nameId);
        RuntimeFunction* DefaultCreateFunction(FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType, PropertyId nameId);
        RuntimeFunction* DefaultCreateFunction(FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType, Var nameId);
        JavascriptFunction* AddFunction(DynamicObject* object, PropertyId propertyId, RuntimeFunction* function);
        void AddMember(DynamicObject* object, PropertyId propertyId, Var value);
        void AddMember(DynamicObject* object, PropertyId propertyId, Var value, PropertyAttributes attributes);
        JavascriptString* CreateEmptyString();

        template<uint cacheSlotCount> EnumeratorCache* GetEnumeratorCache(Type* type, Field(EnumeratorCache*)* cacheSlots);

        static bool __cdecl InitializeGeneratorFunction(DynamicObject* function, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        static size_t const LibraryFunctionArgC[BuiltinFunction::Count + 1];
        static int const LibraryFunctionFlags[BuiltinFunction::Count + 1];   // returns enum BuiltInFlags.
#if ENABLE_DEBUG_CONFIG_OPTIONS
        static char16 const * const LibraryFunctionName[BuiltinFunction::Count + 1];
#endif

    public:
        template<bool addPrototype, bool addName, bool useLengthType>
        static bool __cdecl InitializeFunction(DynamicObject* function, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        virtual void Finalize(bool isShutdown) override;

#if DBG
        void DumpLibraryByteCode();
#endif
    private:
        typedef JsUtil::BaseHashSet<Js::PropertyRecord const *, Recycler, PowerOf2SizePolicy> ReferencedPropertyRecordHashSet;
        Field(ReferencedPropertyRecordHashSet*) referencedPropertyRecords;

        ReferencedPropertyRecordHashSet * EnsureReferencedPropertyRecordList()
        {
            ReferencedPropertyRecordHashSet* pidList = this->referencedPropertyRecords;
            if (pidList == nullptr)
            {
                pidList = RecyclerNew(this->recycler, ReferencedPropertyRecordHashSet, this->recycler, 173);
                this->referencedPropertyRecords = pidList;
            }
            return pidList;
        }

        ReferencedPropertyRecordHashSet * GetReferencedPropertyRecordList() const
        {
            return this->referencedPropertyRecords;
        }

        HRESULT ProfilerRegisterObject();
        HRESULT ProfilerRegisterArray();
        HRESULT ProfilerRegisterBoolean();
        HRESULT ProfilerRegisterDate();
        HRESULT ProfilerRegisterFunction();
        HRESULT ProfilerRegisterMath();
        HRESULT ProfilerRegisterNumber();
        HRESULT ProfilerRegisterBigInt();
        HRESULT ProfilerRegisterString();
        HRESULT ProfilerRegisterRegExp();
        HRESULT ProfilerRegisterJSON();
        HRESULT ProfilerRegisterMap();
        HRESULT ProfilerRegisterSet();
        HRESULT ProfilerRegisterWeakMap();
        HRESULT ProfilerRegisterWeakSet();
        HRESULT ProfilerRegisterSymbol();
        HRESULT ProfilerRegisterIterator();
        HRESULT ProfilerRegisterArrayIterator();
        HRESULT ProfilerRegisterMapIterator();
        HRESULT ProfilerRegisterSetIterator();
        HRESULT ProfilerRegisterStringIterator();
        HRESULT ProfilerRegisterTypedArray();
        HRESULT ProfilerRegisterPromise();
        HRESULT ProfilerRegisterProxy();
        HRESULT ProfilerRegisterReflect();
        HRESULT ProfilerRegisterGenerator();
        HRESULT ProfilerRegisterAtomics();

#ifdef IR_VIEWER
        HRESULT ProfilerRegisterIRViewer();
#endif /* IR_VIEWER */
    };
}
