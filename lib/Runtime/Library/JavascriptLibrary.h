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

    typedef JsUtil::BaseDictionary<JavascriptMethod, JavascriptFunction*, Recycler, PowerOf2SizePolicy> BuiltInLibraryFunctionMap;

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
        Field(PropertyStringMap*) propertyStrings[80];
        Field(JavascriptString *) lastNumberToStringRadix10String;
        Field(EnumeratedObjectCache) enumObjCache;
        Field(JavascriptString *) lastUtcTimeFromStrString;
        Field(EvalCacheDictionary*) evalCacheDictionary;
        Field(EvalCacheDictionary*) indirectEvalCacheDictionary;
        Field(NewFunctionCache*) newFunctionCache;
        Field(RegexPatternMruMap *) dynamicRegexMap;
        Field(SourceContextInfoMap*) sourceContextInfoMap;   // maps host provided context cookie to the URL of the script buffer passed.
        Field(DynamicSourceContextInfoMap*) dynamicSourceContextInfoMap;
        Field(SourceContextInfo*) noContextSourceContextInfo;
        Field(SRCINFO*) noContextGlobalSourceInfo;
        Field(Field(SRCINFO const *)*) moduleSrcInfo;
        Field(BuiltInLibraryFunctionMap*) builtInLibraryFunctions;
#if ENABLE_PROFILE_INFO
#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)
        Field(DynamicProfileInfoList*) profileInfoList;
#endif
#endif
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

    template <typename T>
    struct StringTemplateCallsiteObjectComparer
    {
        static bool Equals(T x, T y)
        {
            static_assert(false, "Unexpected type T");
        }
        static hash_t GetHashCode(T i)
        {
            static_assert(false, "Unexpected type T");
        }
    };

    template <>
    struct StringTemplateCallsiteObjectComparer<ParseNodePtr>
    {
        static bool Equals(ParseNodePtr x, RecyclerWeakReference<Js::RecyclableObject>* y);
        static bool Equals(ParseNodePtr x, ParseNodePtr y);
        static hash_t GetHashCode(ParseNodePtr i);
    };

    template <>
    struct StringTemplateCallsiteObjectComparer<RecyclerWeakReference<Js::RecyclableObject>*>
    {
        static bool Equals(RecyclerWeakReference<Js::RecyclableObject>* x, RecyclerWeakReference<Js::RecyclableObject>* y);
        static bool Equals(RecyclerWeakReference<Js::RecyclableObject>* x, ParseNodePtr y);
        static hash_t GetHashCode(RecyclerWeakReference<Js::RecyclableObject>* o);
    };

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
        Field(DynamicTypeHandler *) functionWithPrototypeTypeHandler;
        Field(DynamicType *) externalFunctionWithDeferredPrototypeType;
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

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        Field(DynamicType *) simdBool8x16TypeDynamic;
        Field(DynamicType *) simdBool16x8TypeDynamic;
        Field(DynamicType *) simdBool32x4TypeDynamic;
        Field(DynamicType *) simdInt8x16TypeDynamic;
        Field(DynamicType *) simdInt16x8TypeDynamic;
        Field(DynamicType *) simdInt32x4TypeDynamic;
        Field(DynamicType *) simdUint8x16TypeDynamic;
        Field(DynamicType *) simdUint16x8TypeDynamic;
        Field(DynamicType *) simdUint32x4TypeDynamic;
        Field(DynamicType *) simdFloat32x4TypeDynamic;

        Field(StaticType *) simdFloat32x4TypeStatic;
        Field(StaticType *) simdInt32x4TypeStatic;
        Field(StaticType *) simdInt8x16TypeStatic;
        Field(StaticType *) simdFloat64x2TypeStatic;
        Field(StaticType *) simdInt16x8TypeStatic;
        Field(StaticType *) simdBool32x4TypeStatic;
        Field(StaticType *) simdBool16x8TypeStatic;
        Field(StaticType *) simdBool8x16TypeStatic;
        Field(StaticType *) simdUint32x4TypeStatic;
        Field(StaticType *) simdUint16x8TypeStatic;
        Field(StaticType *) simdUint8x16TypeStatic;
#endif // #ifdef ENABLE_SIMDJS

        Field(DynamicType *) numberTypeDynamic;
        Field(DynamicType *) objectTypes[PreInitializedObjectTypeCount];
        Field(DynamicType *) objectHeaderInlinedTypes[PreInitializedObjectTypeCount];
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
        Field(ConstructorCache*) builtInConstructorCache;

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        Field(JavascriptFunction*) debugObjectFaultInjectionCookieGetterFunction;
        Field(JavascriptFunction*) debugObjectFaultInjectionCookieSetterFunction;
#endif

        Field(JavascriptFunction*) evalFunctionObject;
        Field(JavascriptFunction*) parseIntFunctionObject;
        Field(JavascriptFunction*) parseFloatFunctionObject;
        Field(JavascriptFunction*) arrayPrototypeToStringFunction;
        Field(JavascriptFunction*) arrayPrototypeToLocaleStringFunction;
        Field(JavascriptFunction*) identityFunction;
        Field(JavascriptFunction*) throwerFunction;
        Field(JavascriptFunction*) generatorNextFunction;
        Field(JavascriptFunction*) generatorThrowFunction;

        Field(JavascriptFunction*) objectValueOfFunction;
        Field(JavascriptFunction*) objectToStringFunction;

#ifdef ENABLE_WASM
        Field(DynamicObject*) webAssemblyObject;
        Field(JavascriptFunction*) webAssemblyQueryResponseFunction;
        Field(JavascriptFunction*) webAssemblyCompileFunction;
        Field(JavascriptFunction*) webAssemblyInstantiateBoundFunction;
#endif

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        Field(JavascriptFunction*) simdFloat32x4ToStringFunction;
        Field(JavascriptFunction*) simdFloat64x2ToStringFunction;
        Field(JavascriptFunction*) simdInt32x4ToStringFunction;
        Field(JavascriptFunction*) simdInt16x8ToStringFunction;
        Field(JavascriptFunction*) simdInt8x16ToStringFunction;
        Field(JavascriptFunction*) simdBool32x4ToStringFunction;
        Field(JavascriptFunction*) simdBool16x8ToStringFunction;
        Field(JavascriptFunction*) simdBool8x16ToStringFunction;
        Field(JavascriptFunction*) simdUint32x4ToStringFunction;
        Field(JavascriptFunction*) simdUint16x8ToStringFunction;
        Field(JavascriptFunction*) simdUint8x16ToStringFunction;
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

        typedef JsUtil::BaseHashSet<RecyclerWeakReference<RecyclableObject>*, Recycler, PowerOf2SizePolicy, RecyclerWeakReference<RecyclableObject>*, StringTemplateCallsiteObjectComparer> StringTemplateCallsiteObjectList;

        // Used to store a list of template callsite objects.
        // We use the raw strings in the callsite object (or a string template parse node) to identify unique callsite objects in the list.
        // See abstract operation GetTemplateObject in ES6 Spec (RC1) 12.2.8.3
        Field(StringTemplateCallsiteObjectList*) stringTemplateCallsiteObjectList;

        Field(ModuleRecordList*) moduleRecordList;

        // This list contains types ensured to have only writable data properties in it and all objects in its prototype chain
        // (i.e., no readonly properties or accessors). Only prototype objects' types are stored in the list. When something
        // in the script context adds a readonly property or accessor to an object that is used as a prototype object, this
        // list is cleared. The list is also cleared before garbage collection so that it does not keep growing, and so, it can
        // hold strong references to the types.
        //
        // The cache is used by the type-without-property local inline cache. When setting a property on a type that doesn't
        // have the property, to determine whether to promote the object like an object of that type was last promoted, we need
        // to ensure that objects in the prototype chain have not acquired a readonly property or setter (ideally, only for that
        // property ID, but we just check for any such property). This cache is used to avoid doing this many times, especially
        // when the prototype chain is not short.
        //
        // This list is only used to invalidate the status of types. The type itself contains a boolean indicating whether it
        // and prototypes contain only writable data properties, which is reset upon invalidating the status.
        Field(JsUtil::List<Type *> *) typesEnsuredToHaveOnlyWritableDataPropertiesInItAndPrototypeChain;

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
        static SimpleTypeHandler<1> SharedFunctionWithLengthTypeHandler;
        static SimpleTypeHandler<2> SharedFunctionWithLengthAndNameTypeHandler;
        static SimpleTypeHandler<1> SharedIdMappedFunctionWithPrototypeTypeHandler;
        static SimpleTypeHandler<1> SharedNamespaceSymbolTypeHandler;
        static MissingPropertyTypeHandler MissingPropertyHolderTypeHandler;

        static SimplePropertyDescriptor const SharedFunctionPropertyDescriptors[2];
        static SimplePropertyDescriptor const HeapArgumentsPropertyDescriptors[3];
        static SimplePropertyDescriptor const FunctionWithLengthAndPrototypeTypeDescriptors[2];
        static SimplePropertyDescriptor const FunctionWithLengthAndNameTypeDescriptors[2];
        static SimplePropertyDescriptor const ModuleNamespaceTypeDescriptors[1];

    public:


        static const ObjectInfoBits EnumFunctionClass = EnumClass_1_Bit;

        static void InitializeProperties(ThreadContext * threadContext);

        JavascriptLibrary(GlobalObject* globalObject) :
            JavascriptLibraryBase(globalObject),
            inProfileMode(false),
            inDispatchProfileMode(false),
            propertyStringMap(nullptr),
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
            stringTemplateCallsiteObjectList(nullptr),
            moduleRecordList(nullptr),
            rootPath(nullptr),
            bindRefChunkBegin(nullptr),
            bindRefChunkCurrent(nullptr),
            bindRefChunkEnd(nullptr),
            dynamicFunctionReference(nullptr)
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

#define SCACHE_FUNCTION_PROXY(name) JavascriptString* name() { return stringCache.##name##(); }
#ifdef ENABLE_SIMDJS
        SCACHE_FUNCTION_PROXY(GetSIMDFloat32x4DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDFloat64x2DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDInt32x4DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDInt16x8DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDInt8x16DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDBool32x4DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDBool16x8DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDBool8x16DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDUint32x4DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDUint16x8DisplayString)
        SCACHE_FUNCTION_PROXY(GetSIMDUint8x16DisplayString)
#endif
        SCACHE_FUNCTION_PROXY(GetEmptyObjectString)
        SCACHE_FUNCTION_PROXY(GetQuotesString)
        SCACHE_FUNCTION_PROXY(GetWhackString)
        SCACHE_FUNCTION_PROXY(GetCommaDisplayString)
        SCACHE_FUNCTION_PROXY(GetCommaSpaceDisplayString)
        SCACHE_FUNCTION_PROXY(GetOpenBracketString)
        SCACHE_FUNCTION_PROXY(GetCloseBracketString)
        SCACHE_FUNCTION_PROXY(GetOpenSBracketString)
        SCACHE_FUNCTION_PROXY(GetCloseSBracketString)
        SCACHE_FUNCTION_PROXY(GetEmptyArrayString)
        SCACHE_FUNCTION_PROXY(GetNewLineString)
        SCACHE_FUNCTION_PROXY(GetColonString)
        SCACHE_FUNCTION_PROXY(GetFunctionAnonymousString)
        SCACHE_FUNCTION_PROXY(GetFunctionPTRAnonymousString)
        SCACHE_FUNCTION_PROXY(GetAsyncFunctionAnonymouseString)
        SCACHE_FUNCTION_PROXY(GetOpenRBracketString)
        SCACHE_FUNCTION_PROXY(GetNewLineCloseRBracketString)
        SCACHE_FUNCTION_PROXY(GetSpaceOpenBracketString)
        SCACHE_FUNCTION_PROXY(GetNewLineCloseBracketString)
        SCACHE_FUNCTION_PROXY(GetFunctionPrefixString)
        SCACHE_FUNCTION_PROXY(GetGeneratorFunctionPrefixString)
        SCACHE_FUNCTION_PROXY(GetAsyncFunctionPrefixString)
        SCACHE_FUNCTION_PROXY(GetFunctionDisplayString)
        SCACHE_FUNCTION_PROXY(GetXDomainFunctionDisplayString)
        SCACHE_FUNCTION_PROXY(GetInvalidDateString)
        SCACHE_FUNCTION_PROXY(GetObjectDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectArgumentsDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectArrayDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectBooleanDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectDateDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectErrorDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectFunctionDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectNumberDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectRegExpDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectStringDisplayString)
        SCACHE_FUNCTION_PROXY(GetUndefinedDisplayString)
        SCACHE_FUNCTION_PROXY(GetNaNDisplayString)
        SCACHE_FUNCTION_PROXY(GetNullDisplayString)
        SCACHE_FUNCTION_PROXY(GetUnknownDisplayString)
        SCACHE_FUNCTION_PROXY(GetTrueDisplayString)
        SCACHE_FUNCTION_PROXY(GetFalseDisplayString)
        SCACHE_FUNCTION_PROXY(GetStringTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetObjectTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetFunctionTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetBooleanTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetNumberTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetModuleTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetVariantDateTypeDisplayString)
        SCACHE_FUNCTION_PROXY(GetSymbolTypeDisplayString)
#undef  SCACHE_FUNCTION_PROXY

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
        Js::RecyclableObject* CreateBoundFunction_TTD(RecyclableObject* function, Var bThis, uint32 ct, Var* args);

        Js::RecyclableObject* CreateProxy_TTD(RecyclableObject* handler, RecyclableObject* target);
        Js::RecyclableObject* CreateRevokeFunction_TTD(RecyclableObject* proxy);

        Js::RecyclableObject* CreateHeapArguments_TTD(uint32 numOfArguments, uint32 formalCount, ActivationObject* frameObject, byte* deletedArray);
        Js::RecyclableObject* CreateES5HeapArguments_TTD(uint32 numOfArguments, uint32 formalCount, ActivationObject* frameObject, byte* deletedArray);

        Js::JavascriptPromiseCapability* CreatePromiseCapability_TTD(Var promise, Var resolve, Var reject);
        Js::JavascriptPromiseReaction* CreatePromiseReaction_TTD(RecyclableObject* handler, JavascriptPromiseCapability* capabilities);

        Js::RecyclableObject* CreatePromise_TTD(uint32 status, Var result, JsUtil::List<Js::JavascriptPromiseReaction*, HeapAllocator>& resolveReactions, JsUtil::List<Js::JavascriptPromiseReaction*, HeapAllocator>& rejectReactions);
        JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* CreateAlreadyDefinedWrapper_TTD(bool alreadyDefined);
        Js::RecyclableObject* CreatePromiseResolveOrRejectFunction_TTD(RecyclableObject* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolved);
        Js::RecyclableObject* CreatePromiseReactionTaskFunction_TTD(JavascriptPromiseReaction* reaction, Var argument);

        Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* CreateRemainingElementsWrapper_TTD(Js::ScriptContext* ctx, uint32 value);
        Js::RecyclableObject* CreatePromiseAllResolveElementFunction_TTD(Js::JavascriptPromiseCapability* capabilities, uint32 index, Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* wrapper, Js::RecyclableObject* values, bool alreadyCalled);
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

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        DynamicType * GetDebugDisposableObjectType() { return debugDisposableObjectType; }
        DynamicType * GetDebugFuncExecutorInDisposeObjectType() { return debugFuncExecutorInDisposeObjectType; }
#endif

        DynamicType* GetErrorType(ErrorTypeEnum typeToCreate) const;
        StaticType  * GetBooleanTypeStatic() const { return booleanTypeStatic; }
        DynamicType * GetBooleanTypeDynamic() const { return booleanTypeDynamic; }
        DynamicType * GetDateType() const { return dateType; }
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
#ifdef ENABLE_WASM
        JavascriptFunction* GetWebAssemblyQueryResponseFunction() const { return webAssemblyQueryResponseFunction; }
        JavascriptFunction* GetWebAssemblyCompileFunction() const { return webAssemblyCompileFunction; }
        JavascriptFunction* GetWebAssemblyInstantiateBoundFunction() const { return webAssemblyInstantiateBoundFunction; }
#endif

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        DynamicType * GetSIMDBool8x16TypeDynamic()  const { return simdBool8x16TypeDynamic;  }
        DynamicType * GetSIMDBool16x8TypeDynamic()  const { return simdBool16x8TypeDynamic;  }
        DynamicType * GetSIMDBool32x4TypeDynamic()  const { return simdBool32x4TypeDynamic;  }
        DynamicType * GetSIMDInt8x16TypeDynamic()   const { return simdInt8x16TypeDynamic;   }
        DynamicType * GetSIMDInt16x8TypeDynamic()   const { return simdInt16x8TypeDynamic;   }
        DynamicType * GetSIMDInt32x4TypeDynamic()   const { return simdInt32x4TypeDynamic;   }
        DynamicType * GetSIMDUint8x16TypeDynamic()  const { return simdUint8x16TypeDynamic;  }
        DynamicType * GetSIMDUint16x8TypeDynamic()  const { return simdUint16x8TypeDynamic;  }
        DynamicType * GetSIMDUint32x4TypeDynamic()  const { return simdUint32x4TypeDynamic;  }
        DynamicType * GetSIMDFloat32x4TypeDynamic() const { return simdFloat32x4TypeDynamic; }

        StaticType* GetSIMDFloat32x4TypeStatic() const { return simdFloat32x4TypeStatic; }
        StaticType* GetSIMDFloat64x2TypeStatic() const { return simdFloat64x2TypeStatic; }
        StaticType* GetSIMDInt32x4TypeStatic()   const { return simdInt32x4TypeStatic; }
        StaticType* GetSIMDInt16x8TypeStatic()   const { return simdInt16x8TypeStatic; }
        StaticType* GetSIMDInt8x16TypeStatic()   const { return simdInt8x16TypeStatic; }
        StaticType* GetSIMDBool32x4TypeStatic() const { return simdBool32x4TypeStatic; }
        StaticType* GetSIMDBool16x8TypeStatic() const { return simdBool16x8TypeStatic; }
        StaticType* GetSIMDBool8x16TypeStatic() const { return simdBool8x16TypeStatic; }
        StaticType* GetSIMDUInt32x4TypeStatic()   const { return simdUint32x4TypeStatic; }
        StaticType* GetSIMDUint16x8TypeStatic()   const { return simdUint16x8TypeStatic; }
        StaticType* GetSIMDUint8x16TypeStatic()   const { return simdUint8x16TypeStatic; }
#endif // #ifdef ENABLE_SIMDJS

        DynamicType * GetObjectLiteralType(uint16 requestedInlineSlotCapacity);
        DynamicType * GetObjectHeaderInlinedLiteralType(uint16 requestedInlineSlotCapacity);
        DynamicType * GetObjectType() const { return objectTypes[0]; }
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

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        JavascriptFunction* GetSIMDFloat32x4ToStringFunction() const { return simdFloat32x4ToStringFunction;  }
        JavascriptFunction* GetSIMDFloat64x2ToStringFunction() const { return simdFloat64x2ToStringFunction; }
        JavascriptFunction* GetSIMDInt32x4ToStringFunction()   const { return simdInt32x4ToStringFunction; }
        JavascriptFunction* GetSIMDInt16x8ToStringFunction()   const { return simdInt16x8ToStringFunction; }
        JavascriptFunction* GetSIMDInt8x16ToStringFunction()   const { return simdInt8x16ToStringFunction; }
        JavascriptFunction* GetSIMDBool32x4ToStringFunction()   const { return simdBool32x4ToStringFunction; }
        JavascriptFunction* GetSIMDBool16x8ToStringFunction()   const { return simdBool16x8ToStringFunction; }
        JavascriptFunction* GetSIMDBool8x16ToStringFunction()   const { return simdBool8x16ToStringFunction; }
        JavascriptFunction* GetSIMDUint32x4ToStringFunction()   const { return simdUint32x4ToStringFunction; }
        JavascriptFunction* GetSIMDUint16x8ToStringFunction()   const { return simdUint16x8ToStringFunction; }
        JavascriptFunction* GetSIMDUint8x16ToStringFunction()   const { return simdUint8x16ToStringFunction; }
#endif

        JavascriptFunction* GetDebugObjectNonUserGetterFunction() const { return debugObjectNonUserGetterFunction; }
        JavascriptFunction* GetDebugObjectNonUserSetterFunction() const { return debugObjectNonUserSetterFunction; }

        UnifiedRegex::RegexPattern * GetEmptyRegexPattern() const { return emptyRegexPattern; }
        JavascriptFunction* GetRegexExecFunction() const { return regexExecFunction; }
        JavascriptFunction* GetRegexFlagsGetterFunction() const { return regexFlagsGetterFunction; }
        JavascriptFunction* GetRegexGlobalGetterFunction() const { return regexGlobalGetterFunction; }
        JavascriptFunction* GetRegexStickyGetterFunction() const { return regexStickyGetterFunction; }
        JavascriptFunction* GetRegexUnicodeGetterFunction() const { return regexUnicodeGetterFunction; }

        int GetRegexConstructorSlotIndex() const { return regexConstructorSlotIndex;  }
        int GetRegexExecSlotIndex() const { return regexExecSlotIndex;  }
        int GetRegexFlagsGetterSlotIndex() const { return regexFlagsGetterSlotIndex;  }
        int GetRegexGlobalGetterSlotIndex() const { return regexGlobalGetterSlotIndex;  }
        int GetRegexStickyGetterSlotIndex() const { return regexStickyGetterSlotIndex;  }
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
        class WebAssemblyArrayBuffer* CreateWebAssemblyArrayBuffer(uint32 length);
        class WebAssemblyArrayBuffer* CreateWebAssemblyArrayBuffer(byte* buffer, uint32 length);
        SharedArrayBuffer* CreateSharedArrayBuffer(uint32 length);
        SharedArrayBuffer* CreateSharedArrayBuffer(SharedContents *contents);
        ArrayBuffer* CreateProjectionArraybuffer(uint32 length);
        ArrayBuffer* CreateProjectionArraybuffer(byte* buffer, uint32 length);
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
        static DynamicTypeHandler * GetDeferredAnonymousPrototypeFunctionTypeHandler();
        static DynamicTypeHandler * GetDeferredAnonymousPrototypeGeneratorFunctionTypeHandler();
        static DynamicTypeHandler * GetDeferredAnonymousPrototypeAsyncFunctionTypeHandler();

        DynamicTypeHandler * GetDeferredFunctionTypeHandler();
        DynamicTypeHandler * ScriptFunctionTypeHandler(bool noPrototypeProperty, bool isAnonymousFunction);
        DynamicTypeHandler * GetDeferredAnonymousFunctionTypeHandler();
        template<bool isNameAvailable, bool isPrototypeAvailable = true>
        static DynamicTypeHandler * GetDeferredFunctionTypeHandlerBase();
        template<bool isNameAvailable, bool isPrototypeAvailable = true>
        static DynamicTypeHandler * GetDeferredGeneratorFunctionTypeHandlerBase();
        template<bool isNameAvailable>
        static DynamicTypeHandler * GetDeferredAsyncFunctionTypeHandlerBase();

        DynamicType * CreateDeferredPrototypeFunctionType(JavascriptMethod entrypoint);
        DynamicType * CreateDeferredPrototypeFunctionTypeNoProfileThunk(JavascriptMethod entrypoint, bool isShared = false);
        DynamicType * CreateFunctionType(JavascriptMethod entrypoint, RecyclableObject* prototype = nullptr);
        DynamicType * CreateFunctionWithLengthType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndNameType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndPrototypeType(FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthType(DynamicObject * prototype, FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndNameType(DynamicObject * prototype, FunctionInfo * functionInfo);
        DynamicType * CreateFunctionWithLengthAndPrototypeType(DynamicObject * prototype, FunctionInfo * functionInfo);
        ScriptFunction * CreateScriptFunction(FunctionProxy* proxy);
        AsmJsScriptFunction * CreateAsmJsScriptFunction(FunctionProxy* proxy);
        ScriptFunctionWithInlineCache * CreateScriptFunctionWithInlineCache(FunctionProxy* proxy);
        GeneratorVirtualScriptFunction * CreateGeneratorVirtualScriptFunction(FunctionProxy* proxy);
        DynamicType * CreateGeneratorType(RecyclableObject* prototype);

#if 0
        JavascriptNumber* CreateNumber(double value);
#endif
        JavascriptNumber* CreateNumber(double value, RecyclerJavascriptNumberAllocator * numberAllocator);
        JavascriptGeneratorFunction* CreateGeneratorFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction);
        JavascriptAsyncFunction* CreateAsyncFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction);
        JavascriptExternalFunction* CreateExternalFunction(ExternalMethod entryPointer, PropertyId nameId, Var signature, JavascriptTypeId prototypeTypeId, UINT64 flags);
        JavascriptExternalFunction* CreateExternalFunction(ExternalMethod entryPointer, Var nameId, Var signature, JavascriptTypeId prototypeTypeId, UINT64 flags);
        JavascriptExternalFunction* CreateStdCallExternalFunction(StdCallJavascriptMethod entryPointer, PropertyId nameId, void *callbackState);
        JavascriptExternalFunction* CreateStdCallExternalFunction(StdCallJavascriptMethod entryPointer, Var name, void *callbackState);
        JavascriptPromiseAsyncSpawnExecutorFunction* CreatePromiseAsyncSpawnExecutorFunction(JavascriptMethod entryPoint, JavascriptGenerator* generator, Var target);
        JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction* CreatePromiseAsyncSpawnStepArgumentExecutorFunction(JavascriptMethod entryPoint, JavascriptGenerator* generator, Var argument, Var resolve = nullptr, Var reject = nullptr, bool isReject = false);
        JavascriptPromiseCapabilitiesExecutorFunction* CreatePromiseCapabilitiesExecutorFunction(JavascriptMethod entryPoint, JavascriptPromiseCapability* capability);
        JavascriptPromiseResolveOrRejectFunction* CreatePromiseResolveOrRejectFunction(JavascriptMethod entryPoint, JavascriptPromise* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord);
        JavascriptPromiseReactionTaskFunction* CreatePromiseReactionTaskFunction(JavascriptMethod entryPoint, JavascriptPromiseReaction* reaction, Var argument);
        JavascriptPromiseResolveThenableTaskFunction* CreatePromiseResolveThenableTaskFunction(JavascriptMethod entryPoint, JavascriptPromise* promise, RecyclableObject* thenable, RecyclableObject* thenFunction);
        JavascriptPromiseAllResolveElementFunction* CreatePromiseAllResolveElementFunction(JavascriptMethod entryPoint, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElements);
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
#ifdef ENABLE_SIMDJS
        JavascriptSIMDObject* CreateSIMDObject(Var simdValue, TypeId typeDescriptor);
#endif
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
        JavascriptFunction* EnsureGeneratorNextFunction();
        JavascriptFunction* EnsureGeneratorThrowFunction();
        JavascriptFunction* EnsureArrayPrototypeForEachFunction();
        JavascriptFunction* EnsureArrayPrototypeKeysFunction();
        JavascriptFunction* EnsureArrayPrototypeEntriesFunction();
        JavascriptFunction* EnsureArrayPrototypeValuesFunction();
        JavascriptFunction* EnsureJSONStringifyFunction();
        JavascriptFunction* EnsureObjectFreezeFunction();

        void SetCrossSiteForSharedFunctionType(JavascriptFunction * function);

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

        void EnsureStringTemplateCallsiteObjectList();
        void AddStringTemplateCallsiteObject(RecyclableObject* callsite);
        RecyclableObject* TryGetStringTemplateCallsiteObject(ParseNodePtr pnode);
        RecyclableObject* TryGetStringTemplateCallsiteObject(RecyclableObject* callsite);

        static void CheckAndInvalidateIsConcatSpreadableCache(PropertyId propertyId, ScriptContext *scriptContext);

#if DBG_DUMP
        static const char16* GetStringTemplateCallsiteObjectKey(Var callsite);
#endif

        Field(JavascriptFunction*)* GetBuiltinFunctions();
        INT_PTR* GetVTableAddresses();
        static BuiltinFunction GetBuiltinFunctionForPropId(PropertyId id);
        static BuiltinFunction GetBuiltInForFuncInfo(intptr_t funcInfoAddr, ThreadContextInfo *context);
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
        PropertyStringCacheMap* GetPropertyStringMap() { return this->propertyStringMap; }

        void TypeAndPrototypesAreEnsuredToHaveOnlyWritableDataProperties(Type *const type);
        void NoPrototypeChainsAreEnsuredToHaveOnlyWritableDataProperties();

        static bool ArrayIteratorPrototypeHasUserDefinedNext(ScriptContext *scriptContext);

        CharStringCache& GetCharStringCache() { return charStringCache;  }
        static JavascriptLibrary * FromCharStringCache(CharStringCache * cache)
        {
            return (JavascriptLibrary *)((uintptr_t)cache - offsetof(JavascriptLibrary, charStringCache));
        }

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

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        static bool __cdecl InitializeSIMDObject(DynamicObject* simdObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDOpCodeMaps();

        template<typename SIMDTypeName>
        static void SIMDPrototypeInitHelper(DynamicObject* simdPrototype, JavascriptLibrary* library, JavascriptFunction* constructorFn, JavascriptString* strLiteral);

        static bool __cdecl InitializeSIMDBool8x16Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDBool16x8Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDBool32x4Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDInt8x16Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDInt16x8Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDInt32x4Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDUint8x16Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDUint16x8Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDUint32x4Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDFloat32x4Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        static bool __cdecl InitializeSIMDFloat64x2Prototype(DynamicObject* simdPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
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
        RuntimeFunction* DefaultCreateFunction(FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType, PropertyId nameId);
        RuntimeFunction* DefaultCreateFunction(FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType, Var nameId);
        JavascriptFunction* AddFunction(DynamicObject* object, PropertyId propertyId, RuntimeFunction* function);
        void AddMember(DynamicObject* object, PropertyId propertyId, Var value);
        void AddMember(DynamicObject* object, PropertyId propertyId, Var value, PropertyAttributes attributes);
        JavascriptString* CreateEmptyString();


        static bool __cdecl InitializeGeneratorFunction(DynamicObject* function, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);
        template<bool addPrototype>
        static bool __cdecl InitializeFunction(DynamicObject* function, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

        static size_t const LibraryFunctionArgC[BuiltinFunction::Count + 1];
        static int const LibraryFunctionFlags[BuiltinFunction::Count + 1];   // returns enum BuiltInFlags.
#if ENABLE_DEBUG_CONFIG_OPTIONS
        static char16 const * const LibraryFunctionName[BuiltinFunction::Count + 1];
#endif

    public:
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
#ifdef ENABLE_SIMDJS
        HRESULT ProfilerRegisterSIMD();
#endif
        HRESULT ProfilerRegisterAtomics();

#ifdef IR_VIEWER
        HRESULT ProfilerRegisterIRViewer();
#endif /* IR_VIEWER */
    };
}
