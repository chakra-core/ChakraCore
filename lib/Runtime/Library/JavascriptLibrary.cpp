//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#include "Library/JSON/JSON.h"
#include "Types/MissingPropertyTypeHandler.h"
#include "Types/NullTypeHandler.h"
#include "Types/SimpleTypeHandler.h"
#include "Types/DeferredTypeHandler.h"
#include "Types/PathTypeHandler.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/SimpleDictionaryPropertyDescriptor.h"
#include "Types/SimpleDictionaryTypeHandler.h"

#include "Library/ForInObjectEnumerator.h"
#include "Library/EngineInterfaceObject.h"
#include "Library/IntlEngineInterfaceExtensionObject.h"
#ifdef ENABLE_JS_BUILTINS
#include "Library/JsBuiltInEngineInterfaceExtensionObject.h"
#endif
#include "Library/ThrowErrorObject.h"
#include "Library/Functions/StackScriptFunction.h"

namespace Js
{
    SimplePropertyDescriptor const JavascriptLibrary::SharedFunctionPropertyDescriptors[2] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::name), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::SharedIdMappedFunctionPropertyDescriptors[2] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyNone),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::name), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::FunctionWithLengthAndNameTypeDescriptors[2] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::name), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::FunctionWithPrototypeLengthAndNameTypeDescriptors[3] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::name), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::FunctionWithPrototypeAndLengthTypeDescriptors[2] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::FunctionWithNonWritablePrototypeAndLengthTypeDescriptors[2] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyNone),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable),
    };

    SimplePropertyDescriptor const JavascriptLibrary::FunctionWithNonWritablePrototypeLengthAndNameTypeDescriptors[3] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyNone),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::name), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::ModuleNamespaceTypeDescriptors[1] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::_symbolToStringTag), PropertyNone)
    };

    SimpleTypeHandler<1> JavascriptLibrary::SharedPrototypeTypeHandler(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::constructor), PropertyWritable | PropertyConfigurable, PropertyTypesWritableDataOnly, 4, sizeof(DynamicObject));
    SimpleTypeHandler<1> JavascriptLibrary::SharedFunctionWithoutPrototypeTypeHandler(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::name), PropertyConfigurable);
    SimpleTypeHandler<1> JavascriptLibrary::SharedFunctionWithPrototypeTypeHandlerV11(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyWritable);
    SimpleTypeHandler<2> JavascriptLibrary::SharedFunctionWithPrototypeTypeHandler(NO_WRITE_BARRIER_TAG(SharedFunctionPropertyDescriptors));
    SimpleTypeHandler<2> JavascriptLibrary::SharedIdMappedFunctionWithPrototypeTypeHandler(NO_WRITE_BARRIER_TAG(SharedIdMappedFunctionPropertyDescriptors));
    SimpleTypeHandler<1> JavascriptLibrary::SharedFunctionWithConfigurableLengthTypeHandler(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable);
    SimpleTypeHandler<1> JavascriptLibrary::SharedFunctionWithLengthTypeHandler(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length));
    SimpleTypeHandler<2> JavascriptLibrary::SharedFunctionWithLengthAndNameTypeHandler(NO_WRITE_BARRIER_TAG(FunctionWithLengthAndNameTypeDescriptors));
    SimpleTypeHandler<3> JavascriptLibrary::SharedFunctionWithPrototypeLengthAndNameTypeHandler(NO_WRITE_BARRIER_TAG(FunctionWithPrototypeLengthAndNameTypeDescriptors));
    SimpleTypeHandler<2> JavascriptLibrary::SharedFunctionWithPrototypeAndLengthTypeHandler(NO_WRITE_BARRIER_TAG(FunctionWithPrototypeAndLengthTypeDescriptors));
    SimpleTypeHandler<1> JavascriptLibrary::SharedNamespaceSymbolTypeHandler(NO_WRITE_BARRIER_TAG(ModuleNamespaceTypeDescriptors), PropertyTypesHasSpecialProperties);
    SimpleTypeHandler<2> JavascriptLibrary::SharedFunctionWithNonWritablePrototypeAndLengthTypeHandler(NO_WRITE_BARRIER_TAG(FunctionWithNonWritablePrototypeAndLengthTypeDescriptors));
    SimpleTypeHandler<3> JavascriptLibrary::SharedFunctionWithNonWritablePrototypeLengthAndNameTypeHandler(NO_WRITE_BARRIER_TAG(FunctionWithNonWritablePrototypeLengthAndNameTypeDescriptors));
    MissingPropertyTypeHandler JavascriptLibrary::MissingPropertyHolderTypeHandler;

    SimplePropertyDescriptor const JavascriptLibrary::HeapArgumentsPropertyDescriptors[3] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable | PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::callee), PropertyConfigurable | PropertyWritable),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::_symbolIterator), PropertyConfigurable | PropertyWritable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::FunctionWithLengthAndPrototypeTypeDescriptors[2] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::prototype), PropertyNone),
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::length), PropertyConfigurable)
    };

    SimplePropertyDescriptor const JavascriptLibrary::ClassPrototypePropertyDescriptors[1] =
    {
        SimplePropertyDescriptor(NO_WRITE_BARRIER_TAG(BuiltInPropertyRecords::constructor), PropertyConfigurable | PropertyWritable)
    };

    void JavascriptLibrary::Initialize(ScriptContext* scriptContext, GlobalObject * globalObject)
    {
        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
#ifdef PROFILE_EXEC
        scriptContext->ProfileBegin(Js::LibInitPhase);
#endif
        this->scriptContext = scriptContext;
        this->recycler = scriptContext->GetRecycler();
        this->undeclBlockVarSentinel = RecyclerNew(recycler, UndeclaredBlockVariable, StaticType::New(scriptContext, TypeIds_Null, nullptr, nullptr));

        this->typesWithOnlyWritablePropertyProtoChain.Initialize(scriptContext->GetOnlyWritablePropertyRegistry());
        this->typesWithNoSpecialPropertyProtoChain.Initialize(scriptContext->GetNoSpecialPropertyRegistry());

        // Library is not zero-initialized. memset the memory occupied by builtinFunctions array to 0.
        ClearArray(builtinFunctions, BuiltinFunction::Count);

        // Note: InitializePrototypes and InitializeTypes must be called first.
        InitializePrototypes();
        InitializeTypes();
        InitializeGlobal(globalObject);
        InitializeComplexThings();
        InitializeStaticValues();
        PrecalculateArrayAllocationBuckets();

        this->cache.toStringTagCache = ScriptContextPolymorphicInlineCache::New(32, this);

#if ENABLE_COPYONACCESS_ARRAY
        if (!PHASE_OFF1(CopyOnAccessArrayPhase))
        {
            this->cacheForCopyOnAccessArraySegments = RecyclerNewZ(this->recycler, CacheForCopyOnAccessArraySegments);
        }
#endif
#ifdef PROFILE_EXEC
        scriptContext->ProfileEnd(Js::LibInitPhase);
#endif
    }

    void JavascriptLibrary::Uninitialize()
    {
#if DBG
        if (moduleRecordList != nullptr)
        {
            // This should mostly be a no-op except for error cases where we may not have had a chance to cleaup a module record yet.
            // Do this only in debug builds to avoid reporting of a memory leak. In release builds this doesn't matter as we will cleanup anyways.
            for (int index = 0; index < moduleRecordList->Count(); index++)
            {
                moduleRecordList->Item(index)->ReleaseParserResources();
            }
        }
#endif
        this->globalObject = nullptr;
    }

    void JavascriptLibrary::InitializePrototypes()
    {
        Recycler* recycler = this->GetRecycler();

        // Recycler macros below expect that their arguments will not throw when they're evaluated.
        // We allocate a lot of types for the built-in prototype objects which we need to store temporarily.
        DynamicType* tempDynamicType = nullptr;
        Type* tempType = StaticType::New(scriptContext, TypeIds_Null, nullptr, nullptr);
        nullValue = RecyclerNew(recycler, RecyclableObject, tempType);
        nullValue->GetType()->SetHasSpecialPrototype(true);
        this->rootPath = TypePath::New(recycler);

        // The prototype property of the object prototype is null.
        objectPrototype = ObjectPrototypeObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, nullValue, nullptr,
            DeferredTypeHandler<InitializeObjectPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        constructorPrototypeObjectType = DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            &SharedPrototypeTypeHandler, true, true);

        constructorPrototypeObjectType->SetHasNoEnumerableProperties(true);

        arrayBufferPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeArrayBufferPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        if (scriptContext->GetConfig()->IsESSharedArrayBufferEnabled())
        {
            sharedArrayBufferPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeSharedArrayBufferPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }
        else
        {
            sharedArrayBufferPrototype = nullptr;
        }

        dataViewPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeDataViewPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        typedArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeTypedArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Int8ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeInt8ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Uint8ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeUint8ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        // If ES6 TypedArrays are enabled, we have Khronos Interop mode enabled
        Uint8ClampedArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeUint8ClampedArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Int16ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeInt16ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Uint16ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeUint16ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Int32ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeInt32ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Uint32ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeUint32ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Float32ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeFloat32ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Float64ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeFloat64ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Int64ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeInt64ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        Uint64ArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeUint64ArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        BoolArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeBoolArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        CharArrayPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, typedArrayPrototype, nullptr,
                DeferredTypeHandler<InitializeCharArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        arrayPrototype = JavascriptArray::New<Var, JavascriptArray, 0>(0,
            DynamicType::New(scriptContext, TypeIds_Array, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeArrayPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()), recycler);

        tempDynamicType = DynamicType::New(scriptContext, TypeIds_BooleanObject, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeBooleanPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
        booleanPrototype = RecyclerNew(recycler, JavascriptBooleanObject, nullptr, tempDynamicType);

        tempDynamicType = DynamicType::New(scriptContext, TypeIds_NumberObject, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeNumberPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
        numberPrototype = RecyclerNew(recycler, JavascriptNumberObject, TaggedInt::ToVarUnchecked(0), tempDynamicType);

        bigintPrototype = nullptr;
        if (scriptContext->GetConfig()->IsESBigIntEnabled())
        {
            tempDynamicType = DynamicType::New(scriptContext, TypeIds_BigIntObject, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeBigIntPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
            bigintPrototype = RecyclerNew(recycler, JavascriptBigIntObject, nullptr, tempDynamicType);
        }

        tempDynamicType = DynamicType::New(scriptContext, TypeIds_StringObject, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeStringPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
        stringPrototype = RecyclerNew(recycler, JavascriptStringObject, nullptr, tempDynamicType);

        if (scriptContext->GetConfig()->IsES6PrototypeChain())
        {
            datePrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeDatePrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            regexPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeRegexPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            errorPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeErrorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }
        else
        {
            double initDateValue = JavascriptNumber::NaN;

            tempDynamicType = DynamicType::New(scriptContext, TypeIds_Date, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeDatePrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
            datePrototype = RecyclerNewZ(recycler, JavascriptDate, initDateValue, tempDynamicType);

            tempDynamicType = DynamicType::New(scriptContext, TypeIds_Error, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeErrorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
            errorPrototype = RecyclerNew(this->GetRecycler(), JavascriptError, tempDynamicType, /*isExternalError*/FALSE, /*isPrototype*/TRUE);
        }

#define INIT_ERROR_PROTO(field, initFunc) \
        if (scriptContext->GetConfig()->IsES6PrototypeChain()) \
        { \
            field = DynamicObject::New(recycler, \
                DynamicType::New(scriptContext, TypeIds_Object, errorPrototype, nullptr, \
                DeferredTypeHandler<initFunc, DefaultDeferredTypeFilter, true>::GetDefaultInstance())); \
        } \
        else \
        { \
            tempDynamicType = DynamicType::New(scriptContext, TypeIds_Error, errorPrototype, nullptr, \
                DeferredTypeHandler<initFunc, DefaultDeferredTypeFilter, true>::GetDefaultInstance()); \
            field = RecyclerNew(this->GetRecycler(), JavascriptError, tempDynamicType, /*isExternalError*/FALSE, /*isPrototype*/TRUE); \
        }

        INIT_ERROR_PROTO(evalErrorPrototype, InitializeEvalErrorPrototype);
        INIT_ERROR_PROTO(rangeErrorPrototype, InitializeRangeErrorPrototype);
        INIT_ERROR_PROTO(referenceErrorPrototype, InitializeReferenceErrorPrototype);
        INIT_ERROR_PROTO(syntaxErrorPrototype, InitializeSyntaxErrorPrototype);
        INIT_ERROR_PROTO(typeErrorPrototype, InitializeTypeErrorPrototype);
        INIT_ERROR_PROTO(uriErrorPrototype, InitializeURIErrorPrototype);
        INIT_ERROR_PROTO(aggregateErrorPrototype, InitializeAggregateErrorPrototype);

#ifdef ENABLE_WASM
        if (CONFIG_FLAG(Wasm) && PHASE_ENABLED1(WasmPhase))
        {
            INIT_ERROR_PROTO(webAssemblyCompileErrorPrototype, InitializeWebAssemblyCompileErrorPrototype);
            INIT_ERROR_PROTO(webAssemblyRuntimeErrorPrototype, InitializeWebAssemblyRuntimeErrorPrototype);
            INIT_ERROR_PROTO(webAssemblyLinkErrorPrototype, InitializeWebAssemblyLinkErrorPrototype);
        }
#endif

#undef INIT_ERROR_PROTO

        tempDynamicType = DynamicType::New(scriptContext, TypeIds_Function, objectPrototype, JavascriptFunction::PrototypeEntryPoint,
            DeferredTypeHandler<InitializeFunctionPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
        functionPrototype = RecyclerNew(recycler, JavascriptFunction, tempDynamicType, &JavascriptFunction::EntryInfo::PrototypeEntryPoint);

        promisePrototype = nullptr;
        generatorFunctionPrototype = nullptr;
        generatorPrototype = nullptr;
        asyncFunctionPrototype = nullptr;
        asyncGeneratorPrototype = nullptr;
        asyncGeneratorFunctionPrototype = nullptr;

        symbolPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeSymbolPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        mapPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeMapPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        setPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeSetPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        weakMapPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeWeakMapPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        weakSetPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeWeakSetPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        if (scriptContext->GetConfig()->IsES2018AsyncIterationEnabled())
        {
            asyncIteratorPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeAsyncIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }

        iteratorPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        if (!scriptContext->IsJsBuiltInEnabled())
        {
            arrayIteratorPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, iteratorPrototype, nullptr,
                    DeferredTypeHandler<InitializeArrayIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }

        mapIteratorPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, iteratorPrototype, nullptr,
            DeferredTypeHandler<InitializeMapIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        setIteratorPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, iteratorPrototype, nullptr,
            DeferredTypeHandler<InitializeSetIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        stringIteratorPrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, iteratorPrototype, nullptr,
            DeferredTypeHandler<InitializeStringIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

#ifdef ENABLE_WASM
        if (CONFIG_FLAG(Wasm) && PHASE_ENABLED1(WasmPhase))
        {
            webAssemblyMemoryPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeWebAssemblyMemoryPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            webAssemblyModulePrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeWebAssemblyModulePrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            webAssemblyInstancePrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeWebAssemblyInstancePrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            webAssemblyTablePrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeWebAssemblyTablePrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        }
#endif

        promisePrototype = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializePromisePrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

        if(scriptContext->GetConfig()->IsES6GeneratorsEnabled())
        {
            generatorFunctionPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, functionPrototype, nullptr,
                DeferredTypeHandler<InitializeGeneratorFunctionPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            generatorPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, iteratorPrototype, nullptr,
                DeferredTypeHandler<InitializeGeneratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }

        if(scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            asyncFunctionPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, functionPrototype, nullptr,
                DeferredTypeHandler<InitializeAsyncFunctionPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }

        if (scriptContext->GetConfig()->IsES2018AsyncIterationEnabled())
        {
            asyncGeneratorFunctionPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, functionPrototype, nullptr,
                DeferredTypeHandler<InitializeAsyncGeneratorFunctionPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            asyncGeneratorPrototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, asyncIteratorPrototype, nullptr,
                DeferredTypeHandler<InitializeAsyncGeneratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));

            asyncFromSyncIteratorProtototype = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, asyncIteratorPrototype, nullptr,
                DeferredTypeHandler<InitializeAsyncFromSyncIteratorPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance())); 
        }
    }

    void JavascriptLibrary::InitializeTypes()
    {
        Recycler* recycler = this->GetRecycler();
        ScriptConfiguration const *config = scriptContext->GetConfig();

        enumeratorType = StaticType::New(scriptContext, TypeIds_Enumerator, objectPrototype, nullptr);

        // Initialize Array/Argument types
        heapArgumentsType = DynamicType::New(scriptContext, TypeIds_Arguments, objectPrototype, nullptr,
            SimpleDictionaryTypeHandler::New(scriptContext, HeapArgumentsPropertyDescriptors, _countof(HeapArgumentsPropertyDescriptors), 0, 0, true, true), true, true);

        classPrototypeTypeHandler = 
#if ENABLE_FIXED_FIELDS
            SimpleDictionaryTypeHandler::NewInitialized
#else
            SimpleDictionaryTypeHandler::New
#endif
                (scriptContext, ClassPrototypePropertyDescriptors, _countof(ClassPrototypePropertyDescriptors), 0, 0, true, true);

        TypePath *const strictHeapArgumentsTypePath = TypePath::New(recycler);
        strictHeapArgumentsTypePath->Add(BuiltInPropertyRecords::callee);
        strictHeapArgumentsTypePath->Add<true /*isSetter*/>(BuiltInPropertyRecords::callee);
        strictHeapArgumentsTypePath->Add(BuiltInPropertyRecords::length);
        strictHeapArgumentsTypePath->Add(BuiltInPropertyRecords::_symbolIterator);
        uint8 strictHeapArgumentsTypePathSize = strictHeapArgumentsTypePath->GetPathSize();
        AnalysisAssert(strictHeapArgumentsTypePathSize >= 4);
        ObjectSlotAttributes *strictHeapArgumentsAttributes = RecyclerNewArrayLeaf(recycler, ObjectSlotAttributes, strictHeapArgumentsTypePathSize);
        strictHeapArgumentsAttributes[0] = (ObjectSlotAttributes)(ObjectSlotAttr_Writable | ObjectSlotAttr_Accessor);
        strictHeapArgumentsAttributes[1] = ObjectSlotAttr_Setter;
        strictHeapArgumentsAttributes[2] = (ObjectSlotAttributes)PropertyBuiltInMethodDefaults;
        strictHeapArgumentsAttributes[3] = (ObjectSlotAttributes)PropertyBuiltInMethodDefaults;
        for (int i = 4; i < strictHeapArgumentsTypePathSize; ++i)
        {
            strictHeapArgumentsAttributes[i] = ObjectSlotAttr_Default;
        }
        PathTypeSetterSlotIndex * strictHeapArgumentsSetters = RecyclerNewArrayLeaf(recycler, PathTypeSetterSlotIndex, strictHeapArgumentsTypePathSize);
        strictHeapArgumentsSetters[0] = 1;
        for (int i = 1; i < strictHeapArgumentsTypePathSize; ++i)
        {
            strictHeapArgumentsSetters[i] = NoSetterSlot;
        }
        strictHeapArgumentsType = DynamicType::New(
            scriptContext,
            TypeIds_Arguments,
            objectPrototype,
            nullptr,
            PathTypeHandlerWithAttr::New(
                scriptContext,
                strictHeapArgumentsTypePath,
                strictHeapArgumentsAttributes,
                strictHeapArgumentsSetters,
                1 /*setterCount*/,
                strictHeapArgumentsTypePath->GetPathLength(),
                strictHeapArgumentsTypePath->GetPathLength() /*inlineSlotCapacity*/,
                sizeof(HeapArgumentsObject) /*offsetOfInlineSlots*/,
                true /*isLocked*/,
                true /*isShared*/),
            true /*isLocked*/,
            true /*isShared*/);

#define INIT_SIMPLE_TYPE(field, typeId, prototype) \
        field = DynamicType::New(scriptContext, typeId, prototype, nullptr, \
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true)

        INIT_SIMPLE_TYPE(activationObjectType, TypeIds_ActivationObject, nullValue);
        INIT_SIMPLE_TYPE(arrayType, TypeIds_Array, arrayPrototype);
        INIT_SIMPLE_TYPE(nativeIntArrayType, TypeIds_NativeIntArray, arrayPrototype);
        INIT_SIMPLE_TYPE(nativeFloatArrayType, TypeIds_NativeFloatArray, arrayPrototype);
        INIT_SIMPLE_TYPE(arrayBufferType, TypeIds_ArrayBuffer, arrayBufferPrototype);

#if ENABLE_COPYONACCESS_ARRAY
        INIT_SIMPLE_TYPE(copyOnAccessNativeIntArrayType, TypeIds_CopyOnAccessNativeIntArray, arrayPrototype);
#endif

        if (scriptContext->GetConfig()->IsESSharedArrayBufferEnabled())
        {
            INIT_SIMPLE_TYPE(sharedArrayBufferType, TypeIds_SharedArrayBuffer, sharedArrayBufferPrototype);
        }
        else
        {
            sharedArrayBufferType = nullptr;
        }

        INIT_SIMPLE_TYPE(dataViewType, TypeIds_DataView, dataViewPrototype);
        INIT_SIMPLE_TYPE(int8ArrayType, TypeIds_Int8Array, Int8ArrayPrototype);
        INIT_SIMPLE_TYPE(uint8ArrayType, TypeIds_Uint8Array, Uint8ArrayPrototype);
        INIT_SIMPLE_TYPE(uint8ClampedArrayType, TypeIds_Uint8ClampedArray, Uint8ClampedArrayPrototype);
        INIT_SIMPLE_TYPE(int16ArrayType, TypeIds_Int16Array, Int16ArrayPrototype);
        INIT_SIMPLE_TYPE(uint16ArrayType, TypeIds_Uint16Array, Uint16ArrayPrototype);
        INIT_SIMPLE_TYPE(int32ArrayType, TypeIds_Int32Array, Int32ArrayPrototype);
        INIT_SIMPLE_TYPE(uint32ArrayType, TypeIds_Uint32Array, Uint32ArrayPrototype);
        INIT_SIMPLE_TYPE(float32ArrayType, TypeIds_Float32Array, Float32ArrayPrototype);
        INIT_SIMPLE_TYPE(float64ArrayType, TypeIds_Float64Array, Float64ArrayPrototype);
        INIT_SIMPLE_TYPE(int64ArrayType, TypeIds_Int64Array, Int64ArrayPrototype);
        INIT_SIMPLE_TYPE(uint64ArrayType, TypeIds_Uint64Array, Uint64ArrayPrototype);
        INIT_SIMPLE_TYPE(boolArrayType, TypeIds_BoolArray, BoolArrayPrototype);
        INIT_SIMPLE_TYPE(charArrayType, TypeIds_CharArray, CharArrayPrototype);
        INIT_SIMPLE_TYPE(errorType, TypeIds_Error, errorPrototype);
        INIT_SIMPLE_TYPE(evalErrorType, TypeIds_Error, evalErrorPrototype);
        INIT_SIMPLE_TYPE(rangeErrorType, TypeIds_Error, rangeErrorPrototype);
        INIT_SIMPLE_TYPE(referenceErrorType, TypeIds_Error, referenceErrorPrototype);
        INIT_SIMPLE_TYPE(syntaxErrorType, TypeIds_Error, syntaxErrorPrototype);
        INIT_SIMPLE_TYPE(typeErrorType, TypeIds_Error, typeErrorPrototype);
        INIT_SIMPLE_TYPE(uriErrorType, TypeIds_Error, uriErrorPrototype);
        INIT_SIMPLE_TYPE(aggregateErrorType, TypeIds_Error, aggregateErrorPrototype);

#ifdef ENABLE_WASM
        if (CONFIG_FLAG(Wasm) && PHASE_ENABLED1(WasmPhase))
        {
            INIT_SIMPLE_TYPE(webAssemblyCompileErrorType, TypeIds_Error, webAssemblyCompileErrorPrototype);
            INIT_SIMPLE_TYPE(webAssemblyRuntimeErrorType, TypeIds_Error, webAssemblyRuntimeErrorPrototype);
            INIT_SIMPLE_TYPE(webAssemblyLinkErrorType, TypeIds_Error, webAssemblyLinkErrorPrototype);
        }
#endif

#undef INIT_SIMPLE_TYPE

        withType    = nullptr;
        proxyType   = nullptr;
        promiseType = nullptr;
        moduleNamespaceType = nullptr;

        if (config->IsESBigIntEnabled())
        {
            bigintTypeStatic = StaticType::New(scriptContext, TypeIds_BigInt, bigintPrototype, nullptr);
            bigintTypeDynamic = DynamicType::New(scriptContext, TypeIds_BigIntObject, bigintPrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
        }

        // Initialize boolean types
        booleanTypeStatic = StaticType::New(scriptContext, TypeIds_Boolean, booleanPrototype, nullptr);
        booleanTypeDynamic = DynamicType::New(scriptContext, TypeIds_BooleanObject, booleanPrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);

        // Initialize symbol types
        symbolTypeStatic = StaticType::New(scriptContext, TypeIds_Symbol, symbolPrototype, nullptr);
        symbolTypeDynamic = DynamicType::New(scriptContext, TypeIds_SymbolObject, symbolPrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);

        withType = StaticType::New(scriptContext, TypeIds_UnscopablesWrapperObject, GetNull(), nullptr);

        if (config->IsES6SpreadEnabled())
        {
            SpreadArgumentType = DynamicType::New(scriptContext, TypeIds_SpreadArgument, GetNull(), nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
        }

        if (config->IsES6ProxyEnabled())
        {
            // proxy's prototype is not actually used. once a proxy is used, the GetType()->GetPrototype() is not used in instanceOf style usage as they are trapped.
            // We can use GetType()->GetPrototype() in [[get]], [[set]], and [[hasProperty]] to force the prototype walk to stop at prototype so we don't need to
            // continue prototype chain walk after proxy.
            proxyType = DynamicType::New(scriptContext, TypeIds_Proxy, GetNull(), nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
        }

        promiseType = DynamicType::New(scriptContext, TypeIds_Promise, promisePrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);

        if (config->IsES6ModuleEnabled())
        {
            moduleNamespaceType = DynamicType::New(scriptContext, TypeIds_ModuleNamespace, nullValue, nullptr, &SharedNamespaceSymbolTypeHandler);
            moduleNamespaceType->ShareType();
        }

        // Initialize Date types
        dateType = DynamicType::New(scriptContext, TypeIds_Date, datePrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        //  Initialize function types

        anonymousFunctionTypeHandler = &SharedFunctionWithConfigurableLengthTypeHandler;
        anonymousFunctionWithPrototypeTypeHandler = &SharedFunctionWithPrototypeAndLengthTypeHandler;

        functionTypeHandler = &SharedFunctionWithoutPrototypeTypeHandler;
        functionTypeHandlerWithLength = &SharedFunctionWithLengthAndNameTypeHandler;
        functionWithPrototypeAndLengthTypeHandler = &SharedFunctionWithPrototypeLengthAndNameTypeHandler;
        functionWithPrototypeTypeHandler = &SharedFunctionWithPrototypeTypeHandler;
        functionWithPrototypeTypeHandler->SetHasKnownSlot0();

        externalFunctionWithDeferredPrototypeType = CreateDeferredPrototypeFunctionTypeNoProfileThunk(JavascriptExternalFunction::ExternalFunctionThunk, true /*isShared*/);
        externalFunctionWithLengthAndDeferredPrototypeType = CreateDeferredLengthPrototypeFunctionTypeNoProfileThunk(JavascriptExternalFunction::ExternalFunctionThunk, true /*isShared*/);
        wrappedFunctionWithDeferredPrototypeType = CreateDeferredPrototypeFunctionTypeNoProfileThunk(JavascriptExternalFunction::WrappedFunctionThunk, true /*isShared*/);
        stdCallFunctionWithDeferredPrototypeType = CreateDeferredPrototypeFunctionTypeNoProfileThunk(JavascriptExternalFunction::StdCallExternalFunctionThunk, true /*isShared*/);
        idMappedFunctionWithPrototypeType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, JavascriptExternalFunction::ExternalFunctionThunk,
            &SharedIdMappedFunctionWithPrototypeTypeHandler, true, true);
        externalConstructorFunctionWithDeferredPrototypeType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, JavascriptExternalFunction::ExternalFunctionThunk,
            Js::DeferredTypeHandler<Js::JavascriptExternalFunction::DeferredConstructorInitializer>::GetDefaultInstance(), true, true);
        defaultExternalConstructorFunctionWithDeferredPrototypeType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, JavascriptExternalFunction::DefaultExternalFunctionThunk,
            Js::DeferredTypeHandler<Js::JavascriptExternalFunction::DeferredConstructorInitializer>::GetDefaultInstance(), true, true);

        boundFunctionType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, BoundFunction::NewInstance,
            GetDeferredFunctionWithLengthUnsetTypeHandler(), true, true);
        crossSiteDeferredFunctionType = CreateDeferredFunctionTypeNoProfileThunk(
            scriptContext->CurrentCrossSiteThunk, true /*isShared*/);
        crossSiteDeferredPrototypeFunctionType = CreateDeferredPrototypeFunctionTypeNoProfileThunk(
            scriptContext->CurrentCrossSiteThunk, true /*isShared*/);
        crossSiteIdMappedFunctionWithPrototypeType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, scriptContext->CurrentCrossSiteThunk,
            &SharedIdMappedFunctionWithPrototypeTypeHandler, true, true);
        crossSiteExternalConstructFunctionWithPrototypeType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, scriptContext->CurrentCrossSiteThunk,
            Js::DeferredTypeHandler<Js::JavascriptExternalFunction::DeferredConstructorInitializer>::GetDefaultInstance(), true, true);

        // Initialize Number types
        numberTypeStatic = StaticType::New(scriptContext, TypeIds_Number, numberPrototype, nullptr);
        int64NumberTypeStatic = StaticType::New(scriptContext, TypeIds_Int64Number, numberPrototype, nullptr);
        uint64NumberTypeStatic = StaticType::New(scriptContext, TypeIds_UInt64Number, numberPrototype, nullptr);
        numberTypeDynamic = DynamicType::New(scriptContext, TypeIds_NumberObject, numberPrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);

#ifdef ENABLE_WASM
        if (CONFIG_FLAG(Wasm) && PHASE_ENABLED1(WasmPhase))
        {
            webAssemblyModuleType = DynamicType::New(scriptContext, TypeIds_WebAssemblyModule, webAssemblyModulePrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
            webAssemblyInstanceType = DynamicType::New(scriptContext, TypeIds_WebAssemblyInstance, webAssemblyInstancePrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
            webAssemblyMemoryType = DynamicType::New(scriptContext, TypeIds_WebAssemblyMemory, webAssemblyMemoryPrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
            webAssemblyTableType = DynamicType::New(scriptContext, TypeIds_WebAssemblyTable, webAssemblyTablePrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);
        }
#endif
        // Initialize Object types
        for (int16 i = 0; i < PreInitializedObjectTypeCount; i++)
        {
            PathTypeHandlerNoAttr * typeHandler =
                PathTypeHandlerNoAttr::New(
                    scriptContext,
                    this->GetRootPath(),
                    0,
                    i * InlineSlotCountIncrement,
                    sizeof(DynamicObject),
                    true,
                    true);
            typeHandler->SetIsInlineSlotCapacityLocked();
            objectTypes[i] = DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr, typeHandler, true, true);
        }
        for (int16 i = 0; i < PreInitializedObjectTypeCount; i++)
        {
            PathTypeHandlerNoAttr * typeHandler =
                PathTypeHandlerNoAttr::New(
                    scriptContext,
                    this->GetRootPath(),
                    0,
                    DynamicTypeHandler::GetObjectHeaderInlinableSlotCapacity() + i * InlineSlotCountIncrement,
                    DynamicTypeHandler::GetOffsetOfObjectHeaderInlineSlots(),
                    true,
                    true);
            typeHandler->SetIsInlineSlotCapacityLocked();
            objectHeaderInlinedTypes[i] =
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr, typeHandler, true, true);
        }

        PathTypeHandlerNoAttr * typeHandler = PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true);
        nullPrototypeObjectType = DynamicType::New(scriptContext, TypeIds_Object, nullValue, nullptr, typeHandler, true, true);

        // Initialize regex types
        TypePath *const regexResultPath = TypePath::New(recycler);
        regexResultPath->Add(BuiltInPropertyRecords::input);
        regexResultPath->Add(BuiltInPropertyRecords::index);
        regexResultType = DynamicType::New(scriptContext, TypeIds_Array, arrayPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, regexResultPath, regexResultPath->GetPathLength(), JavascriptRegularExpressionResult::InlineSlotCount, sizeof(JavascriptArray), true, true), true, true);

        // Initialize string types
        // static type is handled under StringCache.h
        stringTypeDynamic = DynamicType::New(scriptContext, TypeIds_StringObject, stringPrototype, nullptr, NullTypeHandler<false>::GetDefaultInstance(), true, true);

        // Initialize Throw error object type
        throwErrorObjectType = StaticType::New(scriptContext, TypeIds_Undefined, nullValue, ThrowErrorObject::DefaultEntryPoint);

        mapType = DynamicType::New(scriptContext, TypeIds_Map, mapPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        setType = DynamicType::New(scriptContext, TypeIds_Set, setPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        weakMapType = DynamicType::New(scriptContext, TypeIds_WeakMap, weakMapPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        weakSetType = DynamicType::New(scriptContext, TypeIds_WeakSet, weakSetPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        TypePath *const iteratorResultPath = TypePath::New(recycler);
        iteratorResultPath->Add(BuiltInPropertyRecords::value);
        iteratorResultPath->Add(BuiltInPropertyRecords::done);
        iteratorResultType = DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, iteratorResultPath, iteratorResultPath->GetPathLength(), 2, sizeof(DynamicObject), true, true), true, true);

        // Create await object type
        auto* awaitObjectPath = TypePath::New(recycler);
        awaitObjectPath->Add(BuiltInPropertyRecords::value);

        auto* awaitObjectHandler = PathTypeHandlerNoAttr::New(
            scriptContext,
            awaitObjectPath,
            awaitObjectPath->GetPathLength(),
            1,
            sizeof(DynamicObject),
            true,
            true);

        awaitObjectType = DynamicType::New(
            scriptContext,
            TypeIds_AwaitObject,
            objectPrototype,
            nullptr,
            awaitObjectHandler,
            true,
            true);

        // Create yield resume object type
        auto* resumeObjectPath = TypePath::New(recycler);
        resumeObjectPath->Add(BuiltInPropertyRecords::value);
        resumeObjectPath->Add(BuiltInPropertyRecords::kind);

        auto* resumeObjectHandler = PathTypeHandlerNoAttr::New(
            scriptContext,
            resumeObjectPath,
            resumeObjectPath->GetPathLength(),
            2,
            sizeof(DynamicObject),
            true,
            true);

        resumeYieldObjectType = DynamicType::New(
            scriptContext,
            TypeIds_Object,
            objectPrototype,
            nullptr,
            resumeObjectHandler,
            true,
            true);

        arrayIteratorType = DynamicType::New(scriptContext, TypeIds_ArrayIterator, arrayIteratorPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);
        mapIteratorType = DynamicType::New(scriptContext, TypeIds_MapIterator, mapIteratorPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);
        setIteratorType = DynamicType::New(scriptContext, TypeIds_SetIterator, setIteratorPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);
        stringIteratorType = DynamicType::New(scriptContext, TypeIds_StringIterator, stringIteratorPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);
        listIteratorType = DynamicType::New(scriptContext, TypeIds_ListIterator, iteratorPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        if (config->IsES6GeneratorsEnabled())
        {
            generatorConstructorPrototypeObjectType = DynamicType::New(scriptContext, TypeIds_Object, generatorPrototype, nullptr,
                NullTypeHandler<false>::GetDefaultInstance(), true, true);

            generatorConstructorPrototypeObjectType->SetHasNoEnumerableProperties(true);
        }

        if (config->IsES2018AsyncIterationEnabled())
        {
            asyncGeneratorConstructorPrototypeObjectType = DynamicType::New(scriptContext, TypeIds_Object, asyncGeneratorPrototype, nullptr,
                NullTypeHandler<false>::GetDefaultInstance(), true, true);

            asyncGeneratorConstructorPrototypeObjectType->SetHasNoEnumerableProperties(true);
        }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        debugDisposableObjectType = DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);

        debugFuncExecutorInDisposeObjectType = DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);
#endif
    }

    bool JavascriptLibrary::InitializeGeneratorFunction(DynamicObject *instance, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptGeneratorFunction *function = VarTo<JavascriptGeneratorFunction>(instance);
        bool isAnonymousFunction = function->IsAnonymousFunction();

        JavascriptLibrary* javascriptLibrary = function->GetType()->GetLibrary();
        typeHandler->ConvertFunction(function, isAnonymousFunction ? javascriptLibrary->anonymousFunctionWithPrototypeTypeHandler : javascriptLibrary->functionWithPrototypeAndLengthTypeHandler);
        function->SetPropertyWithAttributes(PropertyIds::prototype, javascriptLibrary->CreateGeneratorConstructorPrototypeObject(), PropertyWritable, nullptr);

        Var varLength;
        GeneratorVirtualScriptFunction* scriptFunction = function->GetGeneratorVirtualScriptFunction();
        if (!scriptFunction->GetProperty(scriptFunction, PropertyIds::length, &varLength, nullptr, scriptFunction->GetScriptContext()))
        {
            // TODO - remove this if or convert it to a FailFast if this assert never triggers
            // Nothing in the ChakraCore CI will reach this code
            AssertMsg(false, "Initializing Generator function without a length property - why isn't there a length?.");
            varLength = TaggedInt::ToVarUnchecked(0);
        }
        function->SetPropertyWithAttributes(PropertyIds::length, varLength, PropertyConfigurable, nullptr, PropertyOperation_None, SideEffects_None);

        if (!isAnonymousFunction)
        {
            JavascriptString * functionName = nullptr;
            DebugOnly(bool status = ) ((Js::JavascriptFunction*)function)->GetFunctionName(&functionName);
            Assert(status);
            function->SetPropertyWithAttributes(PropertyIds::name,functionName, PropertyConfigurable, nullptr);
        }

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncGeneratorFunction(DynamicObject *instance, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptGeneratorFunction *function = VarTo<JavascriptAsyncGeneratorFunction>(instance);
        bool isAnonymousFunction = function->IsAnonymousFunction();

        JavascriptLibrary* javascriptLibrary = function->GetType()->GetLibrary();
        typeHandler->ConvertFunction(function, isAnonymousFunction ? javascriptLibrary->anonymousFunctionWithPrototypeTypeHandler : javascriptLibrary->functionWithPrototypeAndLengthTypeHandler);
        function->SetPropertyWithAttributes(PropertyIds::prototype, javascriptLibrary->CreateAsyncGeneratorConstructorPrototypeObject(), PropertyWritable, nullptr);

        Var varLength;
        GeneratorVirtualScriptFunction* scriptFunction = function->GetGeneratorVirtualScriptFunction();
        if (!scriptFunction->GetProperty(scriptFunction, PropertyIds::length, &varLength, nullptr, scriptFunction->GetScriptContext()))
        {
            // TODO - remove this if or convert it to a FailFast if this assert never triggers
            // Nothing in the ChakraCore CI will reach this code
            AssertMsg(false, "Initializing Async Generator function without a length property - why isn't there a length?.");
            varLength = TaggedInt::ToVarUnchecked(0);
        }
        function->SetPropertyWithAttributes(PropertyIds::length, varLength, PropertyConfigurable, nullptr, PropertyOperation_None, SideEffects_None);

        if (!isAnonymousFunction)
        {
            JavascriptString * functionName = nullptr;
            DebugOnly(bool status = ) ((Js::JavascriptFunction*)scriptFunction)->GetFunctionName(&functionName);
            Assert(status);
            function->SetPropertyWithAttributes(PropertyIds::name,functionName, PropertyConfigurable, nullptr);
        }

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncFunction(DynamicObject *function, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        // Async function instances do not have a prototype property as they are not constructable
        JavascriptAsyncFunction* asyncFunction = VarTo<JavascriptAsyncFunction>(function);

        if (!asyncFunction->IsAnonymousFunction())
        {
            typeHandler->Convert(function, mode, 2);
            JavascriptString * functionName = nullptr;
            DebugOnly(bool status = ) ((Js::JavascriptFunction*)function)->GetFunctionName(&functionName);
            Assert(status);
            function->SetPropertyWithAttributes(PropertyIds::name, functionName, PropertyConfigurable, nullptr);
        }
        else
        {
            typeHandler->Convert(function, mode, 1);
        }

        Var varLength;
        GeneratorVirtualScriptFunction* scriptFunction = asyncFunction->GetGeneratorVirtualScriptFunction();
        if (!scriptFunction->GetProperty(scriptFunction, PropertyIds::length, &varLength, nullptr, scriptFunction->GetScriptContext()))
        {
            // TODO - remove this if or convert it to a FailFast if this assert never triggers
            // Nothing in the ChakraCore CI will reach this code
            AssertMsg(false, "Initializing Async function without a length property - why isn't there a length?.");
            varLength = TaggedInt::ToVarUnchecked(0);
        }
        function->SetPropertyWithAttributes(PropertyIds::length, varLength, PropertyConfigurable, nullptr, PropertyOperation_None, SideEffects_None);

        return true;
    }

    /* static */
    void JavascriptLibrary::PrecalculateArrayAllocationBuckets()
    {
        JavascriptArray::EnsureCalculationOfAllocationBuckets<Js::JavascriptNativeIntArray>();
        JavascriptArray::EnsureCalculationOfAllocationBuckets<Js::JavascriptNativeFloatArray>();
        JavascriptArray::EnsureCalculationOfAllocationBuckets<Js::JavascriptArray>();
    }

    template<bool addPrototype, bool addName, bool useLengthType, bool addLength>
    bool JavascriptLibrary::InitializeFunction(DynamicObject *instance, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptFunction * function = VarTo<JavascriptFunction>(instance);
        JavascriptLibrary* javascriptLibrary = function->GetType()->GetLibrary();
        ScriptFunction *scriptFunction = nullptr;
        bool useAnonymous = false;
        if (VarIs<ScriptFunction>(function))
        {
            scriptFunction = Js::VarTo<Js::ScriptFunction>(function);
            useAnonymous = scriptFunction->IsAnonymousFunction();
        }

        if (!addPrototype)
        {
            if (!useAnonymous && addName)
            {
                typeHandler->ConvertFunction(function, useLengthType ? javascriptLibrary->functionTypeHandlerWithLength : javascriptLibrary->functionTypeHandler);
            }
            else
            {
                typeHandler->ConvertFunction(function, javascriptLibrary->anonymousFunctionTypeHandler);
            }
        }
        else
        {
            if (useAnonymous)
            {
                typeHandler->ConvertFunction(function, javascriptLibrary->anonymousFunctionWithPrototypeTypeHandler);
            }
            else
            {
                typeHandler->ConvertFunction(function, useLengthType ? javascriptLibrary->functionWithPrototypeAndLengthTypeHandler : javascriptLibrary->functionWithPrototypeTypeHandler);
            }
            DynamicObject *protoObject = javascriptLibrary->CreateConstructorPrototypeObject(function);
            if (scriptFunction && scriptFunction->GetFunctionInfo()->IsClassConstructor())
            {
                function->SetPropertyWithAttributes(PropertyIds::prototype, protoObject, PropertyNone, nullptr);
            }
            else
            {
                function->SetProperty(PropertyIds::prototype, protoObject, PropertyOperation_None, nullptr);
            }
        }

        if (scriptFunction)
        {
            ParseableFunctionInfo * funcInfo = scriptFunction->GetFunctionProxy()->EnsureDeserialized();

            CompileAssert(!addLength || useLengthType);
            if (addLength)
            {
                function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(funcInfo->GetReportedInParamsCount() - 1), PropertyConfigurable, nullptr, PropertyOperation_None, SideEffects_None);
            }

            if (useAnonymous || funcInfo->GetIsStaticNameFunction())
            {
                return true;
            }
        }

        if (addName)
        {
            JavascriptString * functionName = nullptr;
            if (((Js::JavascriptFunction*)function)->GetFunctionName(&functionName))
            {
                function->SetPropertyWithAttributes(PropertyIds::name, functionName, PropertyConfigurable, nullptr);
            }
        }

        return true;
    }

    DynamicType* JavascriptLibrary::GetErrorType(ErrorTypeEnum typeToFind) const
    {
        switch (typeToFind)
        {
        case kjstError:
            return GetErrorType();

        case kjstEvalError:
            return GetEvalErrorType();

        case kjstRangeError:
            return GetRangeErrorType();

        case kjstReferenceError:
            return GetReferenceErrorType();

        case kjstSyntaxError:
            return GetSyntaxErrorType();

        case kjstTypeError:
            return GetTypeErrorType();

        case kjstURIError:
            return GetURIErrorType();

        case kjstAggregateError:
            return GetAggregateErrorType();

        case kjstWebAssemblyCompileError:
            return GetWebAssemblyCompileErrorType();

        case kjstWebAssemblyRuntimeError:
            return GetWebAssemblyRuntimeErrorType();

        case kjstWebAssemblyLinkError:
            return GetWebAssemblyLinkErrorType();
        }

        return nullptr;
    }

    template<bool isNameAvailable, bool isPrototypeAvailable, bool isLengthAvailable = false>
    class InitializeFunctionDeferredTypeHandlerFilter
    {
    public:
        static bool HasFilter() { return true; }
        static bool HasProperty(PropertyId propertyId)
        {
            switch (propertyId)
            {
            case PropertyIds::prototype:
                return isPrototypeAvailable;
            case PropertyIds::name:
                return isNameAvailable;
            case PropertyIds::length:
                return isLengthAvailable;
            }
            return false;
        }
    };

    template<bool isNameAvailable, bool isPrototypeAvailable, bool isLengthAvailable, bool addLength>
    DynamicTypeHandler * JavascriptLibrary::GetDeferredFunctionTypeHandlerBase()
    {
        return DeferredTypeHandler<InitializeFunction<isPrototypeAvailable, isNameAvailable, isLengthAvailable, addLength>, InitializeFunctionDeferredTypeHandlerFilter<isNameAvailable, isPrototypeAvailable, isLengthAvailable>>::GetDefaultInstance();
    }

    template<bool isNameAvailable, bool isPrototypeAvailable>
    DynamicTypeHandler * JavascriptLibrary::GetDeferredGeneratorFunctionTypeHandlerBase()
    {
        return DeferredTypeHandler<InitializeGeneratorFunction, InitializeFunctionDeferredTypeHandlerFilter<isNameAvailable, isPrototypeAvailable, /*isLengthAvailable*/ true>>::GetDefaultInstance();
    }

    template<bool isNameAvailable, bool isPrototypeAvailable>
    DynamicTypeHandler * JavascriptLibrary::GetDeferredAsyncGeneratorFunctionTypeHandlerBase()
    {
        return DeferredTypeHandler<InitializeAsyncGeneratorFunction, InitializeFunctionDeferredTypeHandlerFilter<isNameAvailable, isPrototypeAvailable, /*isLengthAvailable*/ true>>::GetDefaultInstance();
    }

    template<bool isNameAvailable>
    DynamicTypeHandler * JavascriptLibrary::GetDeferredAsyncFunctionTypeHandlerBase()
    {
        // Async functions do not have the prototype property
        return DeferredTypeHandler<InitializeAsyncFunction, InitializeFunctionDeferredTypeHandlerFilter<isNameAvailable, /* isPrototypeAvailable */ false, /*isLengthAvailable*/ true>>::GetDefaultInstance();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredAnonymousPrototypeGeneratorFunctionTypeHandler()
    {
        return JavascriptLibrary::GetDeferredGeneratorFunctionTypeHandlerBase</*isNameAvailable*/ false>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredAnonymousPrototypeAsyncGeneratorFunctionTypeHandler()
    {
        return JavascriptLibrary::GetDeferredAsyncGeneratorFunctionTypeHandlerBase</*isNameAvailable*/ false>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredAnonymousPrototypeAsyncFunctionTypeHandler()
    {
        return JavascriptLibrary::GetDeferredAsyncFunctionTypeHandlerBase</*isNameAvailable*/ false>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredPrototypeGeneratorFunctionTypeHandler(ScriptContext* scriptContext)
    {
        return JavascriptLibrary::GetDeferredGeneratorFunctionTypeHandlerBase</*isNameAvailable*/ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredPrototypeAsyncGeneratorFunctionTypeHandler()
    {
        return JavascriptLibrary::GetDeferredAsyncGeneratorFunctionTypeHandlerBase</*isNameAvailable*/ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredPrototypeAsyncFunctionTypeHandler(ScriptContext* scriptContext)
    {
        return JavascriptLibrary::GetDeferredAsyncFunctionTypeHandlerBase</*isNameAvailable*/ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredAnonymousPrototypeFunctionWithLengthTypeHandler()
    {
        return JavascriptLibrary::GetDeferredFunctionTypeHandlerBase</*isNameAvailable*/ false, /* isPrototypeAvailable */ true, /* isLengthAvailable */ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredPrototypeFunctionTypeHandler(ScriptContext* scriptContext)
    {
        return JavascriptLibrary::GetDeferredFunctionTypeHandlerBase</* isNameAvailable */ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredPrototypeFunctionWithNameAndLengthTypeHandler()
    {
        return JavascriptLibrary::GetDeferredFunctionTypeHandlerBase</* isNameAvailable */ true, /* isPrototypeAvailable */ true, /* isLengthAvailable */ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::ClassConstructorTypeHandler()
    {
        return &SharedFunctionWithNonWritablePrototypeLengthAndNameTypeHandler;
    }

    DynamicTypeHandler * JavascriptLibrary::AnonymousClassConstructorTypeHandler()
    {
        return &SharedFunctionWithNonWritablePrototypeAndLengthTypeHandler;
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredPrototypeFunctionWithLengthTypeHandler(ScriptContext* scriptContext)
    {
        return DeferredTypeHandler<Js::JavascriptExternalFunction::DeferredLengthInitializer, InitializeFunctionDeferredTypeHandlerFilter</* isNameAvailable */ true, /* isPrototypeAvailable */ true, /* isLengthAvailable */ true>>::GetDefaultInstance();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredAnonymousFunctionWithLengthTypeHandler()
    {
        return JavascriptLibrary::GetDeferredFunctionTypeHandlerBase</* isNameAvailable */ false, /* isPrototypeAvailable */ false, /* isLengthAvailable */ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredAnonymousFunctionTypeHandler()
    {
        return JavascriptLibrary::GetDeferredFunctionTypeHandlerBase</* isNameAvailable */ false, /* isPrototypeAvailable */ false, /* isLengthAvailable */ false>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredFunctionTypeHandler()
    {
        return GetDeferredFunctionTypeHandlerBase</*isNameAvailable*/ true, /*isPrototypeAvailable*/ false, /* isLengthAvailable */ false>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredFunctionWithLengthTypeHandler()
    {
        return GetDeferredFunctionTypeHandlerBase</*isNameAvailable*/ true, /*isPrototypeAvailable*/ false, /* isLengthAvailable */ true>();
    }

    DynamicTypeHandler * JavascriptLibrary::GetDeferredFunctionWithLengthUnsetTypeHandler()
    {
        return GetDeferredFunctionTypeHandlerBase</*isNameAvailable*/ true, /*isPrototypeAvailable*/ false, /* isLengthAvailable */ true, /* addLength */ false>();
    }

    DynamicTypeHandler * JavascriptLibrary::ScriptFunctionTypeHandler(bool noPrototypeProperty, bool isAnonymousFunction)
    {
        DynamicTypeHandler * scriptFunctionTypeHandler = nullptr;

        if (noPrototypeProperty)
        {
            scriptFunctionTypeHandler = isAnonymousFunction ?
                this->GetDeferredAnonymousFunctionWithLengthTypeHandler() :
                this->GetDeferredFunctionWithLengthTypeHandler();
        }
        else
        {
            scriptFunctionTypeHandler = isAnonymousFunction ?
                JavascriptLibrary::GetDeferredAnonymousPrototypeFunctionWithLengthTypeHandler() :
                JavascriptLibrary::GetDeferredPrototypeFunctionWithNameAndLengthTypeHandler();
        }
        return scriptFunctionTypeHandler;
    }

    DynamicType * JavascriptLibrary::CreateDeferredPrototypeGeneratorFunctionType(JavascriptMethod entrypoint, bool isAnonymousFunction, bool isShared)
    {
        return DynamicType::New(scriptContext, TypeIds_Function, generatorFunctionPrototype, entrypoint,
            isAnonymousFunction ? GetDeferredAnonymousPrototypeGeneratorFunctionTypeHandler() : GetDeferredPrototypeGeneratorFunctionTypeHandler(scriptContext), isShared, isShared);
    }

    DynamicType * JavascriptLibrary::CreateDeferredPrototypeAsyncGeneratorFunctionType(JavascriptMethod entrypoint, bool isAnonymousFunction, bool isShared)
    {
        return DynamicType::New(scriptContext, TypeIds_Function, asyncGeneratorFunctionPrototype, entrypoint,
            isAnonymousFunction ? GetDeferredAnonymousPrototypeAsyncGeneratorFunctionTypeHandler() : GetDeferredPrototypeAsyncGeneratorFunctionTypeHandler(), isShared, isShared);
    }

    DynamicType * JavascriptLibrary::CreateDeferredPrototypeAsyncFunctionType(JavascriptMethod entrypoint, bool isAnonymousFunction, bool isShared)
    {
        return DynamicType::New(scriptContext, TypeIds_Function, asyncFunctionPrototype, entrypoint,
            isAnonymousFunction ? GetDeferredAnonymousPrototypeAsyncFunctionTypeHandler() : GetDeferredPrototypeAsyncFunctionTypeHandler(scriptContext), isShared, isShared);
    }

    DynamicType * JavascriptLibrary::CreateDeferredFunctionType(JavascriptMethod entrypoint)
    {
        return CreateDeferredFunctionTypeNoProfileThunk(this->inDispatchProfileMode ? ProfileEntryThunk : entrypoint);
    }

    DynamicType * JavascriptLibrary::CreateDeferredPrototypeFunctionType(JavascriptMethod entrypoint)
    {
        return CreateDeferredPrototypeFunctionTypeNoProfileThunk(this->inDispatchProfileMode ? ProfileEntryThunk : entrypoint);
    }

    DynamicType * JavascriptLibrary::CreateDeferredFunctionTypeNoProfileThunk(JavascriptMethod entryPoint, bool isShared)
    {
        return CreateDeferredFunctionTypeNoProfileThunk_Internal<false, false>(entryPoint, isShared);
    }

    DynamicType * JavascriptLibrary::CreateDeferredLengthFunctionTypeNoProfileThunk(JavascriptMethod entryPoint, bool isShared)
    {
        return CreateDeferredFunctionTypeNoProfileThunk_Internal<true, false>(entryPoint, isShared);
    }

    DynamicType * JavascriptLibrary::CreateDeferredPrototypeFunctionTypeNoProfileThunk(JavascriptMethod entryPoint, bool isShared)
    {
        return CreateDeferredFunctionTypeNoProfileThunk_Internal<false, true>(entryPoint, isShared);
    }

    DynamicType * JavascriptLibrary::CreateDeferredLengthPrototypeFunctionTypeNoProfileThunk(JavascriptMethod entryPoint, bool isShared)
    {
        return CreateDeferredFunctionTypeNoProfileThunk_Internal<true, true>(entryPoint, isShared);
    }

    template<bool isLengthAvailable, bool isPrototypeAvailable>
    DynamicType * JavascriptLibrary::CreateDeferredFunctionTypeNoProfileThunk_Internal(JavascriptMethod entrypoint, bool isShared)
    {
        // Note: the lack of TypeHandler switching here based on the isAnonymousFunction flag is intentional.
        // We can't switch shared typeHandlers and RuntimeFunctions do not produce script code for us to know if a function is Anonymous.
        // As a result we may have an issue where hasProperty would say you have a name property but getProperty returns undefined
        DynamicTypeHandler * typeHandler = 
            isLengthAvailable ?
                (isPrototypeAvailable ?
                    GetDeferredPrototypeFunctionWithLengthTypeHandler(scriptContext) : GetDeferredFunctionWithLengthTypeHandler()) :
                (isPrototypeAvailable ?
                    GetDeferredPrototypeFunctionTypeHandler(scriptContext) : GetDeferredFunctionTypeHandler());
        return DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entrypoint, typeHandler, isShared, isShared);
    }

    DynamicType * JavascriptLibrary::CreateFunctionType(JavascriptMethod entrypoint, RecyclableObject* prototype)
    {
        if (prototype == nullptr)
        {
            prototype = functionPrototype;
        }

        return DynamicType::New(scriptContext, TypeIds_Function, prototype, entrypoint,
            GetDeferredFunctionTypeHandler(), false, false);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithConfigurableLengthType(FunctionInfo * functionInfo)
    {
        return CreateFunctionWithConfigurableLengthType(this->GetFunctionPrototype(), functionInfo);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithLengthType(FunctionInfo * functionInfo)
    {
        return CreateFunctionWithLengthType(this->GetFunctionPrototype(), functionInfo);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithLengthAndNameType(FunctionInfo * functionInfo)
    {
        return CreateFunctionWithLengthAndNameType(this->GetFunctionPrototype(), functionInfo);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithLengthAndPrototypeType(FunctionInfo * functionInfo)
    {
        return CreateFunctionWithLengthAndPrototypeType(this->GetFunctionPrototype(), functionInfo);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithConfigurableLengthType(DynamicObject * prototype, FunctionInfo * functionInfo)
    {
        Assert(!functionInfo->HasBody());
        return DynamicType::New(scriptContext, TypeIds_Function, prototype,
            this->inProfileMode? ProfileEntryThunk : functionInfo->GetOriginalEntryPoint(),
            &SharedFunctionWithConfigurableLengthTypeHandler);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithLengthType(DynamicObject * prototype, FunctionInfo * functionInfo)
    {
        Assert(!functionInfo->HasBody());
        return DynamicType::New(scriptContext, TypeIds_Function, prototype,
            this->inProfileMode? ProfileEntryThunk : functionInfo->GetOriginalEntryPoint(),
            &SharedFunctionWithLengthTypeHandler);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithLengthAndNameType(DynamicObject * prototype, FunctionInfo * functionInfo)
    {
        Assert(!functionInfo->HasBody());
        return DynamicType::New(scriptContext, TypeIds_Function, prototype,
            this->inProfileMode ? ProfileEntryThunk : functionInfo->GetOriginalEntryPoint(),
            &SharedFunctionWithLengthAndNameTypeHandler);
    }

    DynamicType * JavascriptLibrary::CreateFunctionWithLengthAndPrototypeType(DynamicObject * prototype, FunctionInfo * functionInfo)
    {
        Assert(!functionInfo->HasBody());
        return DynamicType::New(scriptContext, TypeIds_Function, prototype,
            this->inProfileMode? ProfileEntryThunk : functionInfo->GetOriginalEntryPoint(),
            SimpleDictionaryTypeHandler::New(scriptContext, FunctionWithLengthAndPrototypeTypeDescriptors, _countof(FunctionWithLengthAndPrototypeTypeDescriptors), 0, 0));
    }

    void JavascriptLibrary::InitializeProperties(ThreadContext * threadContext)
    {
        if ( threadContext->GetMaxPropertyId() < PropertyIds::_countJSOnlyProperty )
        {
            threadContext->UncheckedAddBuiltInPropertyId();
        }
    }

    DynamicObject * JavascriptLibraryBase::GetObjectPrototype()
    {
        return GetObjectPrototypeObject();
    }

    void JavascriptLibraryBase::Finalize(bool isShutdown)
    {
        if (scriptContext)
        {
            // Clear the weak reference dictionary so we don't need to clean them
            // during PostCollectCallBack before Dispose deleting the script context.
            scriptContext->ResetWeakReferenceDictionaryList();
            scriptContext->SetIsFinalized();
            scriptContext->MarkForClose();
            if (scriptContext->IsRegistered())
            {
                scriptContext->GetThreadContext()->UnregisterScriptContext(scriptContext);
            }
        }
    }

    void JavascriptLibraryBase::Dispose(bool isShutdown)
    {
        if (scriptContext)
        {
            HeapDelete(scriptContext);
            scriptContext = nullptr;
        }
    }

    void JavascriptLibrary::Finalize(bool isShutdown)
    {
        __super::Finalize(isShutdown);

        this->SetFakeGlobalFuncForUndefer(nullptr);

        if (this->referencedPropertyRecords != nullptr)
        {
            RECYCLER_PERF_COUNTER_SUB(PropertyRecordBindReference, this->referencedPropertyRecords->Count());
        }
    }

    void JavascriptLibrary::InitializeGlobal(GlobalObject * globalObject)
    {
        RecyclableObject* globalObjectPrototype = GetObjectPrototype();
        globalObject->SetPrototype(globalObjectPrototype);
        Recycler * recycler = this->GetRecycler();
        StaticType* staticString = StaticType::New(scriptContext, TypeIds_String, stringPrototype, nullptr);
        stringCache.Initialize(scriptContext, staticString);

        pi = JavascriptNumber::New(Math::PI, scriptContext);
        nan = JavascriptNumber::New(JavascriptNumber::NaN, scriptContext);
        negativeInfinite = JavascriptNumber::New(JavascriptNumber::NEGATIVE_INFINITY, scriptContext);
        positiveInfinite = JavascriptNumber::New(JavascriptNumber::POSITIVE_INFINITY, scriptContext);
        minValue = JavascriptNumber::New(JavascriptNumber::MIN_VALUE, scriptContext);
        maxValue = JavascriptNumber::New(JavascriptNumber::MAX_VALUE, scriptContext);
        negativeZero = JavascriptNumber::New(JavascriptNumber::NEG_ZERO, scriptContext);

        Type* type = StaticType::New(scriptContext, TypeIds_Undefined, nullValue, nullptr);
        undefinedValue = RecyclerNew(recycler, RecyclableObject, type);
        DynamicType* dynamicType = DynamicType::New(scriptContext, TypeIds_Object, nullValue, nullptr, &MissingPropertyHolderTypeHandler);
        missingPropertyHolder = RecyclerNewPlus(recycler, sizeof(Var), DynamicObject, dynamicType);
        MissingPropertyTypeHandler::SetUndefinedPropertySlot(missingPropertyHolder);

        emptyString = CreateEmptyString(); // Must be created before other calls to CreateString
        nullString = CreateEmptyString(); // Must be distinct from emptyString (for the DOM)

        promiseResolveFunction = nullptr;
        promiseThenFunction = nullptr;
        generatorNextFunction = nullptr;
        generatorThrowFunction = nullptr;
        asyncGeneratorNextFunction = nullptr;
        asyncGeneratorReturnFunction = nullptr;
        asyncGeneratorThrowFunction = nullptr;
        jsonStringifyFunction = nullptr;
        objectFreezeFunction = nullptr;

        symbolAsyncIterator = CreateSymbol(BuiltInPropertyRecords::_symbolAsyncIterator);
        symbolHasInstance = CreateSymbol(BuiltInPropertyRecords::_symbolHasInstance);
        symbolIsConcatSpreadable = CreateSymbol(BuiltInPropertyRecords::_symbolIsConcatSpreadable);
        symbolIterator = CreateSymbol(BuiltInPropertyRecords::_symbolIterator);
        symbolSpecies = CreateSymbol(BuiltInPropertyRecords::_symbolSpecies);
        symbolToPrimitive = CreateSymbol(BuiltInPropertyRecords::_symbolToPrimitive);
        symbolToStringTag = CreateSymbol(BuiltInPropertyRecords::_symbolToStringTag);
        symbolUnscopables = CreateSymbol(BuiltInPropertyRecords::_symbolUnscopables);

        if (scriptContext->GetConfig()->IsES6RegExSymbolsEnabled())
        {
            symbolMatch = CreateSymbol(BuiltInPropertyRecords::_symbolMatch);
            symbolReplace = CreateSymbol(BuiltInPropertyRecords::_symbolReplace);
            symbolSearch = CreateSymbol(BuiltInPropertyRecords::_symbolSearch);
            symbolSplit = CreateSymbol(BuiltInPropertyRecords::_symbolSplit);
        }
        else
        {
            symbolMatch = nullptr;
            symbolReplace = nullptr;
            symbolSearch = nullptr;
            symbolSplit = nullptr;
        }

        debuggerDeadZoneBlockVariableString = CreateStringFromCppLiteral(_u("[Uninitialized block variable]"));
        defaultAccessorFunction = CreateNonProfiledFunction(&JavascriptOperators::EntryInfo::DefaultAccessor);

        if (scriptContext->GetConfig()->IsErrorStackTraceEnabled())
        {
            stackTraceAccessorFunction = CreateNonProfiledFunction(&JavascriptExceptionOperators::EntryInfo::StackTraceAccessor);
            stackTraceAccessorFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);
        }

        throwTypeErrorRestrictedPropertyAccessorFunction = CreateNonProfiledFunction(&JavascriptExceptionOperators::EntryInfo::ThrowTypeErrorRestrictedPropertyAccessor);
        throwTypeErrorRestrictedPropertyAccessorFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);

        __proto__getterFunction = CreateNonProfiledFunction(&ObjectPrototypeObject::EntryInfo::__proto__getter);
        __proto__getterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);

        __proto__setterFunction = CreateNonProfiledFunction(&ObjectPrototypeObject::EntryInfo::__proto__setter);
        __proto__setterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);


        identityFunction = CreateNonProfiledFunction(&JavascriptPromise::EntryInfo::Identity);
        identityFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);

        throwerFunction = CreateNonProfiledFunction(&JavascriptPromise::EntryInfo::Thrower);
        throwerFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);

        booleanTrue = RecyclerNew(recycler, JavascriptBoolean, true, booleanTypeStatic);
        booleanFalse = RecyclerNew(recycler, JavascriptBoolean, false, booleanTypeStatic);

        isPRNGSeeded = false;
        randSeed0 = 0;
        randSeed1 = 0;

        if (globalObject->GetScriptContext()->GetConfig()->IsESGlobalThisEnabled())
        {
            AddMember(globalObject, PropertyIds::globalThis, globalObject->ToThis(), PropertyConfigurable | PropertyWritable);
        }
        AddMember(globalObject, PropertyIds::NaN, nan, PropertyNone);
        AddMember(globalObject, PropertyIds::Infinity, positiveInfinite, PropertyNone);
        AddMember(globalObject, PropertyIds::undefined, undefinedValue, PropertyNone);
        // Note: for global object, we need to set toStringTag to global like other engines (v8)
        if (globalObject->GetScriptContext()->GetConfig()->IsES6ToStringTagEnabled())
        {
            AddMember(globalObject, PropertyIds::_symbolToStringTag, scriptContext->GetPropertyString(PropertyIds::global), PropertyConfigurable | PropertyWritable | PropertyEnumerable);
        }

        // Note: Any global function added/removed/changed here should also be updated in JavascriptLibrary::ProfilerRegisterBuiltinFunctions
        // so that the new functions show up in the profiler too.
        Field(JavascriptFunction*)* builtinFuncs = this->GetBuiltinFunctions();

        evalFunctionObject = AddFunctionToLibraryObject(globalObject, PropertyIds::eval, &GlobalObject::EntryInfo::Eval, 1);
        parseIntFunctionObject = AddFunctionToLibraryObject(globalObject, PropertyIds::parseInt, &GlobalObject::EntryInfo::ParseInt, 2);
        builtinFuncs[BuiltinFunction::GlobalObject_ParseInt] = parseIntFunctionObject;
        parseFloatFunctionObject = AddFunctionToLibraryObject(globalObject, PropertyIds::parseFloat, &GlobalObject::EntryInfo::ParseFloat, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::isNaN, &GlobalObject::EntryInfo::IsNaN, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::isFinite, &GlobalObject::EntryInfo::IsFinite, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::decodeURI, &GlobalObject::EntryInfo::DecodeURI, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::decodeURIComponent, &GlobalObject::EntryInfo::DecodeURIComponent, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::encodeURI, &GlobalObject::EntryInfo::EncodeURI, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::encodeURIComponent, &GlobalObject::EntryInfo::EncodeURIComponent, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::escape, &GlobalObject::EntryInfo::Escape, 1);
        AddFunctionToLibraryObject(globalObject, PropertyIds::unescape, &GlobalObject::EntryInfo::UnEscape, 1);

        if (scriptContext->GetConfig()->IsCollectGarbageEnabled())
        {
            AddFunctionToLibraryObject(globalObject, PropertyIds::CollectGarbage, &GlobalObject::EntryInfo::CollectGarbage, 0);
        }

#if ENABLE_TTD
        if(scriptContext->GetThreadContext()->IsRuntimeInTTDMode())
        {
            //
            //TODO: when we formalize our telemetry library in JS land we will want to move these to a seperate Debug or Telemetry object instead of cluttering the global object
            //
            AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("telemetryLog"), &GlobalObject::EntryInfo::TelemetryLog, 3);

            AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("enabledDiagnosticsTrace"), &GlobalObject::EntryInfo::EnabledDiagnosticsTrace, 1);
            AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("emitTTDLog"), &GlobalObject::EntryInfo::EmitTTDLog, 2);
        }
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("chWriteTraceEvent"), &GlobalObject::EntryInfo::ChWriteTraceEvent, 1);
#endif

#ifdef IR_VIEWER
        if (Js::Configuration::Global.flags.IsEnabled(Js::IRViewerFlag))
        {
            AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("parseIR"), &GlobalObject::EntryInfo::ParseIR, 1);
            AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("functionList"), &GlobalObject::EntryInfo::FunctionList, 1);
            AddFunctionToLibraryObjectWithPropertyName(globalObject, _u("rejitFunction"), &GlobalObject::EntryInfo::RejitFunction, 2);
        }
#endif /* IR_VIEWER */

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        builtInConstructorCache = RecyclerNew(this->GetRecycler(), ConstructorCache);
        builtInConstructorCache->PopulateForSkipDefaultNewObject(this->GetScriptContext());

        objectConstructor = CreateBuiltinConstructor(&JavascriptObject::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeObjectConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Object, objectConstructor);
        arrayConstructor = CreateBuiltinConstructor(&JavascriptArray::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeArrayConstructor>::GetDefaultInstance());
        SetArrayObjectHasUserDefinedSpecies(false);
        AddFunction(globalObject, PropertyIds::Array, arrayConstructor);
        booleanConstructor = CreateBuiltinConstructor(&JavascriptBoolean::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeBooleanConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Boolean, booleanConstructor);

        symbolConstructor = nullptr;
        proxyConstructor = nullptr;
        promiseConstructor = nullptr;
        reflectObject = nullptr;
        debugEval = nullptr;
        getStackTrace = nullptr;
#ifdef EDIT_AND_CONTINUE
        editSource = nullptr;
#endif

        symbolConstructor = CreateBuiltinConstructor(&JavascriptSymbol::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeSymbolConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Symbol, symbolConstructor);

        if (scriptContext->GetConfig()->IsES6ProxyEnabled())
        {
            proxyConstructor = CreateBuiltinConstructor(&JavascriptProxy::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeProxyConstructor>::GetDefaultInstance());
            AddFunction(globalObject, PropertyIds::Proxy, proxyConstructor);
            reflectObject = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeReflectObject>::GetDefaultInstance()));
            AddMember(globalObject, PropertyIds::Reflect, reflectObject);
        }

        promiseConstructor = CreateBuiltinConstructor(&JavascriptPromise::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializePromiseConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Promise, promiseConstructor);

        dateConstructor = CreateBuiltinConstructor(&JavascriptDate::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeDateConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Date, dateConstructor);
        functionConstructor = CreateBuiltinConstructor(&JavascriptFunction::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeFunctionConstructor, DefaultDeferredTypeFilter, true>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Function, functionConstructor);
        mathObject = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeMathObject>::GetDefaultInstance()));
        AddMember(globalObject, PropertyIds::Math, mathObject);

        debugObject = nullptr;

        numberConstructor = CreateBuiltinConstructor(&JavascriptNumber::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeNumberConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Number, numberConstructor);

        bigIntConstructor = nullptr;
        if (scriptContext->GetConfig()->IsESBigIntEnabled())
        {
            bigIntConstructor = CreateBuiltinConstructor(&JavascriptBigInt::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeBigIntConstructor>::GetDefaultInstance());
            AddFunction(globalObject, PropertyIds::BigInt, bigIntConstructor);
        }

        stringConstructor = CreateBuiltinConstructor(&JavascriptString::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeStringConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::String, stringConstructor);
        regexConstructorType = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, JavascriptRegExp::NewInstance,
            DeferredTypeHandler<InitializeRegexConstructor>::GetDefaultInstance());
        regexConstructor = RecyclerNewEnumClass(recycler, EnumFunctionClass,
            JavascriptRegExpConstructor,
            regexConstructorType,
            builtInConstructorCache);
        AddFunction(globalObject, PropertyIds::RegExp, regexConstructor);

        arrayBufferConstructor = CreateBuiltinConstructor(&ArrayBuffer::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeArrayBufferConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::ArrayBuffer, arrayBufferConstructor);
        dataViewConstructor = CreateBuiltinConstructor(&DataView::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeDataViewConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::DataView, dataViewConstructor);

        typedArrayConstructor = CreateBuiltinConstructor(&TypedArrayBase::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeTypedArrayConstructor, DefaultDeferredTypeFilter, true>::GetDefaultInstance(),
            functionPrototype);

        Int8ArrayConstructor = CreateBuiltinConstructor(&Int8Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeInt8ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Int8Array, Int8ArrayConstructor);

        Uint8ArrayConstructor = CreateBuiltinConstructor(&Uint8Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeUint8ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Uint8Array, Uint8ArrayConstructor);

        Uint8ClampedArrayConstructor = CreateBuiltinConstructor(&Uint8ClampedArray::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeUint8ClampedArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Uint8ClampedArray, Uint8ClampedArrayConstructor);

        Int16ArrayConstructor = CreateBuiltinConstructor(&Int16Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeInt16ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Int16Array, Int16ArrayConstructor);

        Uint16ArrayConstructor = CreateBuiltinConstructor(&Uint16Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeUint16ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Uint16Array, Uint16ArrayConstructor);

        Int32ArrayConstructor = CreateBuiltinConstructor(&Int32Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeInt32ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Int32Array, Int32ArrayConstructor);

        Uint32ArrayConstructor = CreateBuiltinConstructor(&Uint32Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeUint32ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Uint32Array, Uint32ArrayConstructor);

        Float32ArrayConstructor = CreateBuiltinConstructor(&Float32Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeFloat32ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Float32Array, Float32ArrayConstructor);

        Float64ArrayConstructor = CreateBuiltinConstructor(&Float64Array::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeFloat64ArrayConstructor>::GetDefaultInstance(),
            typedArrayConstructor);
        AddFunction(globalObject, PropertyIds::Float64Array, Float64ArrayConstructor);

        if (scriptContext->GetConfig()->IsESSharedArrayBufferEnabled())
        {
            sharedArrayBufferConstructor = CreateBuiltinConstructor(&SharedArrayBuffer::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeSharedArrayBufferConstructor>::GetDefaultInstance());
            AddFunction(globalObject, PropertyIds::SharedArrayBuffer, sharedArrayBufferConstructor);

            atomicsObject = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeAtomicsObject>::GetDefaultInstance()));
            AddMember(globalObject, PropertyIds::Atomics, atomicsObject);
        }
        else
        {
            sharedArrayBufferConstructor = nullptr;
            atomicsObject = nullptr;
        }

        JSONObject = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
            DeferredTypeHandler<InitializeJSONObject>::GetDefaultInstance()));
        AddMember(globalObject, PropertyIds::JSON, JSONObject);

#ifdef ENABLE_INTL_OBJECT
        if (scriptContext->IsIntlEnabled())
        {
            IntlObject = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeIntlObject>::GetDefaultInstance()));
            AddMember(globalObject, PropertyIds::Intl, IntlObject);
        }
        else
        {
            IntlObject = nullptr;
        }
#endif

#if defined(ENABLE_INTL_OBJECT) || defined(ENABLE_JS_BUILTINS)
        engineInterfaceObject = EngineInterfaceObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_EngineInterfaceObject, nullValue, nullptr,
            DeferredTypeHandler<InitializeEngineInterfaceObject>::GetDefaultInstance()));

#ifdef ENABLE_INTL_OBJECT
        IntlEngineInterfaceExtensionObject* intlExtension = RecyclerNew(recycler, IntlEngineInterfaceExtensionObject, scriptContext);
        engineInterfaceObject->SetEngineExtension(EngineInterfaceExtensionKind_Intl, intlExtension);
#endif

#ifdef ENABLE_JS_BUILTINS
        chakraLibraryObject = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, nullValue, nullptr,
            DeferredTypeHandler<InitializeChakraLibraryObject>::GetDefaultInstance()));
        if (CONFIG_FLAG(LdChakraLib)) {
            AddMember(globalObject, PropertyIds::__chakraLibrary, chakraLibraryObject);
        }

        JsBuiltInEngineInterfaceExtensionObject* builtInExtension = RecyclerNew(recycler, JsBuiltInEngineInterfaceExtensionObject, scriptContext);
        engineInterfaceObject->SetEngineExtension(EngineInterfaceExtensionKind_JsBuiltIn, builtInExtension);
        this->isArrayFunction = this->DefaultCreateFunction(&JavascriptArray::EntryInfo::IsArray, 1, nullptr, nullptr, PropertyIds::isArray);
#endif

#endif

        mapConstructor = CreateBuiltinConstructor(&JavascriptMap::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeMapConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Map, mapConstructor);

        setConstructor = CreateBuiltinConstructor(&JavascriptSet::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeSetConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Set, setConstructor);

        weakMapConstructor = CreateBuiltinConstructor(&JavascriptWeakMap::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeWeakMapConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::WeakMap, weakMapConstructor);

        weakSetConstructor = CreateBuiltinConstructor(&JavascriptWeakSet::EntryInfo::NewInstance,
            DeferredTypeHandler<InitializeWeakSetConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::WeakSet, weakSetConstructor);

        generatorFunctionConstructor = nullptr;

        if (scriptContext->GetConfig()->IsES6GeneratorsEnabled())
        {
            generatorFunctionConstructor = CreateBuiltinConstructor(&JavascriptGeneratorFunction::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeGeneratorFunctionConstructor>::GetDefaultInstance(),
                functionConstructor);
            // GeneratorFunction is not a global property by ES6 spec so don't add it to the global object
        }

        asyncFunctionConstructor = nullptr;

        if (scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            asyncFunctionConstructor = CreateBuiltinConstructor(&JavascriptFunction::EntryInfo::NewAsyncFunctionInstance,
                DeferredTypeHandler<InitializeAsyncFunctionConstructor>::GetDefaultInstance(),
                functionConstructor);
            // AsyncFunction is not a global property by ES7 spec so don't add it to the global object
        }

        asyncGeneratorFunctionConstructor = nullptr;

        if (scriptContext->GetConfig()->IsES2018AsyncIterationEnabled())
        {
            asyncGeneratorFunctionConstructor = CreateBuiltinConstructor(&JavascriptFunction::EntryInfo::NewAsyncGeneratorFunctionInstance,
            DeferredTypeHandler<InitializeAsyncGeneratorFunctionConstructor>::GetDefaultInstance(),
            functionConstructor);
            // AsyncGeneratorFunction is not a global property by ES2018 spec so don't add it to the global object
        }

        errorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewErrorInstance,
            DeferredTypeHandler<InitializeErrorConstructor>::GetDefaultInstance());
        AddFunction(globalObject, PropertyIds::Error, errorConstructor);

        RuntimeFunction* nativeErrorPrototype = nullptr;
        if (scriptContext->GetConfig()->IsES6PrototypeChain())
        {
            nativeErrorPrototype = errorConstructor;
        }

        evalErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewEvalErrorInstance,
            DeferredTypeHandler<InitializeEvalErrorConstructor>::GetDefaultInstance(),
            nativeErrorPrototype);
        AddFunction(globalObject, PropertyIds::EvalError, evalErrorConstructor);

        rangeErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewRangeErrorInstance,
            DeferredTypeHandler<InitializeRangeErrorConstructor>::GetDefaultInstance(),
            nativeErrorPrototype);
        AddFunction(globalObject, PropertyIds::RangeError, rangeErrorConstructor);

        referenceErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewReferenceErrorInstance,
            DeferredTypeHandler<InitializeReferenceErrorConstructor>::GetDefaultInstance(),
            nativeErrorPrototype);
        AddFunction(globalObject, PropertyIds::ReferenceError, referenceErrorConstructor);

        syntaxErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewSyntaxErrorInstance,
            DeferredTypeHandler<InitializeSyntaxErrorConstructor>::GetDefaultInstance(),
            nativeErrorPrototype);
        AddFunction(globalObject, PropertyIds::SyntaxError, syntaxErrorConstructor);

        typeErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewTypeErrorInstance,
            DeferredTypeHandler<InitializeTypeErrorConstructor>::GetDefaultInstance(),
            nativeErrorPrototype);
        AddFunction(globalObject, PropertyIds::TypeError, typeErrorConstructor);

        uriErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewURIErrorInstance,
            DeferredTypeHandler<InitializeURIErrorConstructor>::GetDefaultInstance(),
            nativeErrorPrototype);
        AddFunction(globalObject, PropertyIds::URIError, uriErrorConstructor);

        if (scriptContext->GetConfig()->IsESPromiseAnyEnabled())
        {
            aggregateErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewAggregateErrorInstance,
                DeferredTypeHandler<InitializeAggregateErrorConstructor>::GetDefaultInstance(),
                nativeErrorPrototype);
            AddFunction(globalObject, PropertyIds::AggregateError, aggregateErrorConstructor);
        }

#ifdef ENABLE_WASM
        if (CONFIG_FLAG(Wasm) && PHASE_ENABLED1(WasmPhase))
        {
            webAssemblyCompileFunction = nullptr;
            // new WebAssembly object
            webAssemblyObject = DynamicObject::New(recycler,
                DynamicType::New(scriptContext, TypeIds_Object, objectPrototype, nullptr,
                    DeferredTypeHandler<InitializeWebAssemblyObject>::GetDefaultInstance()));
            AddMember(globalObject, PropertyIds::WebAssembly, webAssemblyObject);

            webAssemblyCompileErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewWebAssemblyCompileErrorInstance,
                DeferredTypeHandler<InitializeWebAssemblyCompileErrorConstructor>::GetDefaultInstance(),
                nativeErrorPrototype);

            webAssemblyRuntimeErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewWebAssemblyRuntimeErrorInstance,
                DeferredTypeHandler<InitializeWebAssemblyRuntimeErrorConstructor>::GetDefaultInstance(),
                nativeErrorPrototype);

            webAssemblyLinkErrorConstructor = CreateBuiltinConstructor(&JavascriptError::EntryInfo::NewWebAssemblyLinkErrorInstance,
                DeferredTypeHandler<InitializeWebAssemblyLinkErrorConstructor>::GetDefaultInstance(),
                nativeErrorPrototype);

            webAssemblyInstanceConstructor = CreateBuiltinConstructor(&WebAssemblyInstance::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeWebAssemblyInstanceConstructor>::GetDefaultInstance(), webAssemblyInstancePrototype);

            webAssemblyModuleConstructor = CreateBuiltinConstructor(&WebAssemblyModule::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeWebAssemblyModuleConstructor>::GetDefaultInstance(), webAssemblyModulePrototype);

            webAssemblyMemoryConstructor = CreateBuiltinConstructor(&WebAssemblyMemory::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeWebAssemblyMemoryConstructor>::GetDefaultInstance(), webAssemblyMemoryConstructor);

            webAssemblyTableConstructor = CreateBuiltinConstructor(&WebAssemblyTable::EntryInfo::NewInstance,
                DeferredTypeHandler<InitializeWebAssemblyTableConstructor>::GetDefaultInstance(), webAssemblyTableConstructor);
        }
#endif
    }

    void JavascriptLibrary::EnsureDebugObject(DynamicObject* newDebugObject)
    {
        Assert(!debugObject);

        if (!debugObject)
        {
            this->debugObject = newDebugObject;
            AddMember(globalObject, PropertyIds::Debug, debugObject);
        }
    }

    void JavascriptLibrary::SetDebugObjectNonUserAccessor(FunctionInfo *funcGetter, FunctionInfo *funcSetter)
    {
        Assert(funcGetter);
        Assert(funcSetter);

        debugObjectNonUserGetterFunction = CreateNonProfiledFunction(funcGetter);
        debugObjectNonUserGetterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);

        debugObjectNonUserSetterFunction = CreateNonProfiledFunction(funcSetter);
        debugObjectNonUserSetterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);
    }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    void JavascriptLibrary::SetDebugObjectFaultInjectionCookieGetterAccessor(FunctionInfo *funcGetter, FunctionInfo *funcSetter)
    {
        Assert(funcGetter);
        Assert(funcSetter);

        debugObjectFaultInjectionCookieGetterFunction = CreateNonProfiledFunction(funcGetter);
        debugObjectFaultInjectionCookieGetterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);

        debugObjectFaultInjectionCookieSetterFunction = CreateNonProfiledFunction(funcSetter);
        debugObjectFaultInjectionCookieSetterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);
    }
#endif

    void JavascriptLibrary::SetDebugObjectDebugModeAccessor(FunctionInfo *funcGetter)
    {
        Assert(funcGetter);

        debugObjectDebugModeGetterFunction = CreateNonProfiledFunction(funcGetter);
        debugObjectDebugModeGetterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);
    }

    bool JavascriptLibrary::InitializeArrayConstructor(DynamicObject* arrayConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(arrayConstructor, mode, 6);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterArray
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = arrayConstructor->GetScriptContext();
        JavascriptLibrary* library = arrayConstructor->GetLibrary();
        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

        library->AddMember(arrayConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(arrayConstructor, PropertyIds::prototype, scriptContext->GetLibrary()->arrayPrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(arrayConstructor, &JavascriptArray::EntryInfo::GetterSymbolSpecies);
        library->AddMember(arrayConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Array), PropertyConfigurable);

#ifdef ENABLE_JS_BUILTINS
        builtinFuncs[BuiltinFunction::JavascriptArray_IsArray] = library->isArrayFunction;
        library->AddMember(arrayConstructor, PropertyIds::isArray, library->isArrayFunction);
#else
        library->AddFunctionToLibraryObject(arrayConstructor, PropertyIds::isArray, &JavascriptArray::EntryInfo::IsArray, 1);
#endif

        library->AddFunctionToLibraryObject(arrayConstructor, PropertyIds::from, &JavascriptArray::EntryInfo::From, 1);
        library->AddFunctionToLibraryObject(arrayConstructor, PropertyIds::of, &JavascriptArray::EntryInfo::Of, 0);

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        arrayConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

#ifdef ENABLE_JS_BUILTINS
    void EnsureBuiltInEngineIsReady(JsBuiltInFile file, ScriptContext* scriptContext)
    {
        if (scriptContext->IsJsBuiltInEnabled())
        {
            if (scriptContext->VerifyAlive())  // Can't initialize if scriptContext closed, will need to run script
            {
                EngineInterfaceObject* engineInterfaceObject = scriptContext->GetLibrary()->GetEngineInterfaceObject();
                Assert(engineInterfaceObject != nullptr);
                JsBuiltInEngineInterfaceExtensionObject* builtInExtension =
                    static_cast<JsBuiltInEngineInterfaceExtensionObject*>(engineInterfaceObject->GetEngineExtension(EngineInterfaceExtensionKind_JsBuiltIn));
                builtInExtension->InjectJsBuiltInLibraryCode(scriptContext, file);
            }
        }
    }
    
    void JavascriptLibrary::EnsureArrayBuiltInsAreReady()
    {
        EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
    }

    void JavascriptLibrary::EnsureMathBuiltInsAreReady()
    {
        EnsureBuiltInEngineIsReady(JsBuiltInFile::Math_object, scriptContext);
    }
#endif

    bool JavascriptLibrary::IsDefaultArrayValuesFunction(RecyclableObject * function, ScriptContext *scriptContext)
    {
#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            ScriptFunction * scriptFunction = JavascriptOperators::TryFromVar<ScriptFunction>(function);
            if (scriptFunction)
            {
                EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
                return scriptFunction->GetFunctionProxy()->IsJsBuiltInCode();
            }
        }
#endif
        JavascriptMethod method = function->GetEntryPoint();
        return method == JavascriptArray::EntryInfo::Values.GetOriginalEntryPoint();
    }

    JavascriptFunction* JavascriptLibrary::EnsureArrayPrototypeForEachFunction()
    {
        if (arrayPrototypeForEachFunction == nullptr)
        {
            arrayPrototypeForEachFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::ForEach, 1, nullptr, nullptr, PropertyIds::forEach);
        }

        return arrayPrototypeForEachFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureArrayPrototypeKeysFunction()
    {
        if (arrayPrototypeKeysFunction == nullptr)
        {
#ifndef ENABLE_JS_BUILTINS
            arrayPrototypeKeysFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::Keys, 0, nullptr, nullptr, PropertyIds::keys);
#else
            if (!scriptContext->IsJsBuiltInEnabled())
            {
                arrayPrototypeKeysFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::Keys, 0, nullptr, nullptr, PropertyIds::keys);
            }
            else
            {
                EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
            }
#endif
        }
        return arrayPrototypeKeysFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureArrayPrototypeValuesFunction()
    {
        if (arrayPrototypeValuesFunction == nullptr)
        {
#ifndef ENABLE_JS_BUILTINS
            arrayPrototypeValuesFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::Values, 0, nullptr, nullptr, PropertyIds::values);
#else
            if (!scriptContext->IsJsBuiltInEnabled())
            {
                arrayPrototypeValuesFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::Values, 0, nullptr, nullptr, PropertyIds::values);
            }
            else
            {
                EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
            }
#endif
        }
        return arrayPrototypeValuesFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureArrayPrototypeEntriesFunction()
    {
        if (arrayPrototypeEntriesFunction == nullptr)
        {
#ifndef ENABLE_JS_BUILTINS
            arrayPrototypeEntriesFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::Entries, 0, nullptr, nullptr, PropertyIds::entries);
#else
            if (!scriptContext->IsJsBuiltInEnabled())
            {
                arrayPrototypeEntriesFunction = DefaultCreateFunction(&JavascriptArray::EntryInfo::Entries, 0, nullptr, nullptr, PropertyIds::entries);
            }
            else
            {
                EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
            }
#endif
        }
        return arrayPrototypeEntriesFunction;
    }

    bool JavascriptLibrary::InitializeArrayPrototype(DynamicObject* arrayPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(arrayPrototype, mode, 27);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterArray
        // so that the update is in sync with profiler

        ScriptContext* scriptContext = arrayPrototype->GetScriptContext();
        JavascriptLibrary* library = arrayPrototype->GetLibrary();

        library->AddMember(arrayPrototype, PropertyIds::constructor, library->arrayConstructor);

        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

        builtinFuncs[BuiltinFunction::JavascriptArray_At]                 = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::at,              &JavascriptArray::EntryInfo::At,                1);
        builtinFuncs[BuiltinFunction::JavascriptArray_Push]               = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::push,            &JavascriptArray::EntryInfo::Push,              1);
        builtinFuncs[BuiltinFunction::JavascriptArray_Concat]             = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::concat,          &JavascriptArray::EntryInfo::Concat,            1);
        builtinFuncs[BuiltinFunction::JavascriptArray_Join]               = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::join,            &JavascriptArray::EntryInfo::Join,              1);
        builtinFuncs[BuiltinFunction::JavascriptArray_Pop]                = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::pop,             &JavascriptArray::EntryInfo::Pop,               0);
        builtinFuncs[BuiltinFunction::JavascriptArray_Reverse]            = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::reverse,         &JavascriptArray::EntryInfo::Reverse,           0);
        builtinFuncs[BuiltinFunction::JavascriptArray_Shift]              = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::shift,           &JavascriptArray::EntryInfo::Shift,             0);
        builtinFuncs[BuiltinFunction::JavascriptArray_Slice]              = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::slice,           &JavascriptArray::EntryInfo::Slice,             2);
        builtinFuncs[BuiltinFunction::JavascriptArray_Splice]             = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::splice,          &JavascriptArray::EntryInfo::Splice,            2);

        // The toString and toLocaleString properties are shared between Array.prototype and %TypedArray%.prototype.
        // Whichever prototype is initialized first will create the functions, the other should just load the existing function objects.
        if (library->arrayPrototypeToStringFunction == nullptr)
        {
            Assert(library->arrayPrototypeToLocaleStringFunction == nullptr);

            library->arrayPrototypeToLocaleStringFunction = /* No inlining       Array_ToLocaleString */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::toLocaleString, &JavascriptArray::EntryInfo::ToLocaleString, 0);
            library->arrayPrototypeToStringFunction =       /* No inlining       Array_ToString       */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::toString, &JavascriptArray::EntryInfo::ToString, 0);
        }
        else
        {
            Assert(library->arrayPrototypeToLocaleStringFunction);

            library->AddMember(arrayPrototype, PropertyIds::toLocaleString, library->arrayPrototypeToLocaleStringFunction);
            library->AddMember(arrayPrototype, PropertyIds::toString, library->arrayPrototypeToStringFunction);
        }

        builtinFuncs[BuiltinFunction::JavascriptArray_Unshift]            = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::unshift,         &JavascriptArray::EntryInfo::Unshift,           1);

        if (!scriptContext->IsJsBuiltInEnabled())
        {
            builtinFuncs[BuiltinFunction::JavascriptArray_IndexOf] = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::indexOf, &JavascriptArray::EntryInfo::IndexOf, 1);
            builtinFuncs[BuiltinFunction::JavascriptArray_Includes] = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::includes, &JavascriptArray::EntryInfo::Includes, 1);
            /* No inlining                Array_Sort               */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::sort, &JavascriptArray::EntryInfo::Sort, 1);
        }

        builtinFuncs[BuiltinFunction::JavascriptArray_LastIndexOf]    = library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::lastIndexOf,     &JavascriptArray::EntryInfo::LastIndexOf,       1);
        /* No inlining                Array_Map            */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::map,             &JavascriptArray::EntryInfo::Map,               1);
        /* No inlining                Array_ReduceRight    */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::reduceRight,     &JavascriptArray::EntryInfo::ReduceRight,       1);

        if (scriptContext->GetConfig()->IsES6StringExtensionsEnabled()) // This is not a typo, Array.prototype.find and .findIndex are part of the ES6 Improved String APIs feature
        {
            /* No inlining            Array_Find           */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::find,            &JavascriptArray::EntryInfo::Find,              1);
            /* No inlining            Array_FindIndex      */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::findIndex,       &JavascriptArray::EntryInfo::FindIndex,         1);
        }
        if (scriptContext->GetConfig()->IsESArrayFindFromLastEnabled())
        {
            /* No inlining            Array_FindLast           */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::findLast, &JavascriptArray::EntryInfo::FindLast, 1);
            /* No inlining            Array_FindLastIndex      */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::findLastIndex, &JavascriptArray::EntryInfo::FindLastIndex, 1);
        }

#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
        }
        else
#endif
        {
            /* No inlining                Array_Entries        */
            library->AddMember(arrayPrototype, PropertyIds::entries, library->EnsureArrayPrototypeEntriesFunction());

            /* No inlining                Array_Keys           */
            library->AddMember(arrayPrototype, PropertyIds::keys, library->EnsureArrayPrototypeKeysFunction());

            JavascriptFunction *values = library->EnsureArrayPrototypeValuesFunction();
            /* No inlining                Array_Values         */ library->AddMember(arrayPrototype, PropertyIds::values, values);
            /* No inlining                Array_SymbolIterator */ library->AddMember(arrayPrototype, PropertyIds::_symbolIterator, values);

            /* No inlining                Array_Filter         */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::filter, &JavascriptArray::EntryInfo::Filter, 1);
            /* No inlining                Array_ForEach        */ library->AddMember(arrayPrototype, PropertyIds::forEach, library->EnsureArrayPrototypeForEachFunction());
            /* No inlining                Array_Some           */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::some, &JavascriptArray::EntryInfo::Some, 1);
            /* No inlining                Array_Reduce         */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::reduce, &JavascriptArray::EntryInfo::Reduce, 1);
            /* No inlining                Array_Every          */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::every, &JavascriptArray::EntryInfo::Every, 1);
        }

        DynamicType* dynamicType = DynamicType::New(scriptContext, TypeIds_Object, library->nullValue, nullptr, NullTypeHandler<false>::GetDefaultInstance(), false);
        DynamicObject* unscopablesList = DynamicObject::New(library->GetRecycler(), dynamicType);
        unscopablesList->SetProperty(PropertyIds::at,               JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::copyWithin,       JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::entries,          JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::fill,             JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::find,             JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::findIndex,        JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::flat,             JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::flatMap,          JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::includes,         JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::keys,             JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        unscopablesList->SetProperty(PropertyIds::values,           JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);

        if (scriptContext->GetConfig()->IsESArrayFindFromLastEnabled())
        {
            unscopablesList->SetProperty(PropertyIds::findLast, JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
            unscopablesList->SetProperty(PropertyIds::findLastIndex, JavascriptBoolean::ToVar(true, scriptContext), PropertyOperation_None, nullptr);
        }

        library->AddMember(arrayPrototype, PropertyIds::_symbolUnscopables, unscopablesList, PropertyConfigurable);

        /* No inlining            Array_Fill           */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::fill, &JavascriptArray::EntryInfo::Fill, 1);
        /* No inlining            Array_CopyWithin     */ library->AddFunctionToLibraryObject(arrayPrototype, PropertyIds::copyWithin, &JavascriptArray::EntryInfo::CopyWithin, 2);
        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        arrayPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeSharedArrayBufferConstructor(DynamicObject* sharedArrayBufferConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(sharedArrayBufferConstructor, mode, 4);

        ScriptContext* scriptContext = sharedArrayBufferConstructor->GetScriptContext();
        JavascriptLibrary* library = sharedArrayBufferConstructor->GetLibrary();
        library->AddMember(sharedArrayBufferConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(sharedArrayBufferConstructor, PropertyIds::prototype, scriptContext->GetLibrary()->sharedArrayBufferPrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(sharedArrayBufferConstructor, &SharedArrayBuffer::EntryInfo::GetterSymbolSpecies);
        library->AddMember(sharedArrayBufferConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::SharedArrayBuffer), PropertyConfigurable);

        sharedArrayBufferConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeSharedArrayBufferPrototype(DynamicObject* sharedArrayBufferPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(sharedArrayBufferPrototype, mode, 4);

        ScriptContext* scriptContext = sharedArrayBufferPrototype->GetScriptContext();
        JavascriptLibrary* library = sharedArrayBufferPrototype->GetLibrary();
        library->AddMember(sharedArrayBufferPrototype, PropertyIds::constructor, library->sharedArrayBufferConstructor);
        library->AddFunctionToLibraryObject(sharedArrayBufferPrototype, PropertyIds::slice, &SharedArrayBuffer::EntryInfo::Slice, 2);
        library->AddAccessorsToLibraryObject(sharedArrayBufferPrototype, PropertyIds::byteLength, &SharedArrayBuffer::EntryInfo::GetterByteLength, nullptr);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(sharedArrayBufferPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("SharedArrayBuffer")), PropertyConfigurable);
        }

        sharedArrayBufferPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeAtomicsObject(DynamicObject* atomicsObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(atomicsObject, mode, 12);

        JavascriptLibrary* library = atomicsObject->GetLibrary();

        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::add, &AtomicsObject::EntryInfo::Add, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::and_, &AtomicsObject::EntryInfo::And, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::compareExchange, &AtomicsObject::EntryInfo::CompareExchange, 4);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::exchange, &AtomicsObject::EntryInfo::Exchange, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::isLockFree, &AtomicsObject::EntryInfo::IsLockFree, 1);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::load, &AtomicsObject::EntryInfo::Load, 2);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::or_, &AtomicsObject::EntryInfo::Or, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::store, &AtomicsObject::EntryInfo::Store, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::sub, &AtomicsObject::EntryInfo::Sub, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::wait, &AtomicsObject::EntryInfo::Wait, 4);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::notify, &AtomicsObject::EntryInfo::Notify, 3);
        library->AddFunctionToLibraryObject(atomicsObject, PropertyIds::xor_, &AtomicsObject::EntryInfo::Xor, 3);

        if (atomicsObject->GetScriptContext()->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(atomicsObject, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Atomics")), PropertyConfigurable);
        }

        atomicsObject->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeArrayBufferConstructor(DynamicObject* arrayBufferConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(arrayBufferConstructor, mode, 4);

        ScriptContext* scriptContext = arrayBufferConstructor->GetScriptContext();
        JavascriptLibrary* library = arrayBufferConstructor->GetLibrary();
        library->AddMember(arrayBufferConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(arrayBufferConstructor, PropertyIds::prototype, scriptContext->GetLibrary()->arrayBufferPrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(arrayBufferConstructor, &ArrayBuffer::EntryInfo::GetterSymbolSpecies);
        library->AddMember(arrayBufferConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::ArrayBuffer), PropertyConfigurable);

        library->AddFunctionToLibraryObject(arrayBufferConstructor, PropertyIds::isView, &ArrayBuffer::EntryInfo::IsView, 1);

#if ENABLE_DEBUG_CONFIG_OPTIONS
        library->AddFunctionToLibraryObject(arrayBufferConstructor, PropertyIds::detach, &ArrayBuffer::EntryInfo::Detach, 1);
#endif
        arrayBufferConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeArrayBufferPrototype(DynamicObject* arrayBufferPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(arrayBufferPrototype, mode, 2);

        ScriptContext* scriptContext = arrayBufferPrototype->GetScriptContext();
        JavascriptLibrary* library = arrayBufferPrototype->GetLibrary();
        library->AddMember(arrayBufferPrototype, PropertyIds::constructor, library->arrayBufferConstructor);

        library->AddFunctionToLibraryObject(arrayBufferPrototype, PropertyIds::slice, &ArrayBuffer::EntryInfo::Slice, 2);
        library->AddAccessorsToLibraryObject(arrayBufferPrototype, PropertyIds::byteLength, &ArrayBuffer::EntryInfo::GetterByteLength, nullptr);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(arrayBufferPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("ArrayBuffer")), PropertyConfigurable);
        }

        arrayBufferPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeDataViewConstructor(DynamicObject* dataViewConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(dataViewConstructor, mode, 3);

        ScriptContext* scriptContext = dataViewConstructor->GetScriptContext();
        JavascriptLibrary* library = dataViewConstructor->GetLibrary();
        library->AddMember(dataViewConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(3), PropertyConfigurable);
        library->AddMember(dataViewConstructor, PropertyIds::prototype, scriptContext->GetLibrary()->dataViewPrototype, PropertyNone);
        library->AddMember(dataViewConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::DataView), PropertyConfigurable);

        dataViewConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeDataViewPrototype(DynamicObject* dataViewPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(dataViewPrototype, mode, 2);

        ScriptContext* scriptContext = dataViewPrototype->GetScriptContext();
        JavascriptLibrary* library = dataViewPrototype->GetLibrary();
        library->AddMember(dataViewPrototype, PropertyIds::constructor, library->dataViewConstructor);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setInt8, &DataView::EntryInfo::SetInt8, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setUint8, &DataView::EntryInfo::SetUint8, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setInt16, &DataView::EntryInfo::SetInt16, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setUint16, &DataView::EntryInfo::SetUint16, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setInt32, &DataView::EntryInfo::SetInt32, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setUint32, &DataView::EntryInfo::SetUint32, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setFloat32, &DataView::EntryInfo::SetFloat32, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::setFloat64, &DataView::EntryInfo::SetFloat64, 2);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getInt8, &DataView::EntryInfo::GetInt8, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getUint8, &DataView::EntryInfo::GetUint8, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getInt16, &DataView::EntryInfo::GetInt16, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getUint16, &DataView::EntryInfo::GetUint16, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getInt32, &DataView::EntryInfo::GetInt32, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getUint32, &DataView::EntryInfo::GetUint32, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getFloat32, &DataView::EntryInfo::GetFloat32, 1);
        library->AddFunctionToLibraryObject(dataViewPrototype, PropertyIds::getFloat64, &DataView::EntryInfo::GetFloat64, 1);

        library->AddAccessorsToLibraryObject(dataViewPrototype, PropertyIds::buffer, &DataView::EntryInfo::GetterBuffer, nullptr);
        library->AddAccessorsToLibraryObject(dataViewPrototype, PropertyIds::byteLength, &DataView::EntryInfo::GetterByteLength, nullptr);
        library->AddAccessorsToLibraryObject(dataViewPrototype, PropertyIds::byteOffset, &DataView::EntryInfo::GetterByteOffset, nullptr);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(dataViewPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("DataView")), PropertyConfigurable);
        }

        dataViewPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeTypedArrayConstructor(DynamicObject* typedArrayConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(typedArrayConstructor, mode, 5);

        JavascriptLibrary* library = typedArrayConstructor->GetLibrary();

        library->AddMember(typedArrayConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(3), PropertyConfigurable);
        library->AddMember(typedArrayConstructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("TypedArray")), PropertyConfigurable);
        library->AddMember(typedArrayConstructor, PropertyIds::prototype, library->typedArrayPrototype, PropertyNone);

        library->AddFunctionToLibraryObject(typedArrayConstructor, PropertyIds::from, &TypedArrayBase::EntryInfo::From, 1);
        library->AddFunctionToLibraryObject(typedArrayConstructor, PropertyIds::of, &TypedArrayBase::EntryInfo::Of, 0);
        library->AddSpeciesAccessorsToLibraryObject(typedArrayConstructor, &TypedArrayBase::EntryInfo::GetterSymbolSpecies);

        typedArrayConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeTypedArrayPrototype(DynamicObject* typedarrayPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(typedarrayPrototype, mode, 31);

        ScriptContext* scriptContext = typedarrayPrototype->GetScriptContext();
        JavascriptLibrary* library = typedarrayPrototype->GetLibrary();

#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            EnsureBuiltInEngineIsReady(JsBuiltInFile::Array_prototype, scriptContext);
        }
#endif

        library->AddMember(typedarrayPrototype, PropertyIds::constructor, library->typedArrayConstructor);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::set, &TypedArrayBase::EntryInfo::Set, 2);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::subarray, &TypedArrayBase::EntryInfo::Subarray, 2);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::at, &TypedArrayBase::EntryInfo::At, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::copyWithin, &TypedArrayBase::EntryInfo::CopyWithin, 2);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::every, &TypedArrayBase::EntryInfo::Every, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::fill, &TypedArrayBase::EntryInfo::Fill, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::filter, &TypedArrayBase::EntryInfo::Filter, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::find, &TypedArrayBase::EntryInfo::Find, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::findIndex, &TypedArrayBase::EntryInfo::FindIndex, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::forEach, &TypedArrayBase::EntryInfo::ForEach, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::indexOf, &TypedArrayBase::EntryInfo::IndexOf, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::join, &TypedArrayBase::EntryInfo::Join, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::lastIndexOf, &TypedArrayBase::EntryInfo::LastIndexOf, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::map, &TypedArrayBase::EntryInfo::Map, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::reduce, &TypedArrayBase::EntryInfo::Reduce, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::reduceRight, &TypedArrayBase::EntryInfo::ReduceRight, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::reverse, &TypedArrayBase::EntryInfo::Reverse, 0);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::slice, &TypedArrayBase::EntryInfo::Slice, 2);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::some, &TypedArrayBase::EntryInfo::Some, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::sort, &TypedArrayBase::EntryInfo::Sort, 1);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::entries, &TypedArrayBase::EntryInfo::Entries, 0);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::keys, &TypedArrayBase::EntryInfo::Keys, 0);
        JavascriptFunction* valuesFunc = library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::values, &TypedArrayBase::EntryInfo::Values, 0);
        library->AddMember(typedarrayPrototype, PropertyIds::_symbolIterator, valuesFunc);
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::includes, &TypedArrayBase::EntryInfo::Includes, 1);

        library->AddAccessorsToLibraryObject(typedarrayPrototype, PropertyIds::buffer, &TypedArrayBase::EntryInfo::GetterBuffer, nullptr);
        library->AddAccessorsToLibraryObject(typedarrayPrototype, PropertyIds::byteLength, &TypedArrayBase::EntryInfo::GetterByteLength, nullptr);
        library->AddAccessorsToLibraryObject(typedarrayPrototype, PropertyIds::byteOffset, &TypedArrayBase::EntryInfo::GetterByteOffset, nullptr);
        library->AddAccessorsToLibraryObject(typedarrayPrototype, PropertyIds::length, &TypedArrayBase::EntryInfo::GetterLength, nullptr);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            // ES2017 22.2.3.32 get %TypedArray%.prototype[@@toStringTag]
            // %TypedArray%.prototype[@@toStringTag] is an accessor property whose set accessor function is undefined.
            // This property has the attributes { [[Enumerable]]: false, [[Configurable]]: true }.
            // The initial value of the name property of this function is "get [Symbol.toStringTag]".
            library->AddAccessorsToLibraryObjectWithName(typedarrayPrototype, PropertyIds::_symbolToStringTag,
                PropertyIds::_RuntimeFunctionNameId_toStringTag, &TypedArrayBase::EntryInfo::GetterSymbolToStringTag, nullptr);
        }
        // The toString and toLocaleString properties are shared between Array.prototype and %TypedArray%.prototype.
        // Whichever prototype is initialized first will create the functions, the other should just load the existing function objects.
        if (library->arrayPrototypeToStringFunction == nullptr)
        {
            Assert(library->arrayPrototypeToLocaleStringFunction == nullptr);

            library->arrayPrototypeToLocaleStringFunction = library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::toLocaleString, &JavascriptArray::EntryInfo::ToLocaleString, 0);
            library->arrayPrototypeToStringFunction = library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::toString, &JavascriptArray::EntryInfo::ToString, 0);
        }
        else
        {
            Assert(library->arrayPrototypeToLocaleStringFunction);

            library->AddMember(typedarrayPrototype, PropertyIds::toLocaleString, library->arrayPrototypeToLocaleStringFunction);
            library->AddMember(typedarrayPrototype, PropertyIds::toString, library->arrayPrototypeToStringFunction);
        }

        if (scriptContext->GetConfig()->IsESArrayFindFromLastEnabled()) {
            library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::findLast, &TypedArrayBase::EntryInfo::FindLast, 1);
            library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::findLastIndex, &TypedArrayBase::EntryInfo::FindLastIndex, 1);
        }

        typedarrayPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

#define INIT_TYPEDARRAY_CONSTRUCTOR(typedArray, typedarrayPrototype, TypeName) \
    bool  JavascriptLibrary::Initialize##typedArray##Constructor(DynamicObject* typedArrayConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode) \
    { \
        typeHandler->Convert(typedArrayConstructor, mode, 4); \
        ScriptContext* scriptContext = typedArrayConstructor->GetScriptContext(); \
        JavascriptLibrary* library = typedArrayConstructor->GetLibrary(); \
        library->AddMember(typedArrayConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(3), PropertyConfigurable); \
        library->AddMember(typedArrayConstructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u(#typedArray)), PropertyConfigurable); \
        library->AddMember(typedArrayConstructor, PropertyIds::BYTES_PER_ELEMENT, TaggedInt::ToVarUnchecked(sizeof(TypeName)), PropertyNone); \
        library->AddMember(typedArrayConstructor, PropertyIds::prototype, scriptContext->GetLibrary()->##typedarrayPrototype##, PropertyNone); \
        typedArrayConstructor->SetHasNoEnumerableProperties(true); \
        return true; \
    } \

    INIT_TYPEDARRAY_CONSTRUCTOR(Int8Array, Int8ArrayPrototype, int8);
    INIT_TYPEDARRAY_CONSTRUCTOR(Uint8Array, Uint8ArrayPrototype, uint8);
    INIT_TYPEDARRAY_CONSTRUCTOR(Uint8ClampedArray, Uint8ClampedArrayPrototype, uint8);
    INIT_TYPEDARRAY_CONSTRUCTOR(Int16Array, Int16ArrayPrototype, int16);
    INIT_TYPEDARRAY_CONSTRUCTOR(Uint16Array, Uint16ArrayPrototype, uint16);
    INIT_TYPEDARRAY_CONSTRUCTOR(Int32Array, Int32ArrayPrototype, int32);
    INIT_TYPEDARRAY_CONSTRUCTOR(Uint32Array, Uint32ArrayPrototype, uint32);
    INIT_TYPEDARRAY_CONSTRUCTOR(Float32Array, Float32ArrayPrototype, float);
    INIT_TYPEDARRAY_CONSTRUCTOR(Float64Array, Float64ArrayPrototype, double);

#define INIT_TYPEDARRAY_PROTOTYPE(typedArray, typedarrayPrototype, TypeName) \
    bool JavascriptLibrary::Initialize##typedarrayPrototype##(DynamicObject* typedarrayPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode) \
    { \
        typeHandler->Convert(typedarrayPrototype, mode, 2); \
        JavascriptLibrary* library = typedarrayPrototype->GetLibrary(); \
        library->AddMember(typedarrayPrototype, PropertyIds::constructor, library->##typedArray##Constructor); \
        library->AddMember(typedarrayPrototype, PropertyIds::BYTES_PER_ELEMENT, TaggedInt::ToVarUnchecked(sizeof(TypeName)), PropertyNone); \
        typedarrayPrototype->SetHasNoEnumerableProperties(true); \
        return true; \
    } \

    INIT_TYPEDARRAY_PROTOTYPE(Int8Array, Int8ArrayPrototype, int8);
    INIT_TYPEDARRAY_PROTOTYPE(Uint8Array, Uint8ArrayPrototype, uint8);
    INIT_TYPEDARRAY_PROTOTYPE(Uint8ClampedArray, Uint8ClampedArrayPrototype, uint8);
    INIT_TYPEDARRAY_PROTOTYPE(Int16Array, Int16ArrayPrototype, int16);
    INIT_TYPEDARRAY_PROTOTYPE(Uint16Array, Uint16ArrayPrototype, uint16);
    INIT_TYPEDARRAY_PROTOTYPE(Int32Array, Int32ArrayPrototype, int32);
    INIT_TYPEDARRAY_PROTOTYPE(Uint32Array, Uint32ArrayPrototype, uint32);
    INIT_TYPEDARRAY_PROTOTYPE(Float32Array, Float32ArrayPrototype, float);
    INIT_TYPEDARRAY_PROTOTYPE(Float64Array, Float64ArrayPrototype, double);

    // For Microsoft extension typed array, like Int64Array, BoolArray, we don't have constructor as they can only be created from the projection arguments.
    // there is no subarray method either as that's another way to create typed array.
#define INIT_MSINTERNAL_TYPEDARRAY_PROTOTYPE(typedArray, typedarrayPrototype) \
    bool JavascriptLibrary::Initialize##typedarrayPrototype##(DynamicObject* typedarrayPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)  \
    {   \
        typeHandler->Convert(typedarrayPrototype, mode, 1); \
        JavascriptLibrary* library = typedarrayPrototype->GetLibrary(); \
        library->AddFunctionToLibraryObject(typedarrayPrototype, PropertyIds::set, &##typedArray##::EntryInfo::Set, 2); \
        typedarrayPrototype->SetHasNoEnumerableProperties(true); \
        return true; \
    }   \

    INIT_MSINTERNAL_TYPEDARRAY_PROTOTYPE(Int64Array, Int64ArrayPrototype);
    INIT_MSINTERNAL_TYPEDARRAY_PROTOTYPE(Uint64Array, Uint64ArrayPrototype);
    INIT_MSINTERNAL_TYPEDARRAY_PROTOTYPE(BoolArray, BoolArrayPrototype);
    INIT_MSINTERNAL_TYPEDARRAY_PROTOTYPE(CharArray, CharArrayPrototype);

    bool JavascriptLibrary::InitializeErrorConstructor(DynamicObject* constructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(constructor, mode, 4);

        ScriptContext* scriptContext = constructor->GetScriptContext();
        JavascriptLibrary* library = constructor->GetLibrary();

        library->AddMember(constructor, PropertyIds::prototype, library->errorPrototype, PropertyNone);
        library->AddMember(constructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(constructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Error), PropertyConfigurable);

        if (scriptContext->GetConfig()->IsErrorStackTraceEnabled())
        {
            library->AddMember(constructor, PropertyIds::stackTraceLimit, JavascriptNumber::ToVar(JavascriptExceptionOperators::DefaultStackTraceLimit, scriptContext), PropertyConfigurable | PropertyWritable | PropertyEnumerable);
        }

        constructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeErrorPrototype(DynamicObject* prototype, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(prototype, mode, 4);

        JavascriptLibrary* library = prototype->GetLibrary();

        library->AddMember(prototype, PropertyIds::constructor, library->errorConstructor);

        bool hasNoEnumerableProperties = true;
        PropertyAttributes prototypeNameMessageAttributes = PropertyConfigurable | PropertyWritable;

        library->AddMember(prototype, PropertyIds::name, library->CreateStringFromCppLiteral(_u("Error")), prototypeNameMessageAttributes);
        library->AddMember(prototype, PropertyIds::message, library->GetEmptyString(), prototypeNameMessageAttributes);
        library->AddFunctionToLibraryObject(prototype, PropertyIds::toString, &JavascriptError::EntryInfo::ToString, 0);

        prototype->SetHasNoEnumerableProperties(hasNoEnumerableProperties);

        return true;
    }

#define INIT_ERROR_IMPL(error, errorName, ctorLength) \
    bool JavascriptLibrary::Initialize##error##Constructor(DynamicObject* constructor, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode) \
    { \
        typeHandler->Convert(constructor, mode, 3); \
        JavascriptLibrary* library = constructor->GetLibrary(); \
        library->AddMember(constructor, PropertyIds::prototype, library->Get##error##Prototype(), PropertyNone); \
        library->AddMember(constructor, PropertyIds::length, TaggedInt::ToVarUnchecked(ctorLength), PropertyConfigurable); \
        PropertyAttributes prototypeNameMessageAttributes = PropertyConfigurable; \
        library->AddMember(constructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u(#errorName)), prototypeNameMessageAttributes); \
        constructor->SetHasNoEnumerableProperties(true); \
        return true; \
    } \
    bool JavascriptLibrary::Initialize##error##Prototype(DynamicObject* prototype, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode) \
    { \
        typeHandler->Convert(prototype, mode, 3); \
        JavascriptLibrary* library = prototype->GetLibrary(); \
        library->AddMember(prototype, PropertyIds::constructor, library->Get##error##Constructor()); \
        bool hasNoEnumerableProperties = true; \
        PropertyAttributes prototypeNameMessageAttributes = PropertyConfigurable | PropertyWritable; \
        library->AddMember(prototype, PropertyIds::name, library->CreateStringFromCppLiteral(_u(#errorName)), prototypeNameMessageAttributes); \
        library->AddMember(prototype, PropertyIds::message, library->GetEmptyString(), prototypeNameMessageAttributes); \
        prototype->SetHasNoEnumerableProperties(hasNoEnumerableProperties); \
        return true; \
    }

#define INIT_ERROR(error) INIT_ERROR_IMPL(error, error, 1)
    INIT_ERROR(EvalError);
    INIT_ERROR(RangeError);
    INIT_ERROR(ReferenceError);
    INIT_ERROR(SyntaxError);
    INIT_ERROR(TypeError);
    INIT_ERROR(URIError);
    INIT_ERROR_IMPL(AggregateError, AggregateError, 2);
    INIT_ERROR_IMPL(WebAssemblyCompileError, CompileError, 1);
    INIT_ERROR_IMPL(WebAssemblyRuntimeError, RuntimeError, 1);
    INIT_ERROR_IMPL(WebAssemblyLinkError, LinkError, 1);

#undef INIT_ERROR

    bool JavascriptLibrary::InitializeBooleanConstructor(DynamicObject* booleanConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(booleanConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterBoolean
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = booleanConstructor->GetScriptContext();
        JavascriptLibrary* library = booleanConstructor->GetLibrary();
        library->AddMember(booleanConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(booleanConstructor, PropertyIds::prototype, library->booleanPrototype, PropertyNone);
        library->AddMember(booleanConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Boolean), PropertyConfigurable);

        booleanConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeBooleanPrototype(DynamicObject* booleanPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(booleanPrototype, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterBoolean
        // so that the update is in sync with profiler
        JavascriptLibrary* library = booleanPrototype->GetLibrary();
        ScriptContext* scriptContext = booleanPrototype->GetScriptContext();
        library->AddMember(booleanPrototype, PropertyIds::constructor, library->booleanConstructor);
        scriptContext->SetBuiltInLibraryFunction(JavascriptBoolean::EntryInfo::ValueOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(booleanPrototype, PropertyIds::valueOf, &JavascriptBoolean::EntryInfo::ValueOf, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptBoolean::EntryInfo::ToString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(booleanPrototype, PropertyIds::toString, &JavascriptBoolean::EntryInfo::ToString, 0));
        booleanPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeSymbolConstructor(DynamicObject* symbolConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(symbolConstructor, mode, 16);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSymbol
        // so that the update is in sync with profiler
        JavascriptLibrary* library = symbolConstructor->GetLibrary();
        ScriptContext* scriptContext = symbolConstructor->GetScriptContext();
        library->AddMember(symbolConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable);
        library->AddMember(symbolConstructor, PropertyIds::prototype, library->symbolPrototype, PropertyNone);
        library->AddMember(symbolConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Symbol), PropertyConfigurable);
        if (scriptContext->GetConfig()->IsES6HasInstanceEnabled())
        {
            library->AddMember(symbolConstructor, PropertyIds::hasInstance, library->GetSymbolHasInstance(), PropertyNone);
        }
        if (scriptContext->GetConfig()->IsES6IsConcatSpreadableEnabled())
        {
            library->AddMember(symbolConstructor, PropertyIds::isConcatSpreadable, library->GetSymbolIsConcatSpreadable(), PropertyNone);
        }
        library->AddMember(symbolConstructor, PropertyIds::iterator, library->GetSymbolIterator(), PropertyNone);
        if (scriptContext->GetConfig()->IsES2018AsyncIterationEnabled())
        {
            library->AddMember(symbolConstructor, PropertyIds::asyncIterator, library->GetSymbolAsyncIterator(), PropertyNone);
        }

        library->AddMember(symbolConstructor, PropertyIds::species, library->GetSymbolSpecies(), PropertyNone);

        if (scriptContext->GetConfig()->IsES6ToPrimitiveEnabled())
        {
            library->AddMember(symbolConstructor, PropertyIds::toPrimitive, library->GetSymbolToPrimitive(), PropertyNone);
        }

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(symbolConstructor, PropertyIds::toStringTag, library->GetSymbolToStringTag(), PropertyNone);
        }
        library->AddMember(symbolConstructor, PropertyIds::unscopables, library->GetSymbolUnscopables(), PropertyNone);

        if (scriptContext->GetConfig()->IsES6RegExSymbolsEnabled())
        {
            library->AddMember(symbolConstructor, PropertyIds::match, library->GetSymbolMatch(), PropertyNone);
            library->AddMember(symbolConstructor, PropertyIds::replace, library->GetSymbolReplace(), PropertyNone);
            library->AddMember(symbolConstructor, PropertyIds::search, library->GetSymbolSearch(), PropertyNone);
            library->AddMember(symbolConstructor, PropertyIds::split, library->GetSymbolSplit(), PropertyNone);
        }

        library->AddFunctionToLibraryObject(symbolConstructor, PropertyIds::for_, &JavascriptSymbol::EntryInfo::For, 1);
        library->AddFunctionToLibraryObject(symbolConstructor, PropertyIds::keyFor, &JavascriptSymbol::EntryInfo::KeyFor, 1);

        symbolConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeSymbolPrototype(DynamicObject* symbolPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(symbolPrototype, mode, 6);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSymbol
        // so that the update is in sync with profiler
        JavascriptLibrary* library = symbolPrototype->GetLibrary();
        ScriptContext* scriptContext = symbolPrototype->GetScriptContext();

        library->AddMember(symbolPrototype, PropertyIds::constructor, library->symbolConstructor);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(symbolPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Symbol")), PropertyConfigurable);
        }
        scriptContext->SetBuiltInLibraryFunction(JavascriptSymbol::EntryInfo::ValueOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(symbolPrototype, PropertyIds::valueOf, &JavascriptSymbol::EntryInfo::ValueOf, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptSymbol::EntryInfo::ToString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(symbolPrototype, PropertyIds::toString, &JavascriptSymbol::EntryInfo::ToString, 0));

        if (scriptContext->GetConfig()->IsES6ToPrimitiveEnabled())
        {
            scriptContext->SetBuiltInLibraryFunction(JavascriptSymbol::EntryInfo::SymbolToPrimitive.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObjectWithName(symbolPrototype, PropertyIds::_symbolToPrimitive, PropertyIds::_RuntimeFunctionNameId_toPrimitive,
                &JavascriptSymbol::EntryInfo::SymbolToPrimitive, 1));
            symbolPrototype->SetWritable(PropertyIds::_symbolToPrimitive, false);
        }

        if (scriptContext->GetConfig()->IsESSymbolDescriptionEnabled())
        {
            library->AddAccessorsToLibraryObject(symbolPrototype, PropertyIds::description, &JavascriptSymbol::EntryInfo::Description, nullptr);
        }

        symbolPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializePromiseConstructor(DynamicObject* promiseConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(promiseConstructor, mode, 9);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterPromise
        // so that the update is in sync with profiler
        JavascriptLibrary* library = promiseConstructor->GetLibrary();
        ScriptContext* scriptContext = promiseConstructor->GetScriptContext();
        library->AddMember(promiseConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(promiseConstructor, PropertyIds::prototype, library->promisePrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(promiseConstructor, &JavascriptPromise::EntryInfo::GetterSymbolSpecies);
        library->AddMember(promiseConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Promise), PropertyConfigurable);

        library->AddFunctionToLibraryObject(promiseConstructor, PropertyIds::all, &JavascriptPromise::EntryInfo::All, 1);
        library->AddFunctionToLibraryObject(promiseConstructor, PropertyIds::allSettled, &JavascriptPromise::EntryInfo::AllSettled, 1);
        if (scriptContext->GetConfig()->IsESPromiseAnyEnabled())
        {
            library->AddFunctionToLibraryObject(promiseConstructor, PropertyIds::any, &JavascriptPromise::EntryInfo::Any, 1);
        }
        library->AddFunctionToLibraryObject(promiseConstructor, PropertyIds::race, &JavascriptPromise::EntryInfo::Race, 1);
        library->AddFunctionToLibraryObject(promiseConstructor, PropertyIds::reject, &JavascriptPromise::EntryInfo::Reject, 1);
        library->AddMember(promiseConstructor, PropertyIds::resolve, library->EnsurePromiseResolveFunction(), PropertyBuiltInMethodDefaults);

        promiseConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    JavascriptFunction* JavascriptLibrary::EnsurePromiseResolveFunction()
    {
        if (promiseResolveFunction == nullptr)
        {
            promiseResolveFunction = DefaultCreateFunction(&JavascriptPromise::EntryInfo::Resolve, 1, nullptr, nullptr, PropertyIds::resolve);
        }
        return promiseResolveFunction;
    }

    bool JavascriptLibrary::InitializePromisePrototype(DynamicObject* promisePrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(promisePrototype, mode, 4);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterPromise
        // so that the update is in sync with profiler
        JavascriptLibrary* library = promisePrototype->GetLibrary();
        ScriptContext* scriptContext = promisePrototype->GetScriptContext();

        library->AddMember(promisePrototype, PropertyIds::constructor, library->promiseConstructor);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(promisePrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Promise")), PropertyConfigurable);
        }
        scriptContext->SetBuiltInLibraryFunction(JavascriptPromise::EntryInfo::Catch.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(promisePrototype, PropertyIds::catch_, &JavascriptPromise::EntryInfo::Catch, 1));

        scriptContext->SetBuiltInLibraryFunction(JavascriptPromise::EntryInfo::Finally.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(promisePrototype, PropertyIds::finally, &JavascriptPromise::EntryInfo::Finally, 1));

        library->AddMember(promisePrototype, PropertyIds::then, library->EnsurePromiseThenFunction(), PropertyBuiltInMethodDefaults);
        scriptContext->SetBuiltInLibraryFunction(JavascriptPromise::EntryInfo::Then.GetOriginalEntryPoint(), library->EnsurePromiseThenFunction());

        promisePrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    JavascriptFunction* JavascriptLibrary::EnsurePromiseThenFunction()
    {
        if (promiseThenFunction == nullptr)
        {
            promiseThenFunction = DefaultCreateFunction(&JavascriptPromise::EntryInfo::Then, 2, nullptr, nullptr, PropertyIds::then);
        }

        return promiseThenFunction;
    }

    bool JavascriptLibrary::InitializeGeneratorFunctionConstructor(DynamicObject* generatorFunctionConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(generatorFunctionConstructor, mode, 3);
        JavascriptLibrary* library = generatorFunctionConstructor->GetLibrary();

        library->AddMember(generatorFunctionConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(generatorFunctionConstructor, PropertyIds::prototype, library->generatorFunctionPrototype, PropertyNone);
        library->AddMember(generatorFunctionConstructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("GeneratorFunction")), PropertyConfigurable);

        generatorFunctionConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeGeneratorFunctionPrototype(DynamicObject* generatorFunctionPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(generatorFunctionPrototype, mode, 3);
        JavascriptLibrary* library = generatorFunctionPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();

        library->AddMember(generatorFunctionPrototype, PropertyIds::constructor, library->generatorFunctionConstructor, PropertyConfigurable);
        library->AddMember(generatorFunctionPrototype, PropertyIds::prototype, library->generatorPrototype, PropertyConfigurable);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(generatorFunctionPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("GeneratorFunction")), PropertyConfigurable);
        }
        generatorFunctionPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeGeneratorPrototype(DynamicObject* generatorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(generatorPrototype, mode, 5);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterGenerator
        // so that the update is in sync with profiler
        JavascriptLibrary* library = generatorPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();

        library->AddMember(generatorPrototype, PropertyIds::constructor, library->generatorFunctionPrototype, PropertyConfigurable);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled()) {
            library->AddMember(generatorPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Generator")), PropertyConfigurable);
        }
        library->AddMember(generatorPrototype, PropertyIds::return_, library->EnsureGeneratorReturnFunction(), PropertyBuiltInMethodDefaults);
        library->AddMember(generatorPrototype, PropertyIds::next, library->EnsureGeneratorNextFunction(), PropertyBuiltInMethodDefaults);
        library->AddMember(generatorPrototype, PropertyIds::throw_, library->EnsureGeneratorThrowFunction(), PropertyBuiltInMethodDefaults);

        generatorPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    JavascriptFunction* JavascriptLibrary::EnsureGeneratorReturnFunction()
    {
        if (generatorReturnFunction == nullptr)
        {
            generatorReturnFunction = DefaultCreateFunction(&JavascriptGenerator::EntryInfo::Return, 1, nullptr, nullptr, PropertyIds::return_);
        }
        return generatorReturnFunction;
    }


    JavascriptFunction* JavascriptLibrary::EnsureGeneratorNextFunction()
    {
        if (generatorNextFunction == nullptr)
        {
            generatorNextFunction = DefaultCreateFunction(&JavascriptGenerator::EntryInfo::Next, 1, nullptr, nullptr, PropertyIds::next);
        }
        return generatorNextFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureGeneratorThrowFunction()
    {
        if (generatorThrowFunction == nullptr)
        {
            generatorThrowFunction = DefaultCreateFunction(&JavascriptGenerator::EntryInfo::Throw, 1, nullptr, nullptr, PropertyIds::throw_);
        }
        return generatorThrowFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureAsyncGeneratorNextFunction()
    {
        if (asyncGeneratorNextFunction == nullptr)
        {
            asyncGeneratorNextFunction = DefaultCreateFunction(&JavascriptAsyncGenerator::EntryInfo::Next, 1, nullptr, nullptr, PropertyIds::next);
        }
        return asyncGeneratorNextFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureAsyncGeneratorReturnFunction()
    {
        if (asyncGeneratorReturnFunction == nullptr)
        {
            asyncGeneratorReturnFunction = DefaultCreateFunction(&JavascriptAsyncGenerator::EntryInfo::Return, 1, nullptr, nullptr, PropertyIds::return_);
        }
        return asyncGeneratorReturnFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureAsyncGeneratorThrowFunction()
    {
        if (asyncGeneratorThrowFunction == nullptr)
        {
            asyncGeneratorThrowFunction = DefaultCreateFunction(&JavascriptAsyncGenerator::EntryInfo::Throw, 1, nullptr, nullptr, PropertyIds::throw_);
        }
        return asyncGeneratorThrowFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureAsyncFromSyncIteratorNextFunction()
    {
        if (asyncFromSyncIteratorNextFunction == nullptr)
        {
            asyncFromSyncIteratorNextFunction = DefaultCreateFunction(&JavascriptAsyncFromSyncIterator::EntryInfo::Next, 1, nullptr, nullptr, PropertyIds::next);
        }
        return asyncFromSyncIteratorNextFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureAsyncFromSyncIteratorReturnFunction()
    {
        if (asyncFromSyncIteratorReturnFunction == nullptr)
        {
            asyncFromSyncIteratorReturnFunction = DefaultCreateFunction(&JavascriptAsyncFromSyncIterator::EntryInfo::Return, 1, nullptr, nullptr, PropertyIds::return_);
        }
        return asyncFromSyncIteratorReturnFunction;
    }

    JavascriptFunction* JavascriptLibrary::EnsureAsyncFromSyncIteratorThrowFunction()
    {
        if (asyncFromSyncIteratorThrowFunction == nullptr)
        {
            asyncFromSyncIteratorThrowFunction = DefaultCreateFunction(&JavascriptAsyncFromSyncIterator::EntryInfo::Throw, 1, nullptr, nullptr, PropertyIds::throw_);
        }
        return asyncFromSyncIteratorThrowFunction;
    }

    bool JavascriptLibrary::InitializeAsyncFunctionConstructor(DynamicObject* asyncFunctionConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(asyncFunctionConstructor, mode, 3);
        JavascriptLibrary* library = asyncFunctionConstructor->GetLibrary();

        library->AddMember(asyncFunctionConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(asyncFunctionConstructor, PropertyIds::prototype, library->asyncFunctionPrototype, PropertyNone);
        library->AddMember(asyncFunctionConstructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("AsyncFunction")), PropertyConfigurable);

        asyncFunctionConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncFunctionPrototype(DynamicObject* asyncFunctionPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(asyncFunctionPrototype, mode, 2);
        JavascriptLibrary* library = asyncFunctionPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();

        library->AddMember(asyncFunctionPrototype, PropertyIds::constructor, library->asyncFunctionConstructor, PropertyConfigurable);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(asyncFunctionPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("AsyncFunction")), PropertyConfigurable);
        }
        asyncFunctionPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncGeneratorFunctionConstructor(DynamicObject* asyncGeneratorFunctionConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(asyncGeneratorFunctionConstructor, mode, 3);
        JavascriptLibrary* library = asyncGeneratorFunctionConstructor->GetLibrary();
        Assert(library->GetScriptContext()->GetConfig()->IsES2018AsyncIterationEnabled());

        library->AddMember(asyncGeneratorFunctionConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(asyncGeneratorFunctionConstructor, PropertyIds::prototype, library->asyncGeneratorFunctionPrototype, PropertyNone);
        library->AddMember(asyncGeneratorFunctionConstructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("AsyncGeneratorFunction")), PropertyConfigurable);

        asyncGeneratorFunctionConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncGeneratorFunctionPrototype(DynamicObject* asyncGeneratorFunctionPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(asyncGeneratorFunctionPrototype, mode, 3);
        JavascriptLibrary* library = asyncGeneratorFunctionPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();
        Assert(library->GetScriptContext()->GetConfig()->IsES2018AsyncIterationEnabled());

        library->AddMember(asyncGeneratorFunctionPrototype, PropertyIds::constructor, library->asyncGeneratorFunctionConstructor, PropertyConfigurable);
        library->AddMember(asyncGeneratorFunctionPrototype, PropertyIds::prototype, library->asyncGeneratorPrototype, PropertyConfigurable);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(asyncGeneratorFunctionPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("AsyncGeneratorFunction")), PropertyConfigurable);
        }
        asyncGeneratorFunctionPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncGeneratorPrototype(DynamicObject* asyncGeneratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptLibrary* library = asyncGeneratorPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();
        Assert(library->GetScriptContext()->GetConfig()->IsES2018AsyncIterationEnabled());
        typeHandler->Convert(asyncGeneratorPrototype, mode, 5);

        library->AddMember(asyncGeneratorPrototype, PropertyIds::constructor, library->asyncGeneratorFunctionPrototype, PropertyConfigurable);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(asyncGeneratorPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("AsyncGenerator")), PropertyConfigurable);
        }

        library->AddMember(asyncGeneratorPrototype, PropertyIds::return_, library->EnsureAsyncGeneratorReturnFunction(), PropertyBuiltInMethodDefaults);
        library->AddMember(asyncGeneratorPrototype, PropertyIds::next, library->EnsureAsyncGeneratorNextFunction(), PropertyBuiltInMethodDefaults);
        library->AddMember(asyncGeneratorPrototype, PropertyIds::throw_, library->EnsureAsyncGeneratorThrowFunction(), PropertyBuiltInMethodDefaults);

        asyncGeneratorPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncFromSyncIteratorPrototype(DynamicObject* asyncFromSyncIteratorProtototype, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode)
    {
        JavascriptLibrary* library = asyncFromSyncIteratorProtototype->GetLibrary();
        Assert(library->GetScriptContext()->GetConfig()->IsES2018AsyncIterationEnabled());
        typeHandler->Convert(asyncFromSyncIteratorProtototype, mode, 3);
        // note per spec this also has a toStringTag but it is not observable at runtime so omitted

        library->AddMember(asyncFromSyncIteratorProtototype, PropertyIds::return_, library->EnsureAsyncFromSyncIteratorReturnFunction(), PropertyBuiltInMethodDefaults);
        library->AddMember(asyncFromSyncIteratorProtototype, PropertyIds::next, library->EnsureAsyncFromSyncIteratorNextFunction(), PropertyBuiltInMethodDefaults);
        library->AddMember(asyncFromSyncIteratorProtototype, PropertyIds::throw_, library->EnsureAsyncFromSyncIteratorThrowFunction(), PropertyBuiltInMethodDefaults);

        asyncFromSyncIteratorProtototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeProxyConstructor(DynamicObject* proxyConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(proxyConstructor, mode, 4);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSymbol
        // so that the update is in sync with profiler
        JavascriptLibrary* library = proxyConstructor->GetLibrary();
        ScriptContext* scriptContext = proxyConstructor->GetScriptContext();
        library->AddMember(proxyConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(2), PropertyConfigurable);
        library->AddMember(proxyConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Proxy), PropertyConfigurable);

        library->AddFunctionToLibraryObject(proxyConstructor, PropertyIds::revocable, &JavascriptProxy::EntryInfo::Revocable, 2, PropertyConfigurable);

        proxyConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeProxyPrototype(DynamicObject* proxyPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(proxyPrototype, mode, 1);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSymbol
        // so that the update is in sync with profiler
        JavascriptLibrary* library = proxyPrototype->GetLibrary();
        library->AddMember(proxyPrototype, PropertyIds::constructor, library->proxyConstructor);

        proxyPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeDateConstructor(DynamicObject* dateConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(dateConstructor, mode, 6);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterDate
        // so that the update is in sync with profiler
        JavascriptLibrary* library = dateConstructor->GetLibrary();
        ScriptContext* scriptContext = dateConstructor->GetScriptContext();
        library->AddMember(dateConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(7), PropertyConfigurable);
        library->AddMember(dateConstructor, PropertyIds::prototype, library->datePrototype, PropertyNone);
        library->AddMember(dateConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Date), PropertyConfigurable);

        library->AddFunctionToLibraryObject(dateConstructor, PropertyIds::parse, &JavascriptDate::EntryInfo::Parse, 1); // should be static
        library->AddFunctionToLibraryObject(dateConstructor, PropertyIds::now, &JavascriptDate::EntryInfo::Now, 0);     // should be static
        library->AddFunctionToLibraryObject(dateConstructor, PropertyIds::UTC, &JavascriptDate::EntryInfo::UTC, 7);     // should be static

        dateConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeDatePrototype(DynamicObject* datePrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(datePrototype, mode, 48);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterDate
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = datePrototype->GetScriptContext();
        JavascriptLibrary* library = datePrototype->GetLibrary();
        library->AddMember(datePrototype, PropertyIds::constructor, library->dateConstructor);
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetDate.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getDate, &JavascriptDate::EntryInfo::GetDate, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetDay.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getDay, &JavascriptDate::EntryInfo::GetDay, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetFullYear.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getFullYear, &JavascriptDate::EntryInfo::GetFullYear, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetHours.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getHours, &JavascriptDate::EntryInfo::GetHours, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetMilliseconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getMilliseconds, &JavascriptDate::EntryInfo::GetMilliseconds, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetMinutes.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getMinutes, &JavascriptDate::EntryInfo::GetMinutes, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetMonth.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getMonth, &JavascriptDate::EntryInfo::GetMonth, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetSeconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getSeconds, &JavascriptDate::EntryInfo::GetSeconds, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetTime.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getTime, &JavascriptDate::EntryInfo::GetTime, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetTimezoneOffset.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getTimezoneOffset, &JavascriptDate::EntryInfo::GetTimezoneOffset, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCDate.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCDate, &JavascriptDate::EntryInfo::GetUTCDate, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCDay.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCDay, &JavascriptDate::EntryInfo::GetUTCDay, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCFullYear.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCFullYear, &JavascriptDate::EntryInfo::GetUTCFullYear, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCHours.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCHours, &JavascriptDate::EntryInfo::GetUTCHours, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCMilliseconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCMilliseconds, &JavascriptDate::EntryInfo::GetUTCMilliseconds, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCMinutes.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCMinutes, &JavascriptDate::EntryInfo::GetUTCMinutes, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCMonth.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCMonth, &JavascriptDate::EntryInfo::GetUTCMonth, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetUTCSeconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getUTCSeconds, &JavascriptDate::EntryInfo::GetUTCSeconds, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::GetYear.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::getYear, &JavascriptDate::EntryInfo::GetYear, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetDate.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setDate, &JavascriptDate::EntryInfo::SetDate, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetFullYear.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setFullYear, &JavascriptDate::EntryInfo::SetFullYear, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetHours.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setHours, &JavascriptDate::EntryInfo::SetHours, 4));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetMilliseconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setMilliseconds, &JavascriptDate::EntryInfo::SetMilliseconds, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetMinutes.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setMinutes, &JavascriptDate::EntryInfo::SetMinutes, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetMonth.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setMonth, &JavascriptDate::EntryInfo::SetMonth, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetSeconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setSeconds, &JavascriptDate::EntryInfo::SetSeconds, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetTime.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setTime, &JavascriptDate::EntryInfo::SetTime, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCDate.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCDate, &JavascriptDate::EntryInfo::SetUTCDate, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCFullYear.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCFullYear, &JavascriptDate::EntryInfo::SetUTCFullYear, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCHours.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCHours, &JavascriptDate::EntryInfo::SetUTCHours, 4));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCMilliseconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCMilliseconds, &JavascriptDate::EntryInfo::SetUTCMilliseconds, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCMinutes.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCMinutes, &JavascriptDate::EntryInfo::SetUTCMinutes, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCMonth.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCMonth, &JavascriptDate::EntryInfo::SetUTCMonth, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetUTCSeconds.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setUTCSeconds, &JavascriptDate::EntryInfo::SetUTCSeconds, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SetYear.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::setYear, &JavascriptDate::EntryInfo::SetYear, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToDateString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toDateString, &JavascriptDate::EntryInfo::ToDateString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToISOString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toISOString, &JavascriptDate::EntryInfo::ToISOString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToJSON.GetOriginalEntryPoint(),
             library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toJSON, &JavascriptDate::EntryInfo::ToJSON, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToLocaleDateString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toLocaleDateString, &JavascriptDate::EntryInfo::ToLocaleDateString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToLocaleString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toLocaleString, &JavascriptDate::EntryInfo::ToLocaleString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToLocaleTimeString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toLocaleTimeString, &JavascriptDate::EntryInfo::ToLocaleTimeString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toString, &JavascriptDate::EntryInfo::ToString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToTimeString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toTimeString, &JavascriptDate::EntryInfo::ToTimeString, 0));

        // Spec stipulates toGMTString must be the same function object as toUTCString
        JavascriptFunction *toUTCStringFunc = library->AddFunctionToLibraryObject(datePrototype, PropertyIds::toUTCString, &JavascriptDate::EntryInfo::ToUTCString, 0);
        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ToUTCString.GetOriginalEntryPoint(), toUTCStringFunc);
        library->AddMember(datePrototype, PropertyIds::toGMTString, toUTCStringFunc, PropertyBuiltInMethodDefaults);

        scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::ValueOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(datePrototype, PropertyIds::valueOf, &JavascriptDate::EntryInfo::ValueOf, 0));

        if (scriptContext->GetConfig()->IsES6ToPrimitiveEnabled())
        {
            scriptContext->SetBuiltInLibraryFunction(JavascriptDate::EntryInfo::SymbolToPrimitive.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObjectWithName(datePrototype, PropertyIds::_symbolToPrimitive, PropertyIds::_RuntimeFunctionNameId_toPrimitive,
                &JavascriptDate::EntryInfo::SymbolToPrimitive, 1));
            datePrototype->SetWritable(PropertyIds::_symbolToPrimitive, false);
        }
        datePrototype->SetHasNoEnumerableProperties(true);

        return true;
    }


    bool JavascriptLibrary::InitializeFunctionConstructor(DynamicObject* functionConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(functionConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterFunction
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = functionConstructor->GetScriptContext();
        JavascriptLibrary* library = functionConstructor->GetLibrary();
        library->AddMember(functionConstructor, PropertyIds::prototype, library->functionPrototype, PropertyNone);
        library->AddMember(functionConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(functionConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Function), PropertyConfigurable);

        functionConstructor->SetHasNoEnumerableProperties(true);

#ifdef ALLOW_JIT_REPRO
        if (CONFIG_FLAG(JitRepro))
        {
            library->AddFunctionToLibraryObject(functionConstructor, PropertyIds::invokeJit, &JavascriptFunction::EntryInfo::InvokeJit, 1);
        }
#endif

        return true;
    }

    bool JavascriptLibrary::InitializeFunctionPrototype(DynamicObject* functionPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(functionPrototype, mode, 7);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterFunction
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = functionPrototype->GetScriptContext();
        JavascriptLibrary* library = functionPrototype->GetLibrary();
        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

        library->AddMember(functionPrototype, PropertyIds::constructor, library->functionConstructor);
        library->AddMember(functionPrototype, PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable);
        library->AddMember(functionPrototype, PropertyIds::name, library->GetEmptyString(), PropertyConfigurable);

        JavascriptFunction *func = library->AddFunctionToLibraryObject(functionPrototype, PropertyIds::apply, &JavascriptFunction::EntryInfo::Apply, 2);
        builtinFuncs[BuiltinFunction::JavascriptFunction_Apply] = func;

        library->AddFunctionToLibraryObject(functionPrototype, PropertyIds::bind, &JavascriptFunction::EntryInfo::Bind, 1);
        func = library->AddFunctionToLibraryObject(functionPrototype, PropertyIds::call, &JavascriptFunction::EntryInfo::Call, 1);
        builtinFuncs[BuiltinFunction::JavascriptFunction_Call] = func;
        library->AddFunctionToLibraryObject(functionPrototype, PropertyIds::toString, &JavascriptFunction::EntryInfo::ToString, 0);

        if (scriptContext->GetConfig()->IsES6HasInstanceEnabled())
        {
            scriptContext->SetBuiltInLibraryFunction(JavascriptFunction::EntryInfo::SymbolHasInstance.GetOriginalEntryPoint(),
                                                     library->AddFunctionToLibraryObjectWithName(functionPrototype, PropertyIds::_symbolHasInstance, PropertyIds::_RuntimeFunctionNameId_hasInstance,
                                                                                                 &JavascriptFunction::EntryInfo::SymbolHasInstance, 1));
            functionPrototype->SetWritable(PropertyIds::_symbolHasInstance, false);
            functionPrototype->SetConfigurable(PropertyIds::_symbolHasInstance, false);
        }

        functionPrototype->DynamicObject::SetAccessors(PropertyIds::caller, library->throwTypeErrorRestrictedPropertyAccessorFunction, library->throwTypeErrorRestrictedPropertyAccessorFunction);
        functionPrototype->SetEnumerable(PropertyIds::caller, false);
        functionPrototype->DynamicObject::SetAccessors(PropertyIds::arguments, library->throwTypeErrorRestrictedPropertyAccessorFunction, library->throwTypeErrorRestrictedPropertyAccessorFunction);
        functionPrototype->SetEnumerable(PropertyIds::arguments, false);

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        functionPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    void JavascriptLibrary::InitializeComplexThings()
    {
        emptyRegexPattern = RegexHelper::CompileDynamic(scriptContext, _u(""), 0, _u(""), 0, false);

        Recycler *const recycler = GetRecycler();

        const ScriptConfiguration *scriptConfig = scriptContext->GetConfig();

        // Creating the regex prototype object requires compiling an empty regex, which may require error types to be
        // initialized first (such as when a stack probe fails). So, the regex prototype and other things that depend on it are
        // initialized here, which will be after the dependency types are initialized.
        //
        // In ES6, RegExp.prototype is not a RegExp object itself so we do not need to wait and create an empty RegExp.
        // Instead, we just create an ordinary object prototype for RegExp.prototype in InitializePrototypes.
        if (!scriptConfig->IsES6PrototypeChain() && regexPrototype == nullptr)
        {
            regexPrototype = RecyclerNew(recycler, JavascriptRegExp, emptyRegexPattern,
                DynamicType::New(scriptContext, TypeIds_RegEx, objectPrototype, nullptr,
                DeferredTypeHandler<InitializeRegexPrototype, DefaultDeferredTypeFilter, true>::GetDefaultInstance()));
        }

        PathTypeHandlerNoAttr *typeHandler =
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true);
        // See JavascriptRegExp::IsWritable for property writability
        if (!scriptConfig->IsES6RegExPrototypePropertiesEnabled())
        {
            typeHandler->ClearHasOnlyWritableDataProperties();
        }

        regexType = DynamicType::New(scriptContext, TypeIds_RegEx, regexPrototype, nullptr, typeHandler, true, true);
    }

    bool JavascriptLibrary::InitializeMathObject(DynamicObject* mathObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(mathObject, mode, 42);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterMath
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = mathObject->GetScriptContext();
        JavascriptLibrary* library = mathObject->GetLibrary();

        library->AddMember(mathObject, PropertyIds::E,       JavascriptNumber::New(Math::E,       scriptContext), PropertyNone);
        library->AddMember(mathObject, PropertyIds::LN10,    JavascriptNumber::New(Math::LN10,    scriptContext), PropertyNone);
        library->AddMember(mathObject, PropertyIds::LN2,     JavascriptNumber::New(Math::LN2,     scriptContext), PropertyNone);
        library->AddMember(mathObject, PropertyIds::LOG2E,   JavascriptNumber::New(Math::LOG2E,   scriptContext), PropertyNone);
        library->AddMember(mathObject, PropertyIds::LOG10E,  JavascriptNumber::New(Math::LOG10E,  scriptContext), PropertyNone);
        library->AddMember(mathObject, PropertyIds::PI,      library->pi,                                         PropertyNone);
        library->AddMember(mathObject, PropertyIds::SQRT1_2, JavascriptNumber::New(Math::SQRT1_2, scriptContext), PropertyNone);
        library->AddMember(mathObject, PropertyIds::SQRT2,   JavascriptNumber::New(Math::SQRT2,   scriptContext), PropertyNone);

        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            EnsureBuiltInEngineIsReady(JsBuiltInFile::Math_object, scriptContext);
        }
        else
#endif
        {
            builtinFuncs[BuiltinFunction::Math_Max] = library->AddFunctionToLibraryObject(mathObject, PropertyIds::max, &Math::EntryInfo::Max, 2);
            builtinFuncs[BuiltinFunction::Math_Min] = library->AddFunctionToLibraryObject(mathObject, PropertyIds::min, &Math::EntryInfo::Min, 2);
        }

        builtinFuncs[BuiltinFunction::Math_Abs]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::abs,    &Math::EntryInfo::Abs,    1);
        builtinFuncs[BuiltinFunction::Math_Acos]   = library->AddFunctionToLibraryObject(mathObject, PropertyIds::acos,   &Math::EntryInfo::Acos,   1);
        builtinFuncs[BuiltinFunction::Math_Asin]   = library->AddFunctionToLibraryObject(mathObject, PropertyIds::asin,   &Math::EntryInfo::Asin,   1);
        builtinFuncs[BuiltinFunction::Math_Atan]   = library->AddFunctionToLibraryObject(mathObject, PropertyIds::atan,   &Math::EntryInfo::Atan,   1);
        builtinFuncs[BuiltinFunction::Math_Atan2]  = library->AddFunctionToLibraryObject(mathObject, PropertyIds::atan2,  &Math::EntryInfo::Atan2,  2);
        builtinFuncs[BuiltinFunction::Math_Cos]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::cos,    &Math::EntryInfo::Cos,    1);
        builtinFuncs[BuiltinFunction::Math_Ceil]   = library->AddFunctionToLibraryObject(mathObject, PropertyIds::ceil,   &Math::EntryInfo::Ceil,   1);
        builtinFuncs[BuiltinFunction::Math_Exp]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::exp,    &Math::EntryInfo::Exp,    1);
        builtinFuncs[BuiltinFunction::Math_Floor]  = library->AddFunctionToLibraryObject(mathObject, PropertyIds::floor,  &Math::EntryInfo::Floor,  1);
        builtinFuncs[BuiltinFunction::Math_Log]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::log,    &Math::EntryInfo::Log,    1);
        builtinFuncs[BuiltinFunction::Math_Pow]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::pow,    &Math::EntryInfo::Pow,    2);
        builtinFuncs[BuiltinFunction::Math_Random] = library->AddFunctionToLibraryObject(mathObject, PropertyIds::random, &Math::EntryInfo::Random, 0);
        builtinFuncs[BuiltinFunction::Math_Round]  = library->AddFunctionToLibraryObject(mathObject, PropertyIds::round,  &Math::EntryInfo::Round,  1);
        builtinFuncs[BuiltinFunction::Math_Sin]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::sin,    &Math::EntryInfo::Sin,    1);
        builtinFuncs[BuiltinFunction::Math_Sqrt]   = library->AddFunctionToLibraryObject(mathObject, PropertyIds::sqrt,   &Math::EntryInfo::Sqrt,   1);
        builtinFuncs[BuiltinFunction::Math_Tan]    = library->AddFunctionToLibraryObject(mathObject, PropertyIds::tan,    &Math::EntryInfo::Tan,    1);

        if (scriptContext->GetConfig()->IsES6MathExtensionsEnabled())
        {
            builtinFuncs[BuiltinFunction::Math_Imul] = library->AddFunctionToLibraryObject(mathObject, PropertyIds::imul, &Math::EntryInfo::Imul, 2);
            builtinFuncs[BuiltinFunction::Math_Fround] = library->AddFunctionToLibraryObject(mathObject, PropertyIds::fround, &Math::EntryInfo::Fround, 1);
            /*builtinFuncs[BuiltinFunction::Math_Log10] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::log10, &Math::EntryInfo::Log10, 1);
            /*builtinFuncs[BuiltinFunction::Math_Log2]  =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::log2,  &Math::EntryInfo::Log2,  1);
            /*builtinFuncs[BuiltinFunction::Math_Log1p] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::log1p, &Math::EntryInfo::Log1p, 1);
            /*builtinFuncs[BuiltinFunction::Math_Expm1] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::expm1, &Math::EntryInfo::Expm1, 1);
            /*builtinFuncs[BuiltinFunction::Math_Cosh]  =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::cosh,  &Math::EntryInfo::Cosh,  1);
            /*builtinFuncs[BuiltinFunction::Math_Sinh]  =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::sinh,  &Math::EntryInfo::Sinh,  1);
            /*builtinFuncs[BuiltinFunction::Math_Tanh]  =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::tanh,  &Math::EntryInfo::Tanh,  1);
            /*builtinFuncs[BuiltinFunction::Math_Acosh] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::acosh, &Math::EntryInfo::Acosh, 1);
            /*builtinFuncs[BuiltinFunction::Math_Asinh] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::asinh, &Math::EntryInfo::Asinh, 1);
            /*builtinFuncs[BuiltinFunction::Math_Atanh] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::atanh, &Math::EntryInfo::Atanh, 1);
            /*builtinFuncs[BuiltinFunction::Math_Hypot] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::hypot, &Math::EntryInfo::Hypot, 2);
            /*builtinFuncs[BuiltinFunction::Math_Trunc] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::trunc, &Math::EntryInfo::Trunc, 1);
            /*builtinFuncs[BuiltinFunction::Math_Sign]  =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::sign,  &Math::EntryInfo::Sign,  1);
            /*builtinFuncs[BuiltinFunction::Math_Cbrt]  =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::cbrt,  &Math::EntryInfo::Cbrt,  1);
            /*builtinFuncs[BuiltinFunction::Math_Clz32] =*/ library->AddFunctionToLibraryObject(mathObject, PropertyIds::clz32, &Math::EntryInfo::Clz32, 1);
        }

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(mathObject, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Math")), PropertyConfigurable);
        }

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        mathObject->SetHasNoEnumerableProperties(true);

        return true;
    }

#ifdef ENABLE_WASM

    bool JavascriptLibrary::InitializeWebAssemblyTablePrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(prototype, mode, 6);

        JavascriptLibrary* library = prototype->GetLibrary();
        ScriptContext* scriptContext = prototype->GetScriptContext();

        library->AddMember(prototype, PropertyIds::constructor, library->webAssemblyTableConstructor);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(prototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WebAssembly.Table")), PropertyConfigurable);
        }
        scriptContext->SetBuiltInLibraryFunction(WebAssemblyTable::EntryInfo::Grow.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(prototype, PropertyIds::grow, &WebAssemblyTable::EntryInfo::Grow, 1));

        scriptContext->SetBuiltInLibraryFunction(WebAssemblyTable::EntryInfo::Get.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(prototype, PropertyIds::get, &WebAssemblyTable::EntryInfo::Get, 1));

        scriptContext->SetBuiltInLibraryFunction(WebAssemblyTable::EntryInfo::Set.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(prototype, PropertyIds::set, &WebAssemblyTable::EntryInfo::Set, 2));

        library->AddAccessorsToLibraryObject(prototype, PropertyIds::length, &WebAssemblyTable::EntryInfo::GetterLength, nullptr);

        prototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyTableConstructor(DynamicObject* constructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(constructor, mode, 3);
        JavascriptLibrary* library = constructor->GetLibrary();

        library->AddMember(constructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(constructor, PropertyIds::prototype, library->webAssemblyTablePrototype, PropertyNone);
        library->AddMember(constructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("Table")), PropertyConfigurable);

        constructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyMemoryPrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(prototype, mode, 4);

        JavascriptLibrary* library = prototype->GetLibrary();
        ScriptContext* scriptContext = prototype->GetScriptContext();

        library->AddMember(prototype, PropertyIds::constructor, library->webAssemblyMemoryConstructor);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(prototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WebAssembly.Memory")), PropertyConfigurable);
        }
        scriptContext->SetBuiltInLibraryFunction(WebAssemblyMemory::EntryInfo::Grow.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(prototype, PropertyIds::grow, &WebAssemblyMemory::EntryInfo::Grow, 1));

        library->AddAccessorsToLibraryObject(prototype, PropertyIds::buffer, &WebAssemblyMemory::EntryInfo::GetterBuffer, nullptr);

        prototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyMemoryConstructor(DynamicObject* constructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(constructor, mode, 3);
        JavascriptLibrary* library = constructor->GetLibrary();

        library->AddMember(constructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(constructor, PropertyIds::prototype, library->webAssemblyMemoryPrototype, PropertyNone);
        library->AddMember(constructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("Memory")), PropertyConfigurable);

        constructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyInstancePrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(prototype, mode, 3);

        JavascriptLibrary* library = prototype->GetLibrary();
        ScriptContext* scriptContext = prototype->GetScriptContext();

        library->AddMember(prototype, PropertyIds::constructor, library->webAssemblyInstanceConstructor);
        library->AddAccessorsToLibraryObject(prototype, PropertyIds::exports, &WebAssemblyInstance::EntryInfo::GetterExports, nullptr);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(prototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WebAssembly.Instance")), PropertyConfigurable);
        }
        prototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyInstanceConstructor(DynamicObject* constructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(constructor, mode, 3);
        JavascriptLibrary* library = constructor->GetLibrary();

        library->AddMember(constructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(constructor, PropertyIds::prototype, library->webAssemblyInstancePrototype, PropertyNone);
        library->AddMember(constructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("Instance")), PropertyConfigurable);

        constructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyModulePrototype(DynamicObject* prototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(prototype, mode, 2);

        JavascriptLibrary* library = prototype->GetLibrary();
        ScriptContext* scriptContext = prototype->GetScriptContext();

        library->AddMember(prototype, PropertyIds::constructor, library->webAssemblyModuleConstructor);
        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(prototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WebAssembly.Module")), PropertyConfigurable);
        }
        prototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyModuleConstructor(DynamicObject* constructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(constructor, mode, 5);
        JavascriptLibrary* library = constructor->GetLibrary();

        library->AddMember(constructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(constructor, PropertyIds::prototype, library->webAssemblyModulePrototype, PropertyNone);
        library->AddMember(constructor, PropertyIds::name, library->CreateStringFromCppLiteral(_u("Module")), PropertyConfigurable);

        library->AddFunctionToLibraryObject(constructor, PropertyIds::exports, &WebAssemblyModule::EntryInfo::Exports, 1);
        library->AddFunctionToLibraryObject(constructor, PropertyIds::imports, &WebAssemblyModule::EntryInfo::Imports, 1);
        library->AddFunctionToLibraryObject(constructor, PropertyIds::customSections, &WebAssemblyModule::EntryInfo::CustomSections, 2);

        constructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWebAssemblyObject(DynamicObject* webAssemblyObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptLibrary* library = webAssemblyObject->GetLibrary();
        int slots = 9;
#ifdef ENABLE_WABT
        // Attaching wabt for testing
        ++slots;
#endif
        typeHandler->Convert(webAssemblyObject, mode, slots);

#ifdef ENABLE_WABT
        // Build wabt object
        Js::DynamicObject* wabtObject = library->CreateObject(true);
        library->AddFunctionToLibraryObject(wabtObject, PropertyIds::convertWast2Wasm, &WabtInterface::EntryInfo::ConvertWast2Wasm, 1);
        library->AddMember(webAssemblyObject, PropertyIds::wabt, wabtObject, PropertyNone);
#endif
        library->webAssemblyCompileFunction =
            library->AddFunctionToLibraryObject(webAssemblyObject, PropertyIds::compile, &WebAssembly::EntryInfo::Compile, 1);
        library->AddFunctionToLibraryObject(webAssemblyObject, PropertyIds::compileStreaming, &WebAssembly::EntryInfo::CompileStreaming, 1);
        library->AddFunctionToLibraryObject(webAssemblyObject, PropertyIds::validate, &WebAssembly::EntryInfo::Validate, 1);
        library->AddFunctionToLibraryObject(webAssemblyObject, PropertyIds::instantiate, &WebAssembly::EntryInfo::Instantiate, 1);
        library->AddFunctionToLibraryObject(webAssemblyObject, PropertyIds::instantiateStreaming, &WebAssembly::EntryInfo::InstantiateStreaming, 1);
        library->webAssemblyQueryResponseFunction = library->DefaultCreateFunction(&WebAssembly::EntryInfo::QueryResponse, 1, nullptr, nullptr, PropertyIds::undefined);
        library->webAssemblyInstantiateBoundFunction = library->DefaultCreateFunction(&WebAssembly::EntryInfo::InstantiateBound, 1, nullptr, nullptr, PropertyIds::undefined);

        library->AddFunction(webAssemblyObject, PropertyIds::Module, library->webAssemblyModuleConstructor);

        library->AddFunction(webAssemblyObject, PropertyIds::Instance, library->webAssemblyInstanceConstructor);

        library->AddFunction(webAssemblyObject, PropertyIds::CompileError, library->webAssemblyCompileErrorConstructor);
        library->AddFunction(webAssemblyObject, PropertyIds::RuntimeError, library->webAssemblyRuntimeErrorConstructor);
        library->AddFunction(webAssemblyObject, PropertyIds::LinkError, library->webAssemblyLinkErrorConstructor);
        library->AddFunction(webAssemblyObject, PropertyIds::Memory, library->webAssemblyMemoryConstructor);
        library->AddFunction(webAssemblyObject, PropertyIds::Table, library->webAssemblyTableConstructor);

        library->AddMember(webAssemblyObject, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WebAssembly")), PropertyConfigurable);

        return true;
    }
#endif

    bool JavascriptLibrary::InitializeReflectObject(DynamicObject* reflectObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(reflectObject, mode, 12);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterReflect
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = reflectObject->GetScriptContext();
        JavascriptLibrary* library = reflectObject->GetLibrary();
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::DefineProperty.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::defineProperty, &JavascriptReflect::EntryInfo::DefineProperty, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::DeleteProperty.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::deleteProperty, &JavascriptReflect::EntryInfo::DeleteProperty, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::Get.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::get, &JavascriptReflect::EntryInfo::Get, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::GetOwnPropertyDescriptor.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::getOwnPropertyDescriptor, &JavascriptReflect::EntryInfo::GetOwnPropertyDescriptor, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::GetPrototypeOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::getPrototypeOf, &JavascriptReflect::EntryInfo::GetPrototypeOf, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::Has.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::has, &JavascriptReflect::EntryInfo::Has, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::IsExtensible.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::isExtensible, &JavascriptReflect::EntryInfo::IsExtensible, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::OwnKeys.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::ownKeys, &JavascriptReflect::EntryInfo::OwnKeys, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::PreventExtensions.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::preventExtensions, &JavascriptReflect::EntryInfo::PreventExtensions, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::Set.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::set, &JavascriptReflect::EntryInfo::Set, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::SetPrototypeOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::setPrototypeOf, &JavascriptReflect::EntryInfo::SetPrototypeOf, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::Apply.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::apply, &JavascriptReflect::EntryInfo::Apply, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptReflect::EntryInfo::Construct.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(reflectObject, PropertyIds::construct, &JavascriptReflect::EntryInfo::Construct, 2));

        return true;
    }

    void JavascriptLibrary::InitializeStaticValues()
    {
        constructorCacheDefaultInstance = &Js::ConstructorCache::DefaultInstance;
        absDoubleCst = Js::JavascriptNumber::AbsDoubleCst;
        uintConvertConst = Js::JavascriptNumber::UIntConvertConst;

        defaultPropertyDescriptor.SetValue(undefinedValue);
        defaultPropertyDescriptor.SetWritable(false);
        defaultPropertyDescriptor.SetGetter(defaultAccessorFunction);
        defaultPropertyDescriptor.SetSetter(defaultAccessorFunction);
        defaultPropertyDescriptor.SetEnumerable(false);
        defaultPropertyDescriptor.SetConfigurable(false);

#if !defined(TARGET_64)

        VirtualTableRecorder<Js::JavascriptNumber>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptNumber);
#else
        vtableAddresses[VTableValue::VtableJavascriptNumber] = 0;
#endif
        VirtualTableRecorder<Js::DynamicObject>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableDynamicObject);
        vtableAddresses[VTableValue::VtableInvalid] = Js::ScriptContextOptimizationOverrideInfo::InvalidVtable;
        VirtualTableRecorder<Js::PropertyString>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtablePropertyString);
        VirtualTableRecorder<Js::LazyJSONString>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableLazyJSONString);
        VirtualTableRecorder<Js::JavascriptBoolean>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptBoolean);
        VirtualTableRecorder<Js::JavascriptArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptArray);
        VirtualTableRecorder<Js::Int8Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt8Array);
        VirtualTableRecorder<Js::Uint8Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint8Array);
        VirtualTableRecorder<Js::Uint8ClampedArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint8ClampedArray);
        VirtualTableRecorder<Js::Int16Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt16Array);
        VirtualTableRecorder<Js::Uint16Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint16Array);
        VirtualTableRecorder<Js::Int32Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt32Array);
        VirtualTableRecorder<Js::Uint32Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint32Array);
        VirtualTableRecorder<Js::Float32Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableFloat32Array);
        VirtualTableRecorder<Js::Float64Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableFloat64Array);
        VirtualTableRecorder<Js::Int64Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt64Array);
        VirtualTableRecorder<Js::Uint64Array>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint64Array);

        VirtualTableRecorder<Js::Int8VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt8VirtualArray);
        VirtualTableRecorder<Js::Uint8VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint8VirtualArray);
        VirtualTableRecorder<Js::Uint8ClampedVirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint8ClampedVirtualArray);
        VirtualTableRecorder<Js::Int16VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt16VirtualArray);
        VirtualTableRecorder<Js::Uint16VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint16VirtualArray);
        VirtualTableRecorder<Js::Int32VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableInt32VirtualArray);
        VirtualTableRecorder<Js::Uint32VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableUint32VirtualArray);
        VirtualTableRecorder<Js::Float32VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableFloat32VirtualArray);
        VirtualTableRecorder<Js::Float64VirtualArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableFloat64VirtualArray);

        VirtualTableRecorder<Js::BoolArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableBoolArray);
        VirtualTableRecorder<Js::CharArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableCharArray);

        VirtualTableRecorder<Js::JavascriptNativeIntArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableNativeIntArray);
#if ENABLE_COPYONACCESS_ARRAY
        VirtualTableRecorder<Js::JavascriptCopyOnAccessNativeIntArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableCopyOnAccessNativeIntArray);
#endif
        VirtualTableRecorder<Js::JavascriptNativeFloatArray>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableNativeFloatArray);
        // don't validate vtable for VtableJavascriptNativeIntArray because its vtable is used for VtableNativeIntArray
        vtableAddresses[VTableValue::VtableJavascriptNativeIntArray] = VirtualTableInfo<Js::JavascriptNativeIntArray>::Address;
        VirtualTableRecorder<Js::JavascriptRegExp>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptRegExp);
        VirtualTableRecorder<Js::StackScriptFunction>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableStackScriptFunction);
        VirtualTableRecorder<Js::ScriptFunction>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunction);
        VirtualTableRecorder<Js::JavascriptGeneratorFunction>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptGeneratorFunction);
        VirtualTableRecorder<Js::JavascriptAsyncFunction>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptAsyncFunction);
        VirtualTableRecorder<Js::JavascriptAsyncGeneratorFunction>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableJavascriptAsyncGeneratorFunction);

        // ScriptFunction
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::AsmJsScriptFunction>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableAsmJsScriptFunctionWithComputedName);
        VirtualTableRecorder<Js::FunctionWithHomeObj<Js::ScriptFunction>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunctionWithHomeObj);
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::ScriptFunction>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunctionWithComputedName);
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::FunctionWithHomeObj<Js::ScriptFunction>>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunctionWithHomeObjAndComputedName);
        VirtualTableRecorder<Js::FunctionWithHomeObj<Js::ScriptFunctionWithInlineCache>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunctionWithInlineCacheAndHomeObj);
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::ScriptFunctionWithInlineCache>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunctionWithInlineCacheAndComputedName);
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::FunctionWithHomeObj<Js::ScriptFunctionWithInlineCache>>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableScriptFunctionWithInlineCacheHomeObjAndComputedName);
        VirtualTableRecorder<Js::FunctionWithHomeObj<Js::GeneratorVirtualScriptFunction>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableVirtualJavascriptGeneratorFunctionWithHomeObj);
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::GeneratorVirtualScriptFunction>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableVirtualJavascriptGeneratorFunctionWithComputedName);
        VirtualTableRecorder<Js::FunctionWithComputedName<Js::FunctionWithHomeObj<Js::GeneratorVirtualScriptFunction>>>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableVirtualJavascriptGeneratorFunctionWithHomeObjAndComputedName);

        VirtualTableRecorder<Js::ConcatStringMulti>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableConcatStringMulti);
        VirtualTableRecorder<Js::CompoundString>::RecordVirtualTableAddress(vtableAddresses, VTableValue::VtableCompoundString);

        for (TypeId typeId = static_cast<TypeId>(0); typeId < TypeIds_Limit; typeId = static_cast<TypeId>(typeId + 1))
        {
            switch (typeId)
            {
            case TypeIds_Undefined:
                typeDisplayStrings[typeId] = GetUndefinedDisplayString();
                break;

            case TypeIds_Function:
                typeDisplayStrings[typeId] = GetFunctionTypeDisplayString();
                break;

            case TypeIds_Boolean:
                typeDisplayStrings[typeId] = GetBooleanTypeDisplayString();
                break;

            case TypeIds_String:
                typeDisplayStrings[typeId] = GetStringTypeDisplayString();
                break;

            case TypeIds_Symbol:
                typeDisplayStrings[typeId] = GetSymbolTypeDisplayString();
                break;

            case TypeIds_Integer:
            case TypeIds_Number:
            case TypeIds_Int64Number:
            case TypeIds_UInt64Number:
                typeDisplayStrings[typeId] = GetNumberTypeDisplayString();
                break;

            case TypeIds_Enumerator:
            case TypeIds_HostDispatch:
            case TypeIds_UnscopablesWrapperObject:
            case TypeIds_UndeclBlockVar:
            case TypeIds_Proxy:
            case TypeIds_SpreadArgument:
                typeDisplayStrings[typeId] = nullptr;
                break;

            default:
                typeDisplayStrings[typeId] = GetObjectTypeDisplayString();
                break;
            }
        }
    }

    // Note: This function is only used in float preferencing scenarios. Should remove it once we do away with float preferencing.

    // Cases like,
    // case PropertyIds::concat:
    // case PropertyIds::indexOf:
    // case PropertyIds::lastIndexOf:
    // case PropertyIds::slice:
    // which have same names for Array and String cannot be resolved just by the property id

    BuiltinFunction JavascriptLibrary::GetBuiltinFunctionForPropId(PropertyId id)
    {
        switch (id)
        {
        case PropertyIds::abs:
            return BuiltinFunction::Math_Abs;

        // For now, avoid mapping Math.atan2 to a direct CRT call, as the
        // fast CRT helper doesn't handle denormals correctly.
        // case PropertyIds::atan2:
        //    return BuiltinFunction::Atan2;

        case PropertyIds::acos:
            return BuiltinFunction::Math_Acos;

        case PropertyIds::asin:
            return BuiltinFunction::Math_Asin;

        case PropertyIds::atan:
            return BuiltinFunction::Math_Atan;

        case PropertyIds::cos:
            return BuiltinFunction::Math_Cos;

        case PropertyIds::exp:
            return BuiltinFunction::Math_Exp;

        case PropertyIds::log:
            return BuiltinFunction::Math_Log;

        case PropertyIds::pow:
            return BuiltinFunction::Math_Pow;

        case PropertyIds::random:
            return BuiltinFunction::Math_Random;

        case PropertyIds::sin:
            return BuiltinFunction::Math_Sin;

        case PropertyIds::sqrt:
            return BuiltinFunction::Math_Sqrt;

        case PropertyIds::tan:
            return BuiltinFunction::Math_Tan;

        case PropertyIds::floor:
            return BuiltinFunction::Math_Floor;

        case PropertyIds::ceil:
            return BuiltinFunction::Math_Ceil;

        case PropertyIds::round:
            return BuiltinFunction::Math_Round;

         case PropertyIds::max:
            return BuiltinFunction::Math_Max;

        case PropertyIds::min:
            return BuiltinFunction::Math_Min;

        case PropertyIds::imul:
            return BuiltinFunction::Math_Imul;

        case PropertyIds::fround:
            return BuiltinFunction::Math_Fround;

        case PropertyIds::codePointAt:
            return BuiltinFunction::JavascriptString_CodePointAt;

        case PropertyIds::at:
            return BuiltinFunction::JavascriptArray_At;

        case PropertyIds::push:
            return BuiltinFunction::JavascriptArray_Push;

        case PropertyIds::concat:
            return BuiltinFunction::JavascriptArray_Concat;

        case PropertyIds::indexOf:
            return BuiltinFunction::JavascriptArray_IndexOf;

        case PropertyIds::includes:
            return BuiltinFunction::JavascriptArray_Includes;

        case PropertyIds::isArray:
            return BuiltinFunction::JavascriptArray_IsArray;

        case PropertyIds::join:
            return BuiltinFunction::JavascriptArray_Join;

        case PropertyIds::lastIndexOf:
            return BuiltinFunction::JavascriptArray_LastIndexOf;

        case PropertyIds::reverse:
            return BuiltinFunction::JavascriptArray_Reverse;

        case PropertyIds::shift:
            return BuiltinFunction::JavascriptArray_Shift;

        case PropertyIds::slice:
            return BuiltinFunction::JavascriptArray_Slice;

        case PropertyIds::splice:
            return BuiltinFunction::JavascriptArray_Splice;

        case PropertyIds::unshift:
            return BuiltinFunction::JavascriptArray_Unshift;

        case PropertyIds::apply:
            return BuiltinFunction::JavascriptFunction_Apply;

        case PropertyIds::charAt:
            return BuiltinFunction::JavascriptString_CharAt;

        case PropertyIds::charCodeAt:
            return BuiltinFunction::JavascriptString_CharCodeAt;

        case PropertyIds::fromCharCode:
            return BuiltinFunction::JavascriptString_FromCharCode;

        case PropertyIds::fromCodePoint:
                return BuiltinFunction::JavascriptString_FromCodePoint;

        case PropertyIds::link:
            return BuiltinFunction::JavascriptString_Link;

        case PropertyIds::match:
            return BuiltinFunction::JavascriptString_Match;

        case PropertyIds::replace:
            return BuiltinFunction::JavascriptString_Replace;

        case PropertyIds::search:
            return BuiltinFunction::JavascriptString_Search;

        case PropertyIds::_symbolSearch:
            return BuiltinFunction::JavascriptRegExp_SymbolSearch;

        case PropertyIds::split:
            return BuiltinFunction::JavascriptString_Split;

        case PropertyIds::substr:
            return BuiltinFunction::JavascriptString_Substr;

        case PropertyIds::substring:
            return BuiltinFunction::JavascriptString_Substring;

        case PropertyIds::toLocaleLowerCase:
            return BuiltinFunction::JavascriptString_ToLocaleLowerCase;

        case PropertyIds::toLocaleUpperCase:
            return BuiltinFunction::JavascriptString_ToLocaleUpperCase;

        case PropertyIds::toLowerCase:
            return BuiltinFunction::JavascriptString_ToLowerCase;

        case PropertyIds::toUpperCase:
            return BuiltinFunction::JavascriptString_ToUpperCase;

        case PropertyIds::trim:
            return BuiltinFunction::JavascriptString_Trim;

        case PropertyIds::trimLeft:
            return BuiltinFunction::JavascriptString_TrimLeft;

        case PropertyIds::trimStart:
            return BuiltinFunction::JavascriptString_TrimStart;

        case PropertyIds::trimRight:
            return BuiltinFunction::JavascriptString_TrimRight;

        case PropertyIds::trimEnd:
            return BuiltinFunction::JavascriptString_TrimEnd;

        case PropertyIds::padStart:
            return BuiltinFunction::JavascriptString_PadStart;

        case PropertyIds::padEnd:
            return BuiltinFunction::JavascriptString_PadEnd;

        case PropertyIds::exec:
            return BuiltinFunction::JavascriptRegExp_Exec;

        case PropertyIds::hasOwnProperty:
            return BuiltinFunction::JavascriptObject_HasOwnProperty;

        case PropertyIds::hasOwn:
            return BuiltinFunction::JavascriptObject_HasOwn;

        default:
            return BuiltinFunction::None;
        }
    }

#if DBG
    /*static*/
    void JavascriptLibrary::CheckRegisteredBuiltIns(
        Field(JavascriptFunction*)* builtInFuncs, ScriptContext *scriptContext)
    {
        byte count = BuiltinFunction::Count;
        for (byte index = 0; index < count; index++)
        {
            Assert(!builtInFuncs[index] || (index == GetBuiltInForFuncInfo(builtInFuncs[index]->GetFunctionInfo()->GetLocalFunctionId())));
        }
    }
#endif

    // Returns built-in enum value for given funcInfo. Ultimately this will work for all built-ins (not only Math.*).
    // Used by inliner.
    BuiltinFunction JavascriptLibrary::GetBuiltInForFuncInfo(LocalFunctionId localFuncId)
    {
#define LIBRARY_FUNCTION(target, name, argc, flags, EntryInfo) \
        if(localFuncId == EntryInfo.GetLocalFunctionId()) \
        { \
            return BuiltinFunction::##target##_##name; \
        }
#include "LibraryFunction.h"
#undef LIBRARY_FUNCTION
        return BuiltinFunction::None;
    }


    // Returns true if the function's return type is always float.
    BOOL JavascriptLibrary::IsFltFunc(BuiltinFunction index)
    {
        // Note: MathFunction is one of built-ins.
        if (!JavascriptLibrary::CanFloatPreferenceFunc(index))
        {
            return FALSE;
        }

        Js::BuiltInFlags builtInFlags = JavascriptLibrary::GetFlagsForBuiltIn(index);
        Js::BuiltInArgSpecializationType dstType = Js::JavascriptLibrary::GetBuiltInArgType(builtInFlags, Js::BuiltInArgShift::BIAS_Dst);
        bool isFloatFunc = dstType == Js::BuiltInArgSpecializationType::BIAST_Float;
        return isFloatFunc;
    }

    size_t const JavascriptLibrary::LibraryFunctionArgC[] = {
#define LIBRARY_FUNCTION(obj, name, argc, flags, entry) argc,
#include "LibraryFunction.h"
#undef LIBRARY_FUNCTION
        0
    };

#if ENABLE_DEBUG_CONFIG_OPTIONS
    char16 const * const JavascriptLibrary::LibraryFunctionName[] = {
#define LIBRARY_FUNCTION(obj, name, argc, flags, entry) _u(#obj) _u(".") _u(#name),
#include "LibraryFunction.h"
#undef LIBRARY_FUNCTION
        0
    };
#endif

    int const JavascriptLibrary::LibraryFunctionFlags[] = {
#define LIBRARY_FUNCTION(obj, name, argc, flags, entry) flags,
#include "LibraryFunction.h"
#undef LIBRARY_FUNCTION
        BIF_None
    };

    bool JavascriptLibrary::IsFloatFunctionCallsite(BuiltinFunction index, size_t argc)
    {
        if (IsFltFunc(index))
        {
            Assert(index < BuiltinFunction::Count);
            if (argc)
            {
                return JavascriptLibrary::LibraryFunctionArgC[index] <= (argc - 1 /* this */);
            }
        }

        return false;
    }

    // For abs, min, max -- return can be int or float, but still return true from here.
    BOOL JavascriptLibrary::CanFloatPreferenceFunc(BuiltinFunction index)
    {
        // Shortcut the common case:
        if (index == BuiltinFunction::None)
        {
            return FALSE;
        }

        switch (index)
        {
        case BuiltinFunction::Math_Abs:
        case BuiltinFunction::Math_Acos:
        case BuiltinFunction::Math_Asin:
        case BuiltinFunction::Math_Atan:
        case BuiltinFunction::Math_Cos:
        case BuiltinFunction::Math_Exp:
        case BuiltinFunction::Math_Log:
        case BuiltinFunction::Math_Min:
        case BuiltinFunction::Math_Max:
        case BuiltinFunction::Math_Pow:
        case BuiltinFunction::Math_Random:
        case BuiltinFunction::Math_Sin:
        case BuiltinFunction::Math_Sqrt:
        case BuiltinFunction::Math_Tan:
        case BuiltinFunction::Math_Fround:
            return TRUE;
        }
        return FALSE;
    }

    bool JavascriptLibrary::IsFltBuiltInConst(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case Js::PropertyIds::E:
        case Js::PropertyIds::LN10:
        case Js::PropertyIds::LN2:
        case Js::PropertyIds::LOG2E:
        case Js::PropertyIds::LOG10E:
        case Js::PropertyIds::PI:
        case Js::PropertyIds::SQRT1_2:
        case Js::PropertyIds::SQRT2:
            return true;
        }
        return false;
    }

    bool JavascriptLibrary::ArrayIteratorPrototypeHasUserDefinedNext(ScriptContext *scriptContext)
    {
        Var arrayIteratorPrototypeNext = nullptr;
        ImplicitCallFlags flags = scriptContext->GetThreadContext()->TryWithDisabledImplicitCall(
                [&]() { arrayIteratorPrototypeNext = JavascriptOperators::GetPropertyNoCache(scriptContext->GetLibrary()->GetArrayIteratorPrototype(), PropertyIds::next, scriptContext); });

        return (flags != ImplicitCall_None) || arrayIteratorPrototypeNext != scriptContext->GetLibrary()->GetArrayIteratorPrototypeBuiltinNextFunction();
    }

    bool JavascriptLibrary::InitializeBigIntConstructor(DynamicObject* bigIntConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        const int numberOfProperties = 3;
        typeHandler->Convert(bigIntConstructor, mode, numberOfProperties);

        // TODO(BigInt): Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterBigInt
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = bigIntConstructor->GetScriptContext();
        JavascriptLibrary* library = bigIntConstructor->GetLibrary();
        library->AddMember(bigIntConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(bigIntConstructor, PropertyIds::prototype, library->bigintPrototype, PropertyNone);
        library->AddMember(bigIntConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::BigInt), PropertyConfigurable);

        bigIntConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }


    bool JavascriptLibrary::InitializeNumberConstructor(DynamicObject* numberConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(numberConstructor, mode, 17);

        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterNumber
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = numberConstructor->GetScriptContext();
        JavascriptLibrary* library = numberConstructor->GetLibrary();
        library->AddMember(numberConstructor, PropertyIds::length,            TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(numberConstructor, PropertyIds::prototype,         library->numberPrototype,     PropertyNone);
        library->AddMember(numberConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Number), PropertyConfigurable);
        library->AddMember(numberConstructor, PropertyIds::MAX_VALUE,         library->maxValue,            PropertyNone);
        library->AddMember(numberConstructor, PropertyIds::MIN_VALUE,         library->minValue,            PropertyNone);
        library->AddMember(numberConstructor, PropertyIds::NaN,               library->nan,                 PropertyNone);
        library->AddMember(numberConstructor, PropertyIds::NEGATIVE_INFINITY, library->negativeInfinite,    PropertyNone);
        library->AddMember(numberConstructor, PropertyIds::POSITIVE_INFINITY, library->positiveInfinite,    PropertyNone);

        if (scriptContext->GetConfig()->IsES6NumberExtensionsEnabled())
        {
#ifdef DBG
            double epsilon = 0.0;
            for (double next = 1.0; next + 1.0 != 1.0; next = next / 2.0)
            {
                epsilon = next;
            }
            Assert(epsilon == Math::EPSILON);
#endif
            library->AddMember(numberConstructor, PropertyIds::EPSILON,     JavascriptNumber::New(Math::EPSILON,     scriptContext), PropertyNone);
            library->AddMember(numberConstructor, PropertyIds::MAX_SAFE_INTEGER, JavascriptNumber::New(Math::MAX_SAFE_INTEGER, scriptContext), PropertyNone);
            library->AddMember(numberConstructor, PropertyIds::MIN_SAFE_INTEGER, JavascriptNumber::New(Math::MIN_SAFE_INTEGER, scriptContext), PropertyNone);

            AssertMsg(library->parseIntFunctionObject != nullptr, "Where is parseIntFunctionObject? Should have been initialized with Global object initialization");
            AssertMsg(library->parseFloatFunctionObject != nullptr, "Where is parseIntFunctionObject? Should have been initialized with Global object initialization");
            library->AddMember(numberConstructor, PropertyIds::parseInt, library->parseIntFunctionObject);
            library->AddMember(numberConstructor, PropertyIds::parseFloat, library->parseFloatFunctionObject);
            library->AddFunctionToLibraryObject(numberConstructor, PropertyIds::isNaN, &JavascriptNumber::EntryInfo::IsNaN, 1);
            library->AddFunctionToLibraryObject(numberConstructor, PropertyIds::isFinite, &JavascriptNumber::EntryInfo::IsFinite, 1);
            library->AddFunctionToLibraryObject(numberConstructor, PropertyIds::isInteger, &JavascriptNumber::EntryInfo::IsInteger, 1);
            library->AddFunctionToLibraryObject(numberConstructor, PropertyIds::isSafeInteger, &JavascriptNumber::EntryInfo::IsSafeInteger, 1);
        }

        numberConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeBigIntPrototype(DynamicObject* bigIntPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        const int numberOfProperties = 1;
        typeHandler->Convert(bigIntPrototype, mode, numberOfProperties);
        // TODO(BigInt): Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterBigInt
        // so that the update is in sync with profiler
        JavascriptLibrary* library = bigIntPrototype->GetLibrary();
        library->AddMember(bigIntPrototype, PropertyIds::constructor, library->bigIntConstructor);

        bigIntPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeNumberPrototype(DynamicObject* numberPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(numberPrototype, mode, 8);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterNumber
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = numberPrototype->GetScriptContext();
        JavascriptLibrary* library = numberPrototype->GetLibrary();
        library->AddMember(numberPrototype, PropertyIds::constructor, library->numberConstructor);
        scriptContext->SetBuiltInLibraryFunction(JavascriptNumber::EntryInfo::ToExponential.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(numberPrototype, PropertyIds::toExponential, &JavascriptNumber::EntryInfo::ToExponential, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptNumber::EntryInfo::ToFixed.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(numberPrototype, PropertyIds::toFixed, &JavascriptNumber::EntryInfo::ToFixed, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptNumber::EntryInfo::ToPrecision.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(numberPrototype, PropertyIds::toPrecision, &JavascriptNumber::EntryInfo::ToPrecision, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptNumber::EntryInfo::ToLocaleString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(numberPrototype, PropertyIds::toLocaleString, &JavascriptNumber::EntryInfo::ToLocaleString, 0));
        scriptContext->SetBuiltInLibraryFunction(JavascriptNumber::EntryInfo::ToString.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(numberPrototype, PropertyIds::toString, &JavascriptNumber::EntryInfo::ToString, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptNumber::EntryInfo::ValueOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(numberPrototype, PropertyIds::valueOf, &JavascriptNumber::EntryInfo::ValueOf, 0));

        numberPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeObjectConstructor(DynamicObject* objectConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterObject
        // so that the update is in sync with profiler
        JavascriptLibrary* library = objectConstructor->GetLibrary();
        ScriptContext* scriptContext = objectConstructor->GetScriptContext();
        int propertyCount = 19;
        if (scriptContext->GetConfig()->IsES6ObjectExtensionsEnabled())
        {
            propertyCount += 2;
        }

        if (scriptContext->GetConfig()->IsESObjectGetOwnPropertyDescriptorsEnabled())
        {
            propertyCount++;
        }

        if (scriptContext->GetConfig()->IsES7ValuesEntriesEnabled())
        {
            propertyCount += 2;
        }

#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            propertyCount++;
        }
#endif

        typeHandler->Convert(objectConstructor, mode, propertyCount);

        library->AddMember(objectConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(objectConstructor, PropertyIds::prototype, library->objectPrototype, PropertyNone);
        library->AddMember(objectConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Object), PropertyConfigurable);

        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::DefineProperty.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::defineProperty, &JavascriptObject::EntryInfo::DefineProperty, 3));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::GetOwnPropertyDescriptor.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::getOwnPropertyDescriptor, &JavascriptObject::EntryInfo::GetOwnPropertyDescriptor, 2));
        if (scriptContext->GetConfig()->IsESObjectGetOwnPropertyDescriptorsEnabled())
        {
            scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::GetOwnPropertyDescriptors.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::getOwnPropertyDescriptors, &JavascriptObject::EntryInfo::GetOwnPropertyDescriptors, 1));
        }
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::DefineProperties.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::defineProperties, &JavascriptObject::EntryInfo::DefineProperties, 2));
        library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::create, &JavascriptObject::EntryInfo::Create, 2);
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Seal.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::seal, &JavascriptObject::EntryInfo::Seal, 1));
        library->AddMember(objectConstructor, PropertyIds::freeze, library->EnsureObjectFreezeFunction(), PropertyBuiltInMethodDefaults);
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Freeze.GetOriginalEntryPoint(), library->EnsureObjectFreezeFunction());
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::PreventExtensions.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::preventExtensions, &JavascriptObject::EntryInfo::PreventExtensions, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::IsSealed.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::isSealed, &JavascriptObject::EntryInfo::IsSealed, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::IsFrozen.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::isFrozen, &JavascriptObject::EntryInfo::IsFrozen, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::IsExtensible.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::isExtensible, &JavascriptObject::EntryInfo::IsExtensible, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::GetPrototypeOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::getPrototypeOf, &JavascriptObject::EntryInfo::GetPrototypeOf, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Keys.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::keys, &JavascriptObject::EntryInfo::Keys, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::GetOwnPropertyNames.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::getOwnPropertyNames, &JavascriptObject::EntryInfo::GetOwnPropertyNames, 1));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::SetPrototypeOf.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::setPrototypeOf, &JavascriptObject::EntryInfo::SetPrototypeOf, 2));
        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::GetOwnPropertySymbols.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::getOwnPropertySymbols, &JavascriptObject::EntryInfo::GetOwnPropertySymbols, 1));
        if (scriptContext->GetConfig()->IsES6ObjectExtensionsEnabled())
        {
            scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Is.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::is, &JavascriptObject::EntryInfo::Is, 2));
            scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Assign.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::assign, &JavascriptObject::EntryInfo::Assign, 2));
        }

        if (scriptContext->GetConfig()->IsES7ValuesEntriesEnabled())
        {
            scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Values.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::values, &JavascriptObject::EntryInfo::Values, 1));
            scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::Entries.GetOriginalEntryPoint(),
                library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::entries, &JavascriptObject::EntryInfo::Entries, 1));
        }

        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::HasOwn.GetOriginalEntryPoint(),
            library->AddFunctionToLibraryObject(objectConstructor, PropertyIds::hasOwn, &JavascriptObject::EntryInfo::HasOwn, 2));

#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            EnsureBuiltInEngineIsReady(JsBuiltInFile::Object_constructor, scriptContext);
        }
#endif

        objectConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    JavascriptFunction* JavascriptLibrary::EnsureObjectFreezeFunction()
    {
        if (objectFreezeFunction == nullptr)
        {
            objectFreezeFunction = DefaultCreateFunction(&JavascriptObject::EntryInfo::Freeze, 1, nullptr, nullptr, PropertyIds::freeze);
        }
        return objectFreezeFunction;
    }

    bool JavascriptLibrary::InitializeObjectPrototype(DynamicObject* objectPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptLibrary* library = objectPrototype->GetLibrary();
        ScriptContext* scriptContext = objectPrototype->GetScriptContext();
        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

        typeHandler->Convert(objectPrototype, mode, 11, true);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterObject
        // so that the update is in sync with profiler
        library->AddMember(objectPrototype, PropertyIds::constructor, library->objectConstructor);
        builtinFuncs[BuiltinFunction::JavascriptObject_HasOwnProperty] = library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::hasOwnProperty, &JavascriptObject::EntryInfo::HasOwnProperty, 1);
        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::propertyIsEnumerable, &JavascriptObject::EntryInfo::PropertyIsEnumerable, 1);
        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::isPrototypeOf, &JavascriptObject::EntryInfo::IsPrototypeOf, 1);
        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::toLocaleString, &JavascriptObject::EntryInfo::ToLocaleString, 0);

        library->objectToStringFunction = library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::toString, &JavascriptObject::EntryInfo::ToString, 0);
        library->objectValueOfFunction = library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::valueOf, &JavascriptObject::EntryInfo::ValueOf, 0);

        scriptContext->SetBuiltInLibraryFunction(JavascriptObject::EntryInfo::ToString.GetOriginalEntryPoint(), library->objectToStringFunction);

        bool hadOnlyWritableDataProperties = objectPrototype->GetDynamicType()->GetTypeHandler()->GetHasOnlyWritableDataProperties();
        objectPrototype->SetAccessors(PropertyIds::__proto__, library->Get__proto__getterFunction(), library->Get__proto__setterFunction(), PropertyOperation_NonFixedValue);
        objectPrototype->SetEnumerable(PropertyIds::__proto__, FALSE);
        // Let's pretend __proto__ is actually writable.  We'll make sure we always go through a special code path when writing to it.
        if (hadOnlyWritableDataProperties)
        {
            objectPrototype->GetDynamicType()->GetTypeHandler()->SetHasOnlyWritableDataProperties();
        }

        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::__defineGetter__, &JavascriptObject::EntryInfo::DefineGetter, 2);
        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::__defineSetter__, &JavascriptObject::EntryInfo::DefineSetter, 2);
        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::__lookupGetter__, &JavascriptObject::EntryInfo::LookupGetter, 1);
        library->AddFunctionToLibraryObject(objectPrototype, PropertyIds::__lookupSetter__, &JavascriptObject::EntryInfo::LookupSetter, 1);

        objectPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeRegexConstructor(DynamicObject* regexConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptLibrary* library = regexConstructor->GetLibrary();
        ScriptContext* scriptContext = regexConstructor->GetScriptContext();
        typeHandler->Convert(regexConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterRegExp
        // so that the update is in sync with profiler
        library->AddMember(regexConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(2), PropertyConfigurable);
        library->AddMember(regexConstructor, PropertyIds::prototype, library->regexPrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(regexConstructor, &JavascriptRegExp::EntryInfo::GetterSymbolSpecies);
        library->AddMember(regexConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::RegExp), PropertyConfigurable);

        regexConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeRegexPrototype(DynamicObject* regexPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(regexPrototype, mode, 26);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterRegExp
        // so that the update is in sync with profiler
        JavascriptFunction * func;
        JavascriptLibrary* library = regexPrototype->GetLibrary();
        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

        library->AddMember(regexPrototype, PropertyIds::constructor, library->regexConstructor);
        library->regexConstructorSlotIndex = 0;
        Assert(regexPrototype->GetSlot(library->regexConstructorSlotIndex) == library->regexConstructor);

        func = library->AddFunctionToLibraryObject(regexPrototype, PropertyIds::exec, &JavascriptRegExp::EntryInfo::Exec, 1);
        builtinFuncs[BuiltinFunction::JavascriptRegExp_Exec] = func;
        library->regexExecFunction = func;
        library->regexExecSlotIndex = 1;
        Assert(regexPrototype->GetSlot(library->regexExecSlotIndex) == library->regexExecFunction);

        library->AddFunctionToLibraryObject(regexPrototype, PropertyIds::test, &JavascriptRegExp::EntryInfo::Test, 1);
        library->AddFunctionToLibraryObject(regexPrototype, PropertyIds::toString, &JavascriptRegExp::EntryInfo::ToString, 0);
        // This is deprecated. Should be guarded with appropriate version flag.
        library->AddFunctionToLibraryObject(regexPrototype, PropertyIds::compile, &JavascriptRegExp::EntryInfo::Compile, 2);

        const ScriptConfiguration* scriptConfig = regexPrototype->GetScriptContext()->GetConfig();

        if (scriptConfig->IsES6RegExPrototypePropertiesEnabled())
        {
            library->regexGlobalGetterFunction =
                library->AddGetterToLibraryObject(regexPrototype, PropertyIds::global, &JavascriptRegExp::EntryInfo::GetterGlobal);
            library->regexGlobalGetterSlotIndex = 5;
            Assert(regexPrototype->GetSlot(library->regexGlobalGetterSlotIndex) == library->regexGlobalGetterFunction);

            library->AddAccessorsToLibraryObject(regexPrototype, PropertyIds::ignoreCase, &JavascriptRegExp::EntryInfo::GetterIgnoreCase, nullptr);
            library->AddAccessorsToLibraryObject(regexPrototype, PropertyIds::multiline, &JavascriptRegExp::EntryInfo::GetterMultiline, nullptr);
            library->AddAccessorsToLibraryObject(regexPrototype, PropertyIds::options, &JavascriptRegExp::EntryInfo::GetterOptions, nullptr);
            library->AddAccessorsToLibraryObject(regexPrototype, PropertyIds::source, &JavascriptRegExp::EntryInfo::GetterSource, nullptr);

            library->regexFlagsGetterFunction =
                library->AddGetterToLibraryObject(regexPrototype, PropertyIds::flags, &JavascriptRegExp::EntryInfo::GetterFlags);
            library->regexFlagsGetterSlotIndex = 15;
            Assert(regexPrototype->GetSlot(library->regexFlagsGetterSlotIndex) == library->regexFlagsGetterFunction);

            if (scriptConfig->IsES6RegExStickyEnabled())
            {
                library->regexStickyGetterFunction =
                    library->AddGetterToLibraryObject(regexPrototype, PropertyIds::sticky, &JavascriptRegExp::EntryInfo::GetterSticky);
                library->regexStickyGetterSlotIndex = 17;
                Assert(regexPrototype->GetSlot(library->regexStickyGetterSlotIndex) == library->regexStickyGetterFunction);
            }

            if (scriptConfig->IsES6UnicodeExtensionsEnabled())
            {
                library->regexUnicodeGetterFunction =
                    library->AddGetterToLibraryObject(regexPrototype, PropertyIds::unicode, &JavascriptRegExp::EntryInfo::GetterUnicode);
                library->regexUnicodeGetterSlotIndex = scriptConfig->IsES6RegExStickyEnabled() ? 19 : 17;
                Assert(regexPrototype->GetSlot(library->regexUnicodeGetterSlotIndex) == library->regexUnicodeGetterFunction);
            }

            if (scriptConfig->IsES2018RegExDotAllEnabled())
            {
                library->regexDotAllGetterFunction =
                    library->AddGetterToLibraryObject(regexPrototype, PropertyIds::dotAll, &JavascriptRegExp::EntryInfo::GetterDotAll);
                library->regexDotAllGetterSlotIndex = 21 -
                    (scriptConfig->IsES6UnicodeExtensionsEnabled() ? 0 : 2) - (scriptConfig->IsES6RegExStickyEnabled() ? 0 : 2);
                Assert(regexPrototype->GetSlot(library->regexDotAllGetterSlotIndex) == library->regexDotAllGetterFunction);
            }
        }

        if (scriptConfig->IsES6RegExSymbolsEnabled())
        {
            library->AddFunctionToLibraryObjectWithName(
                regexPrototype,
                PropertyIds::_symbolMatch,
                PropertyIds::_RuntimeFunctionNameId_match,
                &JavascriptRegExp::EntryInfo::SymbolMatch,
                1);
            library->AddFunctionToLibraryObjectWithName(
                regexPrototype,
                PropertyIds::_symbolReplace,
                PropertyIds::_RuntimeFunctionNameId_replace,
                &JavascriptRegExp::EntryInfo::SymbolReplace,
                2);
            builtinFuncs[BuiltinFunction::JavascriptRegExp_SymbolSearch] = library->AddFunctionToLibraryObjectWithName(
                regexPrototype,
                PropertyIds::_symbolSearch,
                PropertyIds::_RuntimeFunctionNameId_search,
                &JavascriptRegExp::EntryInfo::SymbolSearch,
                1);
            library->AddFunctionToLibraryObjectWithName(
                regexPrototype,
                PropertyIds::_symbolSplit,
                PropertyIds::_RuntimeFunctionNameId_split,
                &JavascriptRegExp::EntryInfo::SymbolSplit,
                2);
        }

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, library->GetScriptContext()));

        regexPrototype->SetHasNoEnumerableProperties(true);

        library->regexPrototypeType = regexPrototype->GetDynamicType();

        return true;
    }

    bool JavascriptLibrary::InitializeStringConstructor(DynamicObject* stringConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(stringConstructor, mode, 6);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterString
        // so that the update is in sync with profiler
        JavascriptLibrary* library = stringConstructor->GetLibrary();
        ScriptContext* scriptContext = stringConstructor->GetScriptContext();

        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();
        library->AddMember(stringConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable);
        library->AddMember(stringConstructor, PropertyIds::prototype, library->stringPrototype, PropertyNone);
        library->AddMember(stringConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::String), PropertyConfigurable);

        builtinFuncs[BuiltinFunction::JavascriptString_FromCharCode]  = library->AddFunctionToLibraryObject(stringConstructor, PropertyIds::fromCharCode,  &JavascriptString::EntryInfo::FromCharCode,  1);
        if(scriptContext->GetConfig()->IsES6UnicodeExtensionsEnabled())
        {
            builtinFuncs[BuiltinFunction::JavascriptString_FromCodePoint] = library->AddFunctionToLibraryObject(stringConstructor, PropertyIds::fromCodePoint, &JavascriptString::EntryInfo::FromCodePoint, 1);
        }

        /* No inlining                String_Raw           */ library->AddFunctionToLibraryObject(stringConstructor, PropertyIds::raw,           &JavascriptString::EntryInfo::Raw,           1);

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        stringConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeStringPrototype(DynamicObject* stringPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(stringPrototype, mode, 39);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterString
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = stringPrototype->GetScriptContext();
        JavascriptLibrary* library = stringPrototype->GetLibrary();
        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();

        library->AddMember(stringPrototype, PropertyIds::constructor, library->stringConstructor);

        builtinFuncs[BuiltinFunction::JavascriptString_IndexOf]       = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::indexOf,            &JavascriptString::EntryInfo::IndexOf,              1);
        builtinFuncs[BuiltinFunction::JavascriptString_LastIndexOf]   = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::lastIndexOf,        &JavascriptString::EntryInfo::LastIndexOf,          1);
        builtinFuncs[BuiltinFunction::JavascriptString_Replace]       = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::replace,            &JavascriptString::EntryInfo::Replace,              2);
        builtinFuncs[BuiltinFunction::JavascriptString_Search]        = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::search,             &JavascriptString::EntryInfo::Search,               1);
        builtinFuncs[BuiltinFunction::JavascriptString_Slice]         = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::slice,              &JavascriptString::EntryInfo::Slice,                2);

        if (CONFIG_FLAG(ES6Unicode))
        {
            builtinFuncs[BuiltinFunction::JavascriptString_CodePointAt]   = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::codePointAt,        &JavascriptString::EntryInfo::CodePointAt,          1);
            /* builtinFuncs[BuiltinFunction::String_Normalize] =*/library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::normalize,          &JavascriptString::EntryInfo::Normalize,            0);
        }

        builtinFuncs[BuiltinFunction::JavascriptString_At]                = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::at,                 &JavascriptString::EntryInfo::At,               1);
        builtinFuncs[BuiltinFunction::JavascriptString_CharAt]            = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::charAt,             &JavascriptString::EntryInfo::CharAt,               1);
        builtinFuncs[BuiltinFunction::JavascriptString_CharCodeAt]        = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::charCodeAt,         &JavascriptString::EntryInfo::CharCodeAt,           1);
        builtinFuncs[BuiltinFunction::JavascriptString_Concat]            = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::concat,             &JavascriptString::EntryInfo::Concat,               1);
        // Don't inline String.prototype.localeCompare because it immediately calls back into Intl.js, which can break implicitCallFlags
        /* No inlining                String_LocaleCompare */               library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::localeCompare,      &JavascriptString::EntryInfo::LocaleCompare,        1);
        builtinFuncs[BuiltinFunction::JavascriptString_Match]             = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::match,              &JavascriptString::EntryInfo::Match,                1);
        builtinFuncs[BuiltinFunction::JavascriptString_Split]             = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::split,              &JavascriptString::EntryInfo::Split,                2);
        builtinFuncs[BuiltinFunction::JavascriptString_Substring]         = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::substring,          &JavascriptString::EntryInfo::Substring,            2);
        builtinFuncs[BuiltinFunction::JavascriptString_Substr]            = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::substr,             &JavascriptString::EntryInfo::Substr,               2);
        builtinFuncs[BuiltinFunction::JavascriptString_ToLocaleLowerCase] = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::toLocaleLowerCase,  &JavascriptString::EntryInfo::ToLocaleLowerCase,    0);
        builtinFuncs[BuiltinFunction::JavascriptString_ToLocaleUpperCase] = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::toLocaleUpperCase,  &JavascriptString::EntryInfo::ToLocaleUpperCase,    0);
        builtinFuncs[BuiltinFunction::JavascriptString_ToLowerCase]       = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::toLowerCase,        &JavascriptString::EntryInfo::ToLowerCase,          0);
        scriptContext->SetBuiltInLibraryFunction(JavascriptString::EntryInfo::ToString.GetOriginalEntryPoint(),
                                                                  library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::toString,           &JavascriptString::EntryInfo::ToString,             0));
        builtinFuncs[BuiltinFunction::JavascriptString_ToUpperCase]       = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::toUpperCase,        &JavascriptString::EntryInfo::ToUpperCase,          0);
        builtinFuncs[BuiltinFunction::JavascriptString_Trim]              = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::trim,               &JavascriptString::EntryInfo::Trim,                 0);

        scriptContext->SetBuiltInLibraryFunction(JavascriptString::EntryInfo::ValueOf.GetOriginalEntryPoint(),
                                                                  library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::valueOf,            &JavascriptString::EntryInfo::ValueOf,              0));

            /* No inlining                String_Anchor        */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::anchor,             &JavascriptString::EntryInfo::Anchor,               1);
            /* No inlining                String_Big           */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::big,                &JavascriptString::EntryInfo::Big,                  0);
            /* No inlining                String_Blink         */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::blink,              &JavascriptString::EntryInfo::Blink,                0);
            /* No inlining                String_Bold          */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::bold,               &JavascriptString::EntryInfo::Bold,                 0);
            /* No inlining                String_Fixed         */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::fixed,              &JavascriptString::EntryInfo::Fixed,                0);
            /* No inlining                String_FontColor     */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::fontcolor,          &JavascriptString::EntryInfo::FontColor,            1);
            /* No inlining                String_FontSize      */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::fontsize,           &JavascriptString::EntryInfo::FontSize,             1);
            /* No inlining                String_Italics       */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::italics,            &JavascriptString::EntryInfo::Italics,              0);
            builtinFuncs[BuiltinFunction::JavascriptString_Link]          = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::link,               &JavascriptString::EntryInfo::Link,                 1);
            /* No inlining                String_Small         */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::Small,              &JavascriptString::EntryInfo::Small,                0);
            /* No inlining                String_Strike        */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::strike,             &JavascriptString::EntryInfo::Strike,               0);
            /* No inlining                String_Sub           */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::sub,                &JavascriptString::EntryInfo::Sub,                  0);
            /* No inlining                String_Sup           */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::sup,                &JavascriptString::EntryInfo::Sup,                  0);

        if (scriptContext->GetConfig()->IsES6StringExtensionsEnabled())
        {
            /* No inlining                String_Repeat        */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::repeat,             &JavascriptString::EntryInfo::Repeat,               1);
            /* No inlining                String_StartsWith    */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::startsWith,         &JavascriptString::EntryInfo::StartsWith,           1);
            /* No inlining                String_EndsWith      */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::endsWith,           &JavascriptString::EntryInfo::EndsWith,             1);
            /* No inlining                String_Includes      */ library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::includes,           &JavascriptString::EntryInfo::Includes,             1);
            builtinFuncs[BuiltinFunction::JavascriptString_TrimStart]      = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::trimStart,           &JavascriptString::EntryInfo::TrimStart,             0);
            library->AddMember(stringPrototype, PropertyIds::trimLeft, builtinFuncs[BuiltinFunction::JavascriptString_TrimStart], PropertyBuiltInMethodDefaults);
            builtinFuncs[BuiltinFunction::JavascriptString_TrimEnd]     = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::trimEnd,          &JavascriptString::EntryInfo::TrimEnd,            0);
            library->AddMember(stringPrototype, PropertyIds::trimRight, builtinFuncs[BuiltinFunction::JavascriptString_TrimEnd], PropertyBuiltInMethodDefaults);
        }

        library->AddFunctionToLibraryObjectWithName(stringPrototype, PropertyIds::_symbolIterator, PropertyIds::_RuntimeFunctionNameId_iterator, &JavascriptString::EntryInfo::SymbolIterator, 0);

            builtinFuncs[BuiltinFunction::JavascriptString_PadStart] = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::padStart, &JavascriptString::EntryInfo::PadStart, 1);
            builtinFuncs[BuiltinFunction::JavascriptString_PadEnd] = library->AddFunctionToLibraryObject(stringPrototype, PropertyIds::padEnd, &JavascriptString::EntryInfo::PadEnd, 1);

        DebugOnly(CheckRegisteredBuiltIns(builtinFuncs, scriptContext));

        stringPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeMapConstructor(DynamicObject* mapConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(mapConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterMap
        // so that the update is in sync with profiler
        JavascriptLibrary* library = mapConstructor->GetLibrary();
        ScriptContext* scriptContext = mapConstructor->GetScriptContext();
        library->AddMember(mapConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable);
        library->AddMember(mapConstructor, PropertyIds::prototype, library->mapPrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(mapConstructor, &JavascriptMap::EntryInfo::GetterSymbolSpecies);
        library->AddMember(mapConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Map), PropertyConfigurable);

        mapConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeMapPrototype(DynamicObject* mapPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(mapPrototype, mode, 13, true);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterMap
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = mapPrototype->GetScriptContext();
        JavascriptLibrary* library = mapPrototype->GetLibrary();
        library->AddMember(mapPrototype, PropertyIds::constructor, library->mapConstructor);

        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::clear, &JavascriptMap::EntryInfo::Clear, 0);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::delete_, &JavascriptMap::EntryInfo::Delete, 1);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::forEach, &JavascriptMap::EntryInfo::ForEach, 1);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::get, &JavascriptMap::EntryInfo::Get, 1);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::has, &JavascriptMap::EntryInfo::Has, 1);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::set, &JavascriptMap::EntryInfo::Set, 2);

        library->AddAccessorsToLibraryObject(mapPrototype, PropertyIds::size, &JavascriptMap::EntryInfo::SizeGetter, nullptr);

        JavascriptFunction* entriesFunc;
        entriesFunc = library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::entries, &JavascriptMap::EntryInfo::Entries, 0);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::keys, &JavascriptMap::EntryInfo::Keys, 0);
        library->AddFunctionToLibraryObject(mapPrototype, PropertyIds::values, &JavascriptMap::EntryInfo::Values, 0);
        library->AddMember(mapPrototype, PropertyIds::_symbolIterator, entriesFunc);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(mapPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Map")), PropertyConfigurable);
        }

        mapPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeSetConstructor(DynamicObject* setConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(setConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSet
        // so that the update is in sync with profiler
        JavascriptLibrary* library = setConstructor->GetLibrary();
        ScriptContext* scriptContext = setConstructor->GetScriptContext();
        library->AddMember(setConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable);
        library->AddMember(setConstructor, PropertyIds::prototype, library->setPrototype, PropertyNone);
        library->AddSpeciesAccessorsToLibraryObject(setConstructor, &JavascriptSet::EntryInfo::GetterSymbolSpecies);

        library->AddMember(setConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::Set), PropertyConfigurable);

        setConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeSetPrototype(DynamicObject* setPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(setPrototype, mode, 12, true);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSet
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = setPrototype->GetScriptContext();
        JavascriptLibrary* library = setPrototype->GetLibrary();
        library->AddMember(setPrototype, PropertyIds::constructor, library->setConstructor);

        library->AddFunctionToLibraryObject(setPrototype, PropertyIds::add, &JavascriptSet::EntryInfo::Add, 1);
        library->AddFunctionToLibraryObject(setPrototype, PropertyIds::clear, &JavascriptSet::EntryInfo::Clear, 0);
        library->AddFunctionToLibraryObject(setPrototype, PropertyIds::delete_, &JavascriptSet::EntryInfo::Delete, 1);
        library->AddFunctionToLibraryObject(setPrototype, PropertyIds::forEach, &JavascriptSet::EntryInfo::ForEach, 1);
        library->AddFunctionToLibraryObject(setPrototype, PropertyIds::has, &JavascriptSet::EntryInfo::Has, 1);

        library->AddAccessorsToLibraryObject(setPrototype, PropertyIds::size, &JavascriptSet::EntryInfo::SizeGetter, nullptr);

        JavascriptFunction* valuesFunc;
        library->AddFunctionToLibraryObject(setPrototype, PropertyIds::entries, &JavascriptSet::EntryInfo::Entries, 0);
        valuesFunc = library->AddFunctionToLibraryObject(setPrototype, PropertyIds::values, &JavascriptSet::EntryInfo::Values, 0);
        library->AddMember(setPrototype, PropertyIds::keys, valuesFunc);
        library->AddMember(setPrototype, PropertyIds::_symbolIterator, valuesFunc);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(setPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Set")), PropertyConfigurable);
        }

        setPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWeakMapConstructor(DynamicObject* weakMapConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(weakMapConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterWeakMap
        // so that the update is in sync with profiler
        JavascriptLibrary* library = weakMapConstructor->GetLibrary();
        ScriptContext* scriptContext = weakMapConstructor->GetScriptContext();
        library->AddMember(weakMapConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable);
        library->AddMember(weakMapConstructor, PropertyIds::prototype, library->weakMapPrototype, PropertyNone);
        library->AddMember(weakMapConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::WeakMap), PropertyConfigurable);

        weakMapConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWeakMapPrototype(DynamicObject* weakMapPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(weakMapPrototype, mode, 6);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterWeakMap
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = weakMapPrototype->GetScriptContext();
        JavascriptLibrary* library = weakMapPrototype->GetLibrary();
        library->AddMember(weakMapPrototype, PropertyIds::constructor, library->weakMapConstructor);

        library->AddFunctionToLibraryObject(weakMapPrototype, PropertyIds::delete_, &JavascriptWeakMap::EntryInfo::Delete, 1);
        library->AddFunctionToLibraryObject(weakMapPrototype, PropertyIds::get, &JavascriptWeakMap::EntryInfo::Get, 1);
        library->AddFunctionToLibraryObject(weakMapPrototype, PropertyIds::has, &JavascriptWeakMap::EntryInfo::Has, 1);
        library->AddFunctionToLibraryObject(weakMapPrototype, PropertyIds::set, &JavascriptWeakMap::EntryInfo::Set, 2);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(weakMapPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WeakMap")), PropertyConfigurable);
        }

        weakMapPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWeakSetConstructor(DynamicObject* weakSetConstructor, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(weakSetConstructor, mode, 3);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterWeakSet
        // so that the update is in sync with profiler
        JavascriptLibrary* library = weakSetConstructor->GetLibrary();
        ScriptContext* scriptContext = weakSetConstructor->GetScriptContext();
        library->AddMember(weakSetConstructor, PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable);
        library->AddMember(weakSetConstructor, PropertyIds::prototype, library->weakSetPrototype, PropertyNone);
        library->AddMember(weakSetConstructor, PropertyIds::name, scriptContext->GetPropertyString(PropertyIds::WeakSet), PropertyConfigurable);

        weakSetConstructor->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeWeakSetPrototype(DynamicObject* weakSetPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(weakSetPrototype, mode, 5);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterWeakSet
        // so that the update is in sync with profiler
        ScriptContext* scriptContext = weakSetPrototype->GetScriptContext();
        JavascriptLibrary* library = weakSetPrototype->GetLibrary();
        library->AddMember(weakSetPrototype, PropertyIds::constructor, library->weakSetConstructor);

        library->AddFunctionToLibraryObject(weakSetPrototype, PropertyIds::add, &JavascriptWeakSet::EntryInfo::Add, 1);
        library->AddFunctionToLibraryObject(weakSetPrototype, PropertyIds::delete_, &JavascriptWeakSet::EntryInfo::Delete, 1);
        library->AddFunctionToLibraryObject(weakSetPrototype, PropertyIds::has, &JavascriptWeakSet::EntryInfo::Has, 1);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(weakSetPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("WeakSet")), PropertyConfigurable);
        }

        weakSetPrototype->SetHasNoEnumerableProperties(true);

        return true;
    }

    bool JavascriptLibrary::InitializeIteratorPrototype(DynamicObject* iteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(iteratorPrototype, mode, 1);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterIterator
        // so that the update is in sync with profiler

        JavascriptLibrary* library = iteratorPrototype->GetLibrary();

        library->AddFunctionToLibraryObjectWithName(iteratorPrototype, PropertyIds::_symbolIterator, PropertyIds::_RuntimeFunctionNameId_iterator,
            &JavascriptIterator::EntryInfo::SymbolIterator, 0);

        return true;
    }

    bool JavascriptLibrary::InitializeAsyncIteratorPrototype(DynamicObject* asyncIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(asyncIteratorPrototype, mode, 1);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterIterator
        // so that the update is in sync with profiler

        JavascriptLibrary* library = asyncIteratorPrototype->GetLibrary();

        library->AddFunctionToLibraryObjectWithName(asyncIteratorPrototype, PropertyIds::_symbolAsyncIterator, PropertyIds::_RuntimeFunctionNameId_asyncIterator,
            &JavascriptIterator::EntryInfo::SymbolIterator, 0);

        return true;
    }

    bool JavascriptLibrary::InitializeArrayIteratorPrototype(DynamicObject* arrayIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {

        typeHandler->Convert(arrayIteratorPrototype, mode, 2);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterArrayIterator
        // so that the update is in sync with profiler

        JavascriptLibrary* library = arrayIteratorPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();

#ifdef ENABLE_JS_BUILTINS
        Assert(!scriptContext->IsJsBuiltInEnabled());
#endif

        library->arrayIteratorPrototypeBuiltinNextFunction = library->AddFunctionToLibraryObject(arrayIteratorPrototype, PropertyIds::next, &JavascriptArrayIterator::EntryInfo::Next, 0);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(arrayIteratorPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Array Iterator")), PropertyConfigurable);
        }

        return true;
    }

    bool JavascriptLibrary::InitializeMapIteratorPrototype(DynamicObject* mapIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(mapIteratorPrototype, mode, 2);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterMapIterator
        // so that the update is in sync with profiler

        JavascriptLibrary* library = mapIteratorPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();

        library->AddFunctionToLibraryObject(mapIteratorPrototype, PropertyIds::next, &JavascriptMapIterator::EntryInfo::Next, 0);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(mapIteratorPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Map Iterator")), PropertyConfigurable);
        }

        return true;
    }

    bool JavascriptLibrary::InitializeSetIteratorPrototype(DynamicObject* setIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(setIteratorPrototype, mode, 2);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterSetIterator
        // so that the update is in sync with profiler

        JavascriptLibrary* library = setIteratorPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();
        library->AddFunctionToLibraryObject(setIteratorPrototype, PropertyIds::next, &JavascriptSetIterator::EntryInfo::Next, 0);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(setIteratorPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("Set Iterator")), PropertyConfigurable);
        }

        return true;
    }

    bool JavascriptLibrary::InitializeStringIteratorPrototype(DynamicObject* stringIteratorPrototype, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(stringIteratorPrototype, mode, 2);
        // Note: Any new function addition/deletion/modification should also be updated in JavascriptLibrary::ProfilerRegisterStringIterator
        // so that the update is in sync with profiler

        JavascriptLibrary* library = stringIteratorPrototype->GetLibrary();
        ScriptContext* scriptContext = library->GetScriptContext();
        library->AddFunctionToLibraryObject(stringIteratorPrototype, PropertyIds::next, &JavascriptStringIterator::EntryInfo::Next, 0);

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(stringIteratorPrototype, PropertyIds::_symbolToStringTag, library->CreateStringFromCppLiteral(_u("String Iterator")), PropertyConfigurable);
        }

        return true;
    }

    RuntimeFunction* JavascriptLibrary::CreateBuiltinConstructor(FunctionInfo * functionInfo, DynamicTypeHandler * typeHandler, DynamicObject* prototype)
    {
        Assert((functionInfo->GetAttributes() & FunctionInfo::Attributes::ErrorOnNew) == 0);

        if (prototype == nullptr)
        {
            prototype = functionPrototype;
        }

        ConstructorCache* ctorCache = ((functionInfo->GetAttributes() & FunctionInfo::Attributes::SkipDefaultNewObject) != 0) ?
            static_cast<ConstructorCache*>(this->builtInConstructorCache) : &ConstructorCache::DefaultInstance;

        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, prototype, functionInfo->GetOriginalEntryPoint(), typeHandler);

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, RuntimeFunction, type, functionInfo, ctorCache);
    }

    JavascriptExternalFunction* JavascriptLibrary::CreateExternalConstructor(Js::ExternalMethod entryPoint, PropertyId nameId, RecyclableObject * prototype)
    {
        Assert(nameId >= Js::InternalPropertyIds::Count && scriptContext->IsTrackedPropertyId(nameId));
        JavascriptExternalFunction* function = this->CreateIdMappedExternalFunction(entryPoint, idMappedFunctionWithPrototypeType);
        function->SetFunctionNameId(TaggedInt::ToVarUnchecked(nameId));

        Js::RecyclableObject* objPrototype;
        if (prototype == nullptr)
        {
            objPrototype = CreateConstructorPrototypeObject(function);
            Assert(!objPrototype->IsEnumerable(Js::PropertyIds::constructor));
        }
        else
        {
            AssertOrFailFast(Js::VarIsCorrectType(prototype));
            objPrototype = prototype;
            Js::JavascriptOperators::InitProperty(objPrototype, Js::PropertyIds::constructor, function);
            objPrototype->SetEnumerable(Js::PropertyIds::constructor, false);
        }

        Assert(!function->IsEnumerable(Js::PropertyIds::prototype));
        Assert(!function->IsConfigurable(Js::PropertyIds::prototype));
        Assert(!function->IsWritable(Js::PropertyIds::prototype));
        function->SetPropertyWithAttributes(Js::PropertyIds::prototype, objPrototype, PropertyNone, nullptr);

        JavascriptString * functionName = nullptr;
        DebugOnly(bool status =) function->GetFunctionName(&functionName);
        AssertMsg(status,"CreateExternalConstructor sets the functionNameId, status should always be true");
        function->SetPropertyWithAttributes(PropertyIds::name, functionName, PropertyConfigurable, nullptr);

        return function;
    }

    JavascriptExternalFunction* JavascriptLibrary::CreateExternalConstructor(Js::ExternalMethod entryPoint, PropertyId nameId, InitializeMethod method, unsigned short deferredTypeSlots, bool hasAccessors)
    {
        Assert(nameId >= Js::InternalPropertyIds::Count && scriptContext->IsTrackedPropertyId(nameId));

        JavascriptExternalFunction* function = nullptr;
        if (entryPoint != nullptr)
        {
             function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptExternalFunction, entryPoint,
                externalConstructorFunctionWithDeferredPrototypeType, method, deferredTypeSlots, hasAccessors);
        }
        else
        {
            function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptExternalFunction,
                defaultExternalConstructorFunctionWithDeferredPrototypeType, method, deferredTypeSlots, hasAccessors);
        }
        function->SetFunctionNameId(TaggedInt::ToVarUnchecked(nameId));

        return function;
    }

    JsrtExternalType* JavascriptLibrary::GetCachedJsrtExternalType(
#ifdef _CHAKRACOREBUILD
        uintptr_t traceCallback,
#endif
        uintptr_t finalizeCallback,
        uintptr_t prototype)
    {
        RecyclerWeakReference<DynamicType>* dynamicTypeWeakRef = nullptr;
        DynamicType* dynamicType = nullptr;
        if (jsrtExternalTypesCache == nullptr)
        {
            jsrtExternalTypesCache = RecyclerNew(recycler, JsrtExternalTypesCache, recycler, 3);
            // Register for periodic cleanup
            scriptContext->RegisterWeakReferenceDictionary(jsrtExternalTypesCache);
        }
        if (jsrtExternalTypesCache->TryGetValue(JsrtExternalCallbacks(
#ifdef _CHAKRACOREBUILD
            traceCallback,
#endif
            finalizeCallback,
            prototype), &dynamicTypeWeakRef))
        {
            dynamicType = dynamicTypeWeakRef->Get();
        }
        return (JsrtExternalType*)dynamicType;
    }

#ifdef _CHAKRACOREBUILD
    void JavascriptLibrary::CacheJsrtExternalType(uintptr_t traceCallback, uintptr_t finalizeCallback, uintptr_t prototype, JsrtExternalType* dynamicTypeToCache)
    {
        jsrtExternalTypesCache->Item(JsrtExternalCallbacks(traceCallback, finalizeCallback, prototype), recycler->CreateWeakReferenceHandle<DynamicType>((DynamicType*)dynamicTypeToCache));
    }
#else
    void JavascriptLibrary::CacheJsrtExternalType(uintptr_t finalizeCallback, uintptr_t prototype, JsrtExternalType* dynamicTypeToCache)
    {
        jsrtExternalTypesCache->Item(JsrtExternalCallbacks(finalizeCallback, prototype), recycler->CreateWeakReferenceHandle<DynamicType>((DynamicType*)dynamicTypeToCache));
    }
#endif

#ifdef _CHAKRACOREBUILD
    DynamicType* JavascriptLibrary::GetCachedCustomExternalWrapperType(uintptr_t traceCallback, uintptr_t finalizeCallback, uintptr_t interceptors, uintptr_t prototype)
    {
        RecyclerWeakReference<DynamicType>* dynamicTypeWeakRef = nullptr;
        DynamicType* dynamicType = nullptr;
        if (customExternalWrapperTypesCache == nullptr)
        {
            customExternalWrapperTypesCache = RecyclerNew(recycler, CustomExternalWrapperTypesCache, recycler, 3);
            // Register for periodic cleanup
            scriptContext->RegisterWeakReferenceDictionary(customExternalWrapperTypesCache);
        }
        if (customExternalWrapperTypesCache->TryGetValue(CustomExternalWrapperCallbacks(traceCallback, finalizeCallback, interceptors, prototype), &dynamicTypeWeakRef))
        {
            dynamicType = dynamicTypeWeakRef->Get();
        }
        return dynamicType;
    }

    void JavascriptLibrary::CacheCustomExternalWrapperType(uintptr_t traceCallback, uintptr_t finalizeCallback, uintptr_t interceptors, uintptr_t prototype, DynamicType* dynamicTypeToCache)
    {
        customExternalWrapperTypesCache->Item(CustomExternalWrapperCallbacks(traceCallback, finalizeCallback, interceptors, prototype), recycler->CreateWeakReferenceHandle<DynamicType>(dynamicTypeToCache));
    }
#endif

    void JavascriptLibrary::DefaultCreateFunction(ParseableFunctionInfo * functionInfo, int length, DynamicObject * prototype, PropertyId nameId)
    {
        Assert(nameId >= Js::InternalPropertyIds::Count && scriptContext->IsTrackedPropertyId(nameId));
        ScriptFunction* function = scriptContext->GetLibrary()->CreateScriptFunction(functionInfo);
        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(length), PropertyConfigurable, nullptr);
        AddMember(prototype, nameId, function);
    }

    RuntimeFunction* JavascriptLibrary::DefaultCreateFunction(FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType, PropertyId nameId)
    {
        Assert(nameId >= Js::InternalPropertyIds::Count && scriptContext->IsTrackedPropertyId(nameId));
        return DefaultCreateFunction(functionInfo, length, prototype, functionType, TaggedInt::ToVarUnchecked((int)nameId));
    }

    RuntimeFunction* JavascriptLibrary::DefaultCreateFunction(FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType, Var nameId)
    {
        Assert(nameId != nullptr);
        RuntimeFunction * function;

        if (nullptr == functionType)
        {
            functionType = (nullptr == prototype) ?
                                CreateFunctionWithLengthAndNameType(functionInfo) :
                                CreateFunctionWithLengthAndPrototypeType(functionInfo);
        }

        function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, RuntimeFunction, functionType, functionInfo);

        if (prototype != nullptr)
        {
            // NOTE: Assume all built in function prototype doesn't contain valueOf or toString that has side effects
            function->SetPropertyWithAttributes(PropertyIds::prototype, prototype, PropertyNone, nullptr, PropertyOperation_None, SideEffects_None);
        }

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(length), PropertyConfigurable, nullptr);
        function->SetFunctionNameId(nameId);

        JavascriptString * functionName = nullptr;
        DebugOnly(bool status = ) function->GetFunctionName(&functionName);
        AssertMsg(status, "DefaultCreateFunction sets the functionNameId, status should always be true");
        function->SetPropertyWithAttributes(PropertyIds::name, functionName, PropertyConfigurable, nullptr);

#ifdef HEAP_ENUMERATION_VALIDATION
        if (prototype) prototype->SetHeapEnumValidationCookie(HEAP_ENUMERATION_LIBRARY_OBJECT_COOKIE);
        function->SetHeapEnumValidationCookie(HEAP_ENUMERATION_LIBRARY_OBJECT_COOKIE);
#endif
        return function;
    }

    JavascriptFunction* JavascriptLibrary::AddFunction(DynamicObject* object, PropertyId propertyId, RuntimeFunction* function)
    {
       AddMember(object, propertyId, function);
       function->SetFunctionNameId(TaggedInt::ToVarUnchecked((int)propertyId));
       return function;
    }

    JavascriptFunction * JavascriptLibrary::AddFunctionToLibraryObject(DynamicObject* object, PropertyId propertyId, FunctionInfo * functionInfo, int length, PropertyAttributes attributes)
    {
        RuntimeFunction* function = DefaultCreateFunction(functionInfo, length, nullptr, nullptr, propertyId);
        AddMember(object, propertyId, function, attributes);
        return function;
    }

    JavascriptFunction * JavascriptLibrary::AddFunctionToLibraryObjectWithPrototype(DynamicObject * object, PropertyId propertyId, FunctionInfo * functionInfo, int length, DynamicObject * prototype, DynamicType * functionType)
    {
        RuntimeFunction* function = DefaultCreateFunction(functionInfo, length, prototype, functionType, propertyId);
        AddMember(object, propertyId, function);
        return function;
    }

    JavascriptFunction * JavascriptLibrary::AddFunctionToLibraryObjectWithName(DynamicObject* object, PropertyId propertyId, PropertyId name, FunctionInfo * functionInfo, int length)
    {
        RuntimeFunction* function = DefaultCreateFunction(functionInfo, length, nullptr, nullptr, name);
        AddMember(object, propertyId, function);
        return function;
    }

    RuntimeFunction* JavascriptLibrary::AddGetterToLibraryObject(DynamicObject* object, PropertyId propertyId, FunctionInfo* functionInfo)
    {
        RuntimeFunction* getter = CreateGetterFunction(propertyId, functionInfo);
        AddAccessorsToLibraryObject(object, propertyId, getter, nullptr);
        return getter;
    }

    void JavascriptLibrary::AddAccessorsToLibraryObject(DynamicObject* object, PropertyId propertyId, FunctionInfo * getterFunctionInfo, FunctionInfo * setterFunctionInfo)
    {
        AddAccessorsToLibraryObjectWithName(object, propertyId, propertyId, getterFunctionInfo, setterFunctionInfo);
    }

    void JavascriptLibrary::AddAccessorsToLibraryObject(DynamicObject* object, PropertyId propertyId, RecyclableObject* getterFunction, RecyclableObject* setterFunction)
    {
        if (getterFunction == nullptr)
        {
            getterFunction = GetUndefined();
        }

        if (setterFunction == nullptr)
        {
            setterFunction = GetUndefined();
        }

        object->SetAccessors(propertyId, getterFunction, setterFunction);
        object->SetAttributes(propertyId, PropertyConfigurable | PropertyWritable);
    }

    void JavascriptLibrary::AddAccessorsToLibraryObjectWithName(DynamicObject* object, PropertyId propertyId, PropertyId nameId, FunctionInfo * getterFunctionInfo, FunctionInfo * setterFunctionInfo)
    {
        Js::RuntimeFunction* getterFunction = (getterFunctionInfo != nullptr)
            ? CreateGetterFunction(nameId, getterFunctionInfo)
            : nullptr;
        Js::RuntimeFunction* setterFunction = (setterFunctionInfo != nullptr)
            ? CreateSetterFunction(nameId, setterFunctionInfo)
            : nullptr;
        AddAccessorsToLibraryObject(object, propertyId, getterFunction, setterFunction);
    }

    void JavascriptLibrary::AddSpeciesAccessorsToLibraryObject(DynamicObject* object, FunctionInfo * getterFunctionInfo)
    {
        AddAccessorsToLibraryObjectWithName(object, PropertyIds::_symbolSpecies, PropertyIds::_RuntimeFunctionNameId_species, getterFunctionInfo, nullptr);
    }

    RuntimeFunction* JavascriptLibrary::CreateGetterFunction(PropertyId nameId, FunctionInfo* functionInfo)
    {
        Var name_withGetPrefix = JavascriptString::Concat(GetGetterFunctionPrefixString(), scriptContext->GetPropertyString(nameId));
        RuntimeFunction* getterFunction = DefaultCreateFunction(functionInfo, 0, nullptr, nullptr, name_withGetPrefix);
        getterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);
        return getterFunction;
    }

    RuntimeFunction* JavascriptLibrary::CreateSetterFunction(PropertyId nameId, FunctionInfo* functionInfo)
    {
        Var name_withSetPrefix = JavascriptString::Concat(GetSetterFunctionPrefixString(), scriptContext->GetPropertyString(nameId));
        RuntimeFunction* setterFunction = DefaultCreateFunction(functionInfo, 0, nullptr, nullptr, name_withSetPrefix);
        setterFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);
        return setterFunction;
    }

    void JavascriptLibrary::AddMember(DynamicObject* object, PropertyId propertyId, Var value)
    {
        AddMember(object, propertyId, value, PropertyBuiltInMethodDefaults);
    }

    void JavascriptLibrary::AddMember(DynamicObject* object, PropertyId propertyId, Var value, PropertyAttributes attributes)
    {
        // NOTE: Assume all built in member doesn't have side effect
        object->SetPropertyWithAttributes(propertyId, value, attributes, nullptr, PropertyOperation_None, SideEffects_None);
    }

    bool JavascriptLibrary::InitializeJSONObject(DynamicObject* JSONObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(JSONObject, mode, 3);
        JavascriptLibrary* library = JSONObject->GetLibrary();
        library->AddMember(JSONObject, PropertyIds::stringify, library->EnsureJSONStringifyFunction());
        JSONObject->GetScriptContext()->SetBuiltInLibraryFunction(JSON::EntryInfo::Stringify.GetOriginalEntryPoint(), library->EnsureJSONStringifyFunction());
        library->AddFunctionToLibraryObject(JSONObject, PropertyIds::parse, &JSON::EntryInfo::Parse, 2);

        if (JSONObject->GetScriptContext()->GetConfig()->IsES6ToStringTagEnabled())
        {
            library->AddMember(JSONObject, PropertyIds::_symbolToStringTag, JSONObject->GetLibrary()->CreateStringFromCppLiteral(_u("JSON")), PropertyConfigurable);
        }

        JSONObject->SetHasNoEnumerableProperties(true);

        return true;
    }

    JavascriptFunction* JavascriptLibrary::EnsureJSONStringifyFunction()
    {
        if (jsonStringifyFunction == nullptr)
        {
            jsonStringifyFunction = DefaultCreateFunction(&JSON::EntryInfo::Stringify, 3, nullptr, nullptr, PropertyIds::stringify);
        }
        return jsonStringifyFunction;
    }

#if defined(ENABLE_INTL_OBJECT) || defined(ENABLE_JS_BUILTINS)
    bool JavascriptLibrary::InitializeEngineInterfaceObject(DynamicObject* engineInterface, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(engineInterface, mode, 3);

        VarTo<EngineInterfaceObject>(engineInterface)->Initialize();

        engineInterface->SetHasNoEnumerableProperties(true);

        return true;
    }
#endif

    void JavascriptLibrary::SetNativeHostPromiseContinuationFunction(PromiseContinuationCallback function, void *state)
    {
        this->nativeHostPromiseContinuationFunction = function;
        this->nativeHostPromiseContinuationFunctionState = state;
    }

    void JavascriptLibrary::CallNativeHostPromiseRejectionTracker(Var promise, Var reason, bool handled)
    {
        if (this->nativeHostPromiseRejectionTracker != nullptr)
        {
            BEGIN_LEAVE_SCRIPT(scriptContext);
            try
            {
                this->nativeHostPromiseRejectionTracker(promise, reason, handled, this->nativeHostPromiseRejectionTrackerState);
            }
            catch (...)
            {
                // Hosts are required not to pass exceptions back across the callback boundary. If
                // this happens, it is a bug in the host, not something that we are expected to
                // handle gracefully.
                Js::Throw::FatalInternalError();
            }
            END_LEAVE_SCRIPT(scriptContext);
        }
    }

    void JavascriptLibrary::SetJsrtContext(FinalizableObject* jsrtContext)
    {
        // With JsrtContext supporting cross context, ensure that it doesn't get GCed
        // prematurely. So pin the instance to javascriptLibrary so it will stay alive
        // until any object of it are alive.
        Assert(this->jsrtContextObject == nullptr);
        this->jsrtContextObject = jsrtContext;
    }

    FinalizableObject* JavascriptLibrary::GetJsrtContext()
    {
        return this->jsrtContextObject;
    }

    void JavascriptLibrary::EnqueueTask(Var taskVar)
    {
        Assert(VarIs<JavascriptFunction>(taskVar));

        if(this->nativeHostPromiseContinuationFunction)
        {
#if ENABLE_TTD
            TTDAssert(this->scriptContext != nullptr, "We shouldn't be adding tasks if this is the case???");

            if(this->scriptContext->ShouldPerformReplayAction())
            {
                this->scriptContext->GetThreadContext()->TTDRootNestingCount++;

                this->scriptContext->GetThreadContext()->TTDLog->ReplayEnqueueTaskEvent(scriptContext, taskVar);

                this->scriptContext->GetThreadContext()->TTDRootNestingCount--;
            }
            else if(this->scriptContext->ShouldPerformRecordAction())
            {
                this->scriptContext->GetThreadContext()->TTDRootNestingCount++;
                TTD::NSLogEvents::EventLogEntry* evt = this->scriptContext->GetThreadContext()->TTDLog->RecordEnqueueTaskEvent(taskVar);

                BEGIN_LEAVE_SCRIPT(this->scriptContext);
                try
                {
                    this->nativeHostPromiseContinuationFunction(taskVar, this->nativeHostPromiseContinuationFunctionState);
                }
                catch(...)
                {
                    // Hosts are required not to pass exceptions back across the callback boundary. If
                    // this happens, it is a bug in the host, not something that we are expected to
                    // handle gracefully.
                    Js::Throw::FatalInternalError();
                }
                END_LEAVE_SCRIPT(this->scriptContext);

                this->scriptContext->GetThreadContext()->TTDLog->RecordEnqueueTaskEvent_Complete(evt);
                this->scriptContext->GetThreadContext()->TTDRootNestingCount--;
            }
            else
            {
                BEGIN_LEAVE_SCRIPT(scriptContext);
                try
                {
                    this->nativeHostPromiseContinuationFunction(taskVar, this->nativeHostPromiseContinuationFunctionState);
                }
                catch(...)
                {
                    // Hosts are required not to pass exceptions back across the callback boundary. If
                    // this happens, it is a bug in the host, not something that we are expected to
                    // handle gracefully.
                    Js::Throw::FatalInternalError();
                }
                END_LEAVE_SCRIPT(scriptContext);
            }
#else
            BEGIN_LEAVE_SCRIPT(scriptContext);
            try
            {
                this->nativeHostPromiseContinuationFunction(taskVar, this->nativeHostPromiseContinuationFunctionState);
            }
            catch (...)
            {
                // Hosts are required not to pass exceptions back across the callback boundary. If
                // this happens, it is a bug in the host, not something that we are expected to
                // handle gracefully.
                Js::Throw::FatalInternalError();
            }
            END_LEAVE_SCRIPT(scriptContext);
#endif
        }
        else
        {
#if ENABLE_TTD
            if(this->scriptContext->ShouldPerformRecordOrReplayAction())
            {
                //
                //TODO: need to implement support for this path
                //
                TTDAssert(false, "Path not implemented in TTD!!!");
            }
#endif

            HRESULT hr = scriptContext->GetHostScriptContext()->EnqueuePromiseTask(taskVar);
            if (hr != S_OK)
            {
                Js::JavascriptError::MapAndThrowError(scriptContext, JSERR_HostMaybeMissingPromiseContinuationCallback);
            }
        }
    }

#ifdef ENABLE_JS_BUILTINS

    bool JavascriptLibrary::InitializeChakraLibraryObject(DynamicObject * chakraLibraryObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        JavascriptLibrary* library = chakraLibraryObject->GetLibrary();
        typeHandler->Convert(chakraLibraryObject, mode, 8);

        Field(JavascriptFunction*)* builtinFuncs = library->GetBuiltinFunctions();
        JavascriptFunction * func = nullptr;

        library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::toLength, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ToLengthFunction, 1);
        library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::toInteger, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ToIntegerFunction, 1);
        library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::GetLength, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_GetLength, 1);
        library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::InitInternalProperties, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_InitInternalProperties, 1);
        library->AddMember(chakraLibraryObject, PropertyIds::isArray, library->isArrayFunction);
        library->AddMember(chakraLibraryObject, PropertyIds::Object, library->objectConstructor);
        library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::arraySpeciesCreate, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ArraySpeciesCreate, 2);
        library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::arrayCreateDataPropertyOrThrow, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ArrayCreateDataPropertyOrThrow, 3);
        func = library->AddFunctionToLibraryObject(chakraLibraryObject, PropertyIds::builtInCallInstanceFunction, &EngineInterfaceObject::EntryInfo::CallInstanceFunction, 1);
        builtinFuncs[BuiltinFunction::EngineInterfaceObject_CallInstanceFunction] = func;

        return true;
    }

#endif // ENABLE_JS_BUILTINS

#ifdef ENABLE_INTL_OBJECT
    void JavascriptLibrary::ResetIntlObject()
    {
        IntlObject = DynamicObject::New(
            recycler,
            DynamicType::New(scriptContext,
                             TypeIds_Object, objectPrototype, nullptr,
                             DeferredTypeHandler<InitializeIntlObject>::GetDefaultInstance()));
    }

    void JavascriptLibrary::EnsureIntlObjectReady()
    {
        Assert(this->IntlObject != nullptr);

        // For Intl builtins, we need to make sure the Intl object has been initialized before fetching the
        // builtins from the EngineInterfaceObject. This is because the builtins are actually created via
        // Intl.js and are registered into the EngineInterfaceObject as part of Intl object initialization.
        JavascriptExceptionObject * caughtExceptionObject = nullptr;
        try
        {
            this->IntlObject->GetTypeHandler()->EnsureObjectReady(this->IntlObject);
        }
        catch (const JavascriptException& err)
        {
            caughtExceptionObject = err.GetAndClear();
        }

        // Propagate the OOM and SOE exceptions only
        if (caughtExceptionObject != nullptr &&
            (caughtExceptionObject == ThreadContext::GetContextForCurrentThread()->GetPendingOOMErrorObject() ||
            caughtExceptionObject == ThreadContext::GetContextForCurrentThread()->GetPendingSOErrorObject()))
        {
            caughtExceptionObject = caughtExceptionObject->CloneIfStaticExceptionObject(scriptContext);
            JavascriptExceptionOperators::DoThrow(caughtExceptionObject, scriptContext);
        }
    }

    bool JavascriptLibrary::InitializeIntlObject(DynamicObject* IntlObject, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(IntlObject, mode,  /*initSlotCapacity*/ 2);

        auto intlInitializer = [&](IntlEngineInterfaceExtensionObject* intlExtension, ScriptContext * scriptContext, DynamicObject* intlObject) ->void
        {
            intlExtension->InjectIntlLibraryCode(scriptContext, intlObject, IntlInitializationType::Intl);
        };
        IntlObject->GetLibrary()->InitializeIntlForPrototypes(intlInitializer);

        return true;
    }

    void JavascriptLibrary::InitializeIntlForStringPrototype()
    {
        auto stringPrototypeInitializer = [&](IntlEngineInterfaceExtensionObject* intlExtension, ScriptContext * scriptContext, DynamicObject* intlObject) ->void
        {
            intlExtension->InjectIntlLibraryCode(scriptContext, intlObject, IntlInitializationType::StringPrototype);
        };
        InitializeIntlForPrototypes(stringPrototypeInitializer);
    }

    void JavascriptLibrary::InitializeIntlForDatePrototype()
    {
        auto datePrototypeInitializer = [&](IntlEngineInterfaceExtensionObject* intlExtension, ScriptContext * scriptContext, DynamicObject* intlObject) ->void
        {
            intlExtension->InjectIntlLibraryCode(scriptContext, intlObject, IntlInitializationType::DatePrototype);
        };
        InitializeIntlForPrototypes(datePrototypeInitializer);
    }

    void JavascriptLibrary::InitializeIntlForNumberPrototype()
    {
        auto numberPrototypeInitializer = [&](IntlEngineInterfaceExtensionObject* intlExtension, ScriptContext * scriptContext, DynamicObject* intlObject) ->void
        {
            intlExtension->InjectIntlLibraryCode(scriptContext, intlObject, IntlInitializationType::NumberPrototype);
        };
        InitializeIntlForPrototypes(numberPrototypeInitializer);
    }

    template <class Fn>
    void JavascriptLibrary::InitializeIntlForPrototypes(Fn fn)
    {
        ScriptContext* scriptContext = this->IntlObject->GetScriptContext();
        if (scriptContext->VerifyAlive())  // Can't initialize if scriptContext closed, will need to run script
        {
            JavascriptLibrary* library = this->IntlObject->GetLibrary();
            Assert(library->engineInterfaceObject != nullptr);
            IntlEngineInterfaceExtensionObject* intlExtension = static_cast<IntlEngineInterfaceExtensionObject*>(library->GetEngineInterfaceObject()->GetEngineExtension(EngineInterfaceExtensionKind_Intl));
            fn(intlExtension, scriptContext, IntlObject);
        }
    }
#endif

    void JavascriptLibrary::SetProfileMode(bool fSet)
    {
        inProfileMode = fSet;
    }

    void JavascriptLibrary::SetDispatchProfile(bool fSet, JavascriptMethod dispatchInvoke)
    {
        if (!fSet)
        {
            this->inDispatchProfileMode = false;
            if (dispatchInvoke != nullptr)
            {
                this->GetScriptContext()->GetHostScriptContext()->SetDispatchInvoke(dispatchInvoke);
            }
            idMappedFunctionWithPrototypeType->SetEntryPoint(JavascriptExternalFunction::ExternalFunctionThunk);
            externalFunctionWithDeferredPrototypeType->SetEntryPoint(JavascriptExternalFunction::ExternalFunctionThunk);
            stdCallFunctionWithDeferredPrototypeType->SetEntryPoint(JavascriptExternalFunction::StdCallExternalFunctionThunk);
        }
        else
        {
            this->inDispatchProfileMode = true;
            if (dispatchInvoke != nullptr)
            {
                this->GetScriptContext()->GetHostScriptContext()->SetDispatchInvoke(dispatchInvoke);
            }
            idMappedFunctionWithPrototypeType->SetEntryPoint(ProfileEntryThunk);
            externalFunctionWithDeferredPrototypeType->SetEntryPoint(ProfileEntryThunk);
            stdCallFunctionWithDeferredPrototypeType->SetEntryPoint(ProfileEntryThunk);
        }
    }
    JavascriptString* JavascriptLibrary::CreateEmptyString()
    {
        return LiteralStringWithPropertyStringPtr::CreateEmptyString(this);
    }

    JavascriptRegExp* JavascriptLibrary::CreateEmptyRegExp()
    {
        return RecyclerNew(scriptContext->GetRecycler(), JavascriptRegExp, emptyRegexPattern,
                           this->GetRegexType());
    }

#if ENABLE_TTD
    Js::PropertyId JavascriptLibrary::ExtractPrimitveSymbolId_TTD(Var value)
    {
        return Js::VarTo<Js::JavascriptSymbol>(value)->GetValue()->GetPropertyId();
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePrimitveSymbol_TTD(Js::PropertyId pid)
    {
        return this->scriptContext->GetSymbol(pid);
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePrimitveSymbol_TTD(Js::JavascriptString* str)
    {
        return this->CreateSymbol(str);
    }

    Js::RecyclableObject* JavascriptLibrary::CreateDefaultBoxedObject_TTD(Js::TypeId kind)
    {
        switch(kind)
        {
        case Js::TypeIds_BooleanObject:
            return this->CreateBooleanObject();
        case Js::TypeIds_NumberObject:
            return this->CreateNumberObject(Js::TaggedInt::ToVarUnchecked(0));
        case Js::TypeIds_StringObject:
            return this->CreateStringObject(nullptr);
        case Js::TypeIds_SymbolObject:
            return this->CreateSymbolObject(nullptr);
        default:
            TTDAssert(false, "Unsupported nullptr value boxed object.");
            return nullptr;
        }
    }

    void JavascriptLibrary::SetBoxedObjectValue_TTD(Js::RecyclableObject* obj, Js::Var value)
    {
        switch(obj->GetTypeId())
        {
        case Js::TypeIds_BooleanObject:
            Js::VarTo<Js::JavascriptBooleanObject>(obj)->SetValue_TTD(value);
            break;
        case Js::TypeIds_NumberObject:
            Js::VarTo<Js::JavascriptNumberObject>(obj)->SetValue_TTD(value);
            break;
        case Js::TypeIds_StringObject:
            Js::VarTo<Js::JavascriptStringObject>(obj)->SetValue_TTD(value);
            break;
        case Js::TypeIds_SymbolObject:
            Js::VarTo<Js::JavascriptSymbolObject>(obj)->SetValue_TTD(value);
            break;
        default:
            TTDAssert(false, "Unsupported nullptr value boxed object.");
            break;
        }
    }

    Js::RecyclableObject* JavascriptLibrary::CreateDate_TTD(double value)
    {
        return this->CreateDate(value);
    }

    Js::RecyclableObject* JavascriptLibrary::CreateRegex_TTD(const char16* patternSource, uint32 patternLength, UnifiedRegex::RegexFlags flags, CharCount lastIndex, Js::Var lastVar)
    {
        Js::JavascriptRegExp* re = Js::JavascriptRegExp::CreateRegEx(patternSource, patternLength, flags, this->scriptContext);
        re->SetLastIndexInfo_TTD(lastIndex, lastVar);

        return re;
    }

    Js::RecyclableObject* JavascriptLibrary::CreateError_TTD()
    {
        return this->CreateError();
    }

    Js::RecyclableObject* JavascriptLibrary::CreateES5Array_TTD()
    {
        Js::JavascriptArray* arrayObj = this->CreateArray();
        arrayObj->GetTypeHandler()->ConvertToTypeWithItemAttributes(arrayObj);

        return arrayObj;
    }

    void JavascriptLibrary::SetLengthWritableES5Array_TTD(Js::RecyclableObject* es5Array, bool isLengthWritable)
    {
        Js::ES5Array* es5a = Js::VarTo<Js::ES5Array>(es5Array);
        if(es5a->IsLengthWritable() != isLengthWritable)
        {
            es5a->SetWritable(Js::PropertyIds::length, isLengthWritable ? TRUE : FALSE);
        }
    }

    Js::RecyclableObject* JavascriptLibrary::CreateSet_TTD()
    {
        return JavascriptSet::CreateForSnapshotRestore(this->scriptContext);
    }

    Js::RecyclableObject* JavascriptLibrary::CreateWeakSet_TTD()
    {
        return this->CreateWeakSet();
    }

    void JavascriptLibrary::AddSetElementInflate_TTD(Js::JavascriptSet* set, Var value)
    {
        set->Add(value);
    }

    void JavascriptLibrary::AddWeakSetElementInflate_TTD(Js::JavascriptWeakSet* set, Var value)
    {
        set->GetScriptContext()->TTDContextInfo->TTDWeakReferencePinSet->Add(Js::VarTo<Js::DynamicObject>(value));

        set->Add(Js::VarTo<Js::DynamicObject>(value));
    }

    Js::RecyclableObject* JavascriptLibrary::CreateMap_TTD()
    {
        return JavascriptMap::CreateForSnapshotRestore(this->scriptContext);
    }

    Js::RecyclableObject* JavascriptLibrary::CreateWeakMap_TTD()
    {
        return this->CreateWeakMap();
    }

    void JavascriptLibrary::AddMapElementInflate_TTD(Js::JavascriptMap* map, Var key, Var value)
    {
        map->Set(key, value);
    }

    void JavascriptLibrary::AddWeakMapElementInflate_TTD(Js::JavascriptWeakMap* map, Var key, Var value)
    {
        map->GetScriptContext()->TTDContextInfo->TTDWeakReferencePinSet->Add(Js::VarTo<Js::DynamicObject>(key));

        map->Set(Js::VarTo<Js::DynamicObject>(key), value);
    }

    Js::RecyclableObject* JavascriptLibrary::CreateExternalFunction_TTD(Js::Var fname)
    {
        if(TaggedInt::Is(fname))
        {
            PropertyId pid = TaggedInt::ToInt32(fname);
            if(!scriptContext->IsTrackedPropertyId(pid))
            {
                scriptContext->TrackPid(pid);
            }
        }

        JavascriptExternalFunction* function = this->CreateIdMappedExternalFunction(&JavascriptExternalFunction::TTDReplayDummyExternalMethod, stdCallFunctionWithDeferredPrototypeType);
        function->SetFunctionNameId(fname);
        function->SetCallbackState(nullptr);
        return function;
    }

    Js::RecyclableObject* JavascriptLibrary::CreateBoundFunction_TTD(
        RecyclableObject* function, Var bThis, uint32 ct, Field(Var)* args)
    {
        return BoundFunction::InflateBoundFunction(this->scriptContext, function, bThis, ct, args);
    }

    Js::RecyclableObject* JavascriptLibrary::CreateProxy_TTD(RecyclableObject* handler, RecyclableObject* target)
    {
        JavascriptProxy* newProxy = RecyclerNew(this->scriptContext->GetRecycler(), JavascriptProxy, this->GetProxyType(), this->scriptContext, target, handler);

        if(target != nullptr && JavascriptConversion::IsCallable(target))
        {
            newProxy->ChangeType();
            newProxy->GetDynamicType()->SetEntryPoint(JavascriptProxy::FunctionCallTrap);
        }

        return newProxy;
    }

    Js::RecyclableObject* JavascriptLibrary::CreateRevokeFunction_TTD(RecyclableObject* proxy)
    {
        RuntimeFunction* revoker = RecyclerNewEnumClass(this->scriptContext->GetRecycler(), JavascriptLibrary::EnumFunctionClass, RuntimeFunction, this->CreateFunctionWithLengthType(&JavascriptProxy::EntryInfo::Revoke), &JavascriptProxy::EntryInfo::Revoke);

        revoker->SetPropertyWithAttributes(Js::PropertyIds::length, Js::TaggedInt::ToVarUnchecked(0), PropertyNone, NULL);
        revoker->SetInternalProperty(Js::InternalPropertyIds::RevocableProxy, proxy, PropertyOperationFlags::PropertyOperation_Force, nullptr);

        return revoker;
    }

    Js::RecyclableObject* JavascriptLibrary::CreateHeapArguments_TTD(uint32 numOfArguments, uint32 formalCount, ActivationObject* frameObject, byte* deletedArray)
    {
        Js::HeapArgumentsObject* argsObj = this->CreateHeapArguments(frameObject, formalCount);

        argsObj->SetNumberOfArguments(numOfArguments);
        for(uint32 i = 0; i < formalCount; ++i)
        {
            if(deletedArray[i])
            {
                argsObj->DeleteItemAt(i);
            }
        }

        return argsObj;
    }

    Js::RecyclableObject* JavascriptLibrary::CreateES5HeapArguments_TTD(uint32 numOfArguments, uint32 formalCount, ActivationObject* frameObject, byte* deletedArray)
    {
        Js::HeapArgumentsObject* argsObj = this->CreateHeapArguments(frameObject, formalCount);

        argsObj->SetNumberOfArguments(numOfArguments);
        for(uint32 i = 0; i < formalCount; ++i)
        {
            if(deletedArray[i])
            {
                argsObj->DeleteItemAt(i);
            }
        }

        return argsObj->ConvertToES5HeapArgumentsObject_TTD();
    }

    Js::JavascriptPromiseCapability* JavascriptLibrary::CreatePromiseCapability_TTD(Var promise, Var resolve, Var reject)
    {
        return JavascriptPromiseCapability::New(promise, resolve, reject, this->scriptContext);
    }

    Js::JavascriptPromiseReaction* JavascriptLibrary::CreatePromiseReaction_TTD(RecyclableObject* handler, JavascriptPromiseCapability* capabilities)
    {
        return JavascriptPromiseReaction::New(capabilities, handler, this->scriptContext);
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePromise_TTD(uint32 status, bool isHandled, Var result, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& resolveReactions, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& rejectReactions)
    {
        return JavascriptPromise::InitializePromise_TTD(this->scriptContext, status, isHandled, result, resolveReactions, rejectReactions);
    }

    JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* JavascriptLibrary::CreateAlreadyDefinedWrapper_TTD(bool alreadyDefined)
    {
        JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper);
        alreadyResolvedRecord->alreadyResolved = alreadyDefined;

        return alreadyResolvedRecord;
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePromiseResolveOrRejectFunction_TTD(RecyclableObject* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolved)
    {
        TTDAssert(VarIs<JavascriptPromise>(promise), "Not a promise!");

        return this->CreatePromiseResolveOrRejectFunction(JavascriptPromise::EntryResolveOrRejectFunction, static_cast<JavascriptPromise*>(promise), isReject, alreadyResolved);
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePromiseReactionTaskFunction_TTD(JavascriptPromiseReaction* reaction, Var argument)
    {
        return this->CreatePromiseReactionTaskFunction(JavascriptPromise::EntryReactionTaskFunction, reaction, argument);
    }

    JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* JavascriptLibrary::CreateRemainingElementsWrapper_TTD(Js::ScriptContext* ctx, uint32 value)
    {
        JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper = RecyclerNewStructZ(ctx->GetRecycler(), JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper);
        remainingElementsWrapper->remainingElements = value;

        return remainingElementsWrapper;
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePromiseAllResolveElementFunction_TTD(Js::JavascriptPromiseCapability* capabilities, uint32 index, Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* wrapper, Js::RecyclableObject* values, bool alreadyCalled)
    {
        Js::JavascriptPromiseAllResolveElementFunction* res = this->CreatePromiseAllResolveElementFunction(JavascriptPromise::EntryAllResolveElementFunction, index, Js::VarTo<Js::JavascriptArray>(values), capabilities, wrapper);
        res->SetAlreadyCalled(alreadyCalled);

        return res;
    }

    JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* JavascriptLibrary::CreateAlreadyCalledWrapper_TTD(Js::ScriptContext* ctx, bool value)
    {
        JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper = RecyclerNewStructZ(ctx->GetRecycler(), JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper);
        alreadyCalledWrapper->alreadyResolved = value;

        return alreadyCalledWrapper;
    }

    Js::RecyclableObject* JavascriptLibrary::CreatePromiseAllSettledResolveOrRejectElementFunction_TTD(Js::JavascriptPromiseCapability* capabilities, uint32 index, Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* wrapper, Js::RecyclableObject* values, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper, bool isRejecting)
    {
        Js::JavascriptPromiseAllSettledResolveOrRejectElementFunction* res = this->CreatePromiseAllSettledResolveOrRejectElementFunction(JavascriptPromise::EntryAllSettledResolveOrRejectElementFunction, index, Js::VarTo<Js::JavascriptArray>(values), capabilities, wrapper, alreadyCalledWrapper, isRejecting);

        return res;
    }

    Js::RecyclableObject* JavascriptLibrary::CreateJavascriptGenerator_TTD(Js::ScriptContext *ctx,
        Js::RecyclableObject *prototype, Js::Arguments &arguments,
        Js::JavascriptGenerator::GeneratorState generatorState)
    {
        Js::DynamicType* generatorType = CreateGeneratorType(prototype);
        Js::JavascriptGenerator* generator = Js::JavascriptGenerator::New(ctx->GetRecycler(), generatorType, arguments, generatorState);
        return generator;
    }

#endif // ENABLE_TTD

    void JavascriptLibrary::SetCrossSiteForLockedFunctionType(JavascriptFunction * function)
    {
        Assert(function->GetDynamicType()->GetIsLocked());

        if (VarIs<ScriptFunction>(function) || VarIs<BoundFunction>(function))
        {
            this->SetCrossSiteForLockedNonBuiltInFunctionType(function);
        }
        else
        {
            DynamicTypeHandler * typeHandler = function->GetDynamicType()->GetTypeHandler();
            if (typeHandler == JavascriptLibrary::GetDeferredPrototypeFunctionTypeHandler(this->GetScriptContext())
                || typeHandler == JavascriptLibrary::GetDeferredPrototypeFunctionWithLengthTypeHandler(this->GetScriptContext()))
            {
                function->ReplaceType(crossSiteDeferredPrototypeFunctionType);
            }
            else if (typeHandler == JavascriptLibrary::GetDeferredFunctionTypeHandler()
                || typeHandler == JavascriptLibrary::GetDeferredFunctionWithLengthTypeHandler())
            {
                function->ReplaceType(crossSiteDeferredFunctionType);
            }
            else if (typeHandler == Js::DeferredTypeHandler<Js::JavascriptExternalFunction::DeferredConstructorInitializer>::GetDefaultInstance())
            {
                function->ReplaceType(crossSiteExternalConstructFunctionWithPrototypeType);
            }
            else if (typeHandler == &SharedIdMappedFunctionWithPrototypeTypeHandler)
            {
                function->ReplaceType(crossSiteIdMappedFunctionWithPrototypeType);
            }
            else
            {
                this->SetCrossSiteForLockedNonBuiltInFunctionType(function);
            }
        }
    }

    void JavascriptLibrary::SetCrossSiteForLockedNonBuiltInFunctionType(JavascriptFunction * function)
    {
        FunctionProxy * functionProxy = function->GetFunctionProxy();
        DynamicTypeHandler *typeHandler = function->GetTypeHandler();
        if (typeHandler->IsDeferredTypeHandler())
        {
            if (functionProxy && functionProxy->GetCrossSiteDeferredFunctionType())
            {
                function->ReplaceType(functionProxy->GetCrossSiteDeferredFunctionType());
            }
            else
            {
                function->ChangeType();
                function->SetEntryPoint(scriptContext->CurrentCrossSiteThunk);
                if (functionProxy && functionProxy->HasParseableInfo() && !PHASE_OFF1(ShareCrossSiteFuncTypesPhase))
                {
                    function->ShareType();
                    functionProxy->SetCrossSiteDeferredFunctionType(UnsafeVarTo<ScriptFunction>(function)->GetScriptFunctionType());
                }
            }
        }
        else 
        {
            if (functionProxy && functionProxy->GetCrossSiteUndeferredFunctionType())
            {
                function->ReplaceType(functionProxy->GetCrossSiteUndeferredFunctionType());
            }
            else
            {
                if (typeHandler->IsPathTypeHandler())
                {
                    if (!PHASE_OFF1(ShareCrossSiteFuncTypesPhase))
                    {
                        DynamicType *type = function->DuplicateType();
                        PathTypeHandlerBase::FromTypeHandler(typeHandler)->BuildPathTypeFromNewRoot(function, &type);
                        function->ReplaceType(type);
                    }
                    else
                    {
                        PathTypeHandlerBase::FromTypeHandler(typeHandler)->ConvertToNonShareableTypeHandler(function);
                    }
                }
                else
                {
                    function->ChangeType();
                }
                function->SetEntryPoint(scriptContext->CurrentCrossSiteThunk);
                if (functionProxy && functionProxy->HasParseableInfo() && function->GetTypeHandler()->GetMayBecomeShared() && !PHASE_OFF1(ShareCrossSiteFuncTypesPhase))
                {
                    function->ShareType();
                    functionProxy->SetCrossSiteUndeferredFunctionType(UnsafeVarTo<ScriptFunction>(function)->GetScriptFunctionType());
                }
            }
        }
    }

    JavascriptExternalFunction*
    JavascriptLibrary::CreateExternalFunction(ExternalMethod entryPoint, PropertyId nameId, Var signature, UINT64 flags, bool isLengthAvailable)
    {
        Assert(nameId == 0 || scriptContext->IsTrackedPropertyId(nameId));
        return CreateExternalFunction(entryPoint, TaggedInt::ToVarUnchecked(nameId), signature, flags, isLengthAvailable);
    }

    JavascriptExternalFunction*
    JavascriptLibrary::CreateExternalFunction(ExternalMethod entryPoint, Var nameId, Var signature, UINT64 flags, bool isLengthAvailable)
    {
        JavascriptExternalFunction* function = this->CreateIdMappedExternalFunction(entryPoint, isLengthAvailable ? externalFunctionWithLengthAndDeferredPrototypeType : externalFunctionWithDeferredPrototypeType);
        function->SetExternalFlags(flags);
        function->SetFunctionNameId(nameId);
        function->SetSignature(signature);
#ifdef HEAP_ENUMERATION_VALIDATION
        function->SetHeapEnumValidationCookie(HEAP_ENUMERATION_LIBRARY_OBJECT_COOKIE);
#endif

        JS_ETW_INTERNAL(EventWriteJSCRIPT_BUILD_DIRECT_FUNCTION(scriptContext, function, TaggedInt::Is(nameId) ? scriptContext->GetThreadContext()->GetPropertyName(TaggedInt::ToInt32(nameId))->GetBuffer() : ((JavascriptString *)nameId)->GetString()));
#if DBG_DUMP
        if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::HostPhase))
        {
            Output::Print(_u("Create new external function: methodAddr= %p, propertyRecord= %p, propertyName= %s\n"),
                this, nameId,
                TaggedInt::Is(nameId) ? scriptContext->GetThreadContext()->GetPropertyName(TaggedInt::ToInt32(nameId))->GetBuffer() : ((JavascriptString *)nameId)->GetString());
        }
#endif
        return function;
    }

#if DBG_DUMP
    const char16* JavascriptLibrary::GetStringTemplateCallsiteObjectKey(Var callsite)
    {
        // Calculate the key for the string template callsite object.
        // Key is combination of the raw string literals delimited by '${}' since string template literals cannot include that symbol.
        // `str1${expr1}str2${expr2}str3` => "str1${}str2${}str3"

        ES5Array* callsiteObj = VarTo<ES5Array>(callsite);
        ScriptContext* scriptContext = callsiteObj->GetScriptContext();

        Var var = JavascriptOperators::OP_GetProperty(callsiteObj, Js::PropertyIds::raw, scriptContext);
        ES5Array* rawArray = VarTo<ES5Array>(var);
        uint32 arrayLength = rawArray->GetLength();
        uint32 totalStringLength = 0;
        JavascriptString* str;

        Assert(arrayLength != 0);

        // Count the size in characters of the raw strings
        for (uint32 i = 0; i < arrayLength; i++)
        {
            rawArray->DirectGetItemAt(i, &var);
            str = VarTo<JavascriptString>(var);
            totalStringLength += str->GetLength();
        }

        uint32 keyLength = totalStringLength + (arrayLength - 1) * 3 + 1;
        char16* key = RecyclerNewArray(scriptContext->GetRecycler(), char16, keyLength);
        char16* ptr = key;
        charcount_t remainingSpace = keyLength;

        // Get first item before loop - there always must be at least one item
        rawArray->DirectGetItemAt(0, &var);
        str = VarTo<JavascriptString>(var);

        charcount_t len = str->GetLength();
        js_wmemcpy_s(ptr, remainingSpace, str->GetString(), len);
        ptr += len;
        remainingSpace -= len;

        // Append a delimiter and the rest of the items
        for (uint32 i = 1; i < arrayLength; i++)
        {
            len = 3; // strlen(_u("${}"));
            js_wmemcpy_s(ptr, remainingSpace, _u("${}"), len);
            ptr += len;
            remainingSpace -= len;

            rawArray->DirectGetItemAt(i, &var);
            str = VarTo<JavascriptString>(var);

            len = str->GetLength();
            js_wmemcpy_s(ptr, remainingSpace, str->GetString(), len);
            ptr += len;
            remainingSpace -= len;
        }

        // Ensure string is terminated
        key[keyLength - 1] = _u('\0');

        return key;
    }
#endif



    DynamicType * JavascriptLibrary::GetObjectLiteralType(uint16 requestedInlineSlotCapacity)
    {
        if (requestedInlineSlotCapacity <= MaxPreInitializedObjectTypeInlineSlotCount)
        {
            return objectTypes[DynamicTypeHandler::RoundUpInlineSlotCapacity(requestedInlineSlotCapacity) / InlineSlotCountIncrement];
        }
        else
        {
            return objectTypes[PreInitializedObjectTypeCount - 1];
        }
    }

    DynamicType * JavascriptLibrary::GetObjectHeaderInlinedLiteralType(uint16 requestedInlineSlotCapacity)
    {
        Assert(requestedInlineSlotCapacity <= MaxPreInitializedObjectHeaderInlinedTypeInlineSlotCount);

        return
            objectHeaderInlinedTypes[
                (
                    DynamicTypeHandler::RoundUpObjectHeaderInlinedInlineSlotCapacity(requestedInlineSlotCapacity) -
                    DynamicTypeHandler::GetObjectHeaderInlinableSlotCapacity()
                    ) / InlineSlotCountIncrement];
    }

    HeapArgumentsObject* JavascriptLibrary::CreateHeapArguments(Var frameObj, uint32 formalCount, bool isStrictMode)
    {
        AssertMsg(heapArgumentsType, "Where's heapArgumentsType?");
        Assert(strictHeapArgumentsType);

        Recycler *recycler = this->GetRecycler();

        EnsureArrayPrototypeValuesFunction(); //InitializeArrayPrototype can be delay loaded, which could prevent us from access to array.prototype.values

        DynamicType * argumentsType = isStrictMode ? strictHeapArgumentsType : heapArgumentsType;

        return RecyclerNewPlusZ(recycler, argumentsType->GetTypeHandler()->GetInlineSlotCapacity() * sizeof(Var), HeapArgumentsObject, recycler,
            frameObj != GetNull() ? static_cast<ActivationObject*>(frameObj) : nullptr,
            formalCount, argumentsType);
    }

    JavascriptArray* JavascriptLibrary::CreateArray()
    {
        AssertMsg(arrayType, "Where's arrayType?");
        return JavascriptArray::New<Var, JavascriptArray>(this->GetRecycler(), arrayType);
    }

    JavascriptArray* JavascriptLibrary::CreateArray(uint32 length)
    {
        AssertMsg(arrayType, "Where's arrayType?");
        JavascriptArray* arr = JavascriptArray::New<Var, JavascriptArray, 0>(length, arrayType, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        arr->CheckForceES5Array();
#endif
        return arr;
    }

    JavascriptArray *JavascriptLibrary::CreateArrayOnStack(void *const stackAllocationPointer)
    {
        return JavascriptArray::New<JavascriptArray, 0>(stackAllocationPointer, 0, arrayType);
    }

    JavascriptNativeIntArray* JavascriptLibrary::CreateNativeIntArray()
    {
        AssertMsg(nativeIntArrayType, "Where's nativeIntArrayType?");
        return JavascriptArray::New<int32, JavascriptNativeIntArray>(this->GetRecycler(), nativeIntArrayType);
    }

    JavascriptNativeIntArray* JavascriptLibrary::CreateNativeIntArray(uint32 length)
    {
        AssertMsg(nativeIntArrayType, "Where's nativeIntArrayType?");
        JavascriptNativeIntArray* arr = JavascriptArray::New<int32, JavascriptNativeIntArray, 0>(length, nativeIntArrayType, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        arr->CheckForceES5Array();
#endif
        return arr;
    }

    JavascriptNativeFloatArray* JavascriptLibrary::CreateNativeFloatArray()
    {
        AssertMsg(nativeFloatArrayType, "Where's nativeFloatArrayType?");
        return JavascriptArray::New<double, JavascriptNativeFloatArray>(this->GetRecycler(), nativeFloatArrayType);
    }

    JavascriptNativeFloatArray* JavascriptLibrary::CreateNativeFloatArray(uint32 length)
    {
        AssertMsg(nativeFloatArrayType, "Where's nativeIntArrayType?");
        JavascriptNativeFloatArray* arr = JavascriptArray::New<double, JavascriptNativeFloatArray, 0>(length, nativeFloatArrayType, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        arr->CheckForceES5Array();
#endif
        return arr;
    }

    JavascriptArray* JavascriptLibrary::CreateArrayLiteral(uint32 length)
    {
        AssertMsg(arrayType, "Where's arrayType?");
        JavascriptArray* arr = JavascriptArray::NewLiteral<Var, JavascriptArray, 0>(length, arrayType, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        arr->CheckForceES5Array();
#endif
        return arr;
    }

    JavascriptNativeIntArray* JavascriptLibrary::CreateNativeIntArrayLiteral(uint32 length)
    {
        AssertMsg(nativeIntArrayType, "Where's arrayType?");
        JavascriptNativeIntArray* arr = JavascriptArray::NewLiteral<int32, JavascriptNativeIntArray, 0>(length, nativeIntArrayType, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

        return arr;
    }

#if ENABLE_COPYONACCESS_ARRAY
    JavascriptNativeIntArray* JavascriptLibrary::CreateCopyOnAccessNativeIntArrayLiteral(ArrayCallSiteInfo *arrayInfo, FunctionBody *functionBody, const Js::AuxArray<int32> *ints)
    {
        AssertMsg(copyOnAccessNativeIntArrayType, "Where's arrayType?");
        JavascriptNativeIntArray* arr = JavascriptArray::NewCopyOnAccessLiteral<int32, JavascriptCopyOnAccessNativeIntArray, 0>(copyOnAccessNativeIntArrayType, arrayInfo, functionBody, ints, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

        return arr;
    }
#endif

    JavascriptNativeFloatArray* JavascriptLibrary::CreateNativeFloatArrayLiteral(uint32 length)
    {
        AssertMsg(nativeFloatArrayType, "Where's arrayType?");
        JavascriptNativeFloatArray* arr = JavascriptArray::NewLiteral<double, JavascriptNativeFloatArray, 0>(length, nativeFloatArrayType, this->GetRecycler());
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        arr->CheckForceES5Array();
#endif
        return arr;
    }

    JavascriptArray* JavascriptLibrary::CreateArray(uint32 length, uint32 size)
    {
        AssertMsg(arrayType, "Where's arrayType?");
        JavascriptArray* arr = RecyclerNew(this->GetRecycler(), JavascriptArray, length, size, arrayType);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_ARRAY(arr));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        arr->CheckForceES5Array();
#endif
        return arr;
    }

    ArrayBuffer* JavascriptLibrary::CreateArrayBuffer(uint32 length)
    {
        ArrayBuffer* arr = JavascriptArrayBuffer::Create(length, arrayBufferType);
        return arr;
    }

    ArrayBuffer* JavascriptLibrary::CreateArrayBuffer(byte* buffer, uint32 length)
    {
        ArrayBuffer* arr = JavascriptArrayBuffer::Create(buffer, length, arrayBufferType);
        return arr;
    }

    ArrayBuffer* JavascriptLibrary::CreateArrayBuffer(RefCountedBuffer* buffer, uint32 length)
    {
        ArrayBuffer* arr = JavascriptArrayBuffer::Create(buffer, length, arrayBufferType);
        return arr;
    }

#ifdef ENABLE_WASM
    Js::WebAssemblyArrayBuffer* JavascriptLibrary::CreateWebAssemblyArrayBuffer(uint32 length)
    {
        return WebAssemblyArrayBuffer::Create(nullptr, length, arrayBufferType);
    }

    Js::WebAssemblyArrayBuffer* JavascriptLibrary::CreateWebAssemblyArrayBuffer(byte* buffer, uint32 length)
    {
        return WebAssemblyArrayBuffer::Create(buffer, length, arrayBufferType);
    }

#ifdef ENABLE_WASM_THREADS
    WebAssemblySharedArrayBuffer* JavascriptLibrary::CreateWebAssemblySharedArrayBuffer(uint32 length, uint32 maxLength)
    {
        return WebAssemblySharedArrayBuffer::Create(length, maxLength, sharedArrayBufferType);
    }

    WebAssemblySharedArrayBuffer* JavascriptLibrary::CreateWebAssemblySharedArrayBuffer(SharedContents *contents)
    {
        return WebAssemblySharedArrayBuffer::Create(contents, sharedArrayBufferType);
    }
#endif
#endif

    SharedArrayBuffer* JavascriptLibrary::CreateSharedArrayBuffer(uint32 length)
    {
        return JavascriptSharedArrayBuffer::Create(length, sharedArrayBufferType);
    }

    SharedArrayBuffer* JavascriptLibrary::CreateSharedArrayBuffer(SharedContents *contents)
    {
#ifdef ENABLE_WASM_THREADS
        if (contents && contents->IsWebAssembly())
        {
            return CreateWebAssemblySharedArrayBuffer(contents);
        }
#endif
        return JavascriptSharedArrayBuffer::Create(contents, sharedArrayBufferType);
    }

    ArrayBuffer* JavascriptLibrary::CreateProjectionArraybuffer(uint32 length)
    {
        ArrayBuffer* arr = ProjectionArrayBuffer::Create(length, arrayBufferType);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(arr));
        return arr;
    }

    ArrayBuffer* JavascriptLibrary::CreateProjectionArraybuffer(byte* buffer, uint32 length)
    {
        ArrayBuffer* arr = ProjectionArrayBuffer::Create(buffer, length, arrayBufferType);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(arr));
        return arr;
    }

    ArrayBuffer* JavascriptLibrary::CreateProjectionArraybuffer(RefCountedBuffer* buffer, uint32 length)
    {
        ArrayBuffer* arr = ProjectionArrayBuffer::Create(buffer, length, arrayBufferType);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(arr));
        return arr;
    }

    ArrayBuffer* JavascriptLibrary::CreateExternalArrayBuffer(RefCountedBuffer* buffer, uint32 length)
    {
        ArrayBuffer* arr = ExternalArrayBuffer::Create(buffer, length, arrayBufferType);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(arr));
        return arr;
    }

    DataView* JavascriptLibrary::CreateDataView(ArrayBufferBase* arrayBuffer, uint32 offset, uint32 length)
    {
        DataView* dataView = RecyclerNew(this->GetRecycler(), DataView, arrayBuffer, offset, length, dataViewType);

        return dataView;
    }

    JavascriptBoolean* JavascriptLibrary::CreateBoolean(BOOL value)
    {
        AssertMsg(booleanTrue, "Where's booleanTrue?");
        AssertMsg(booleanFalse, "Where's booleanFalse?");
        return value ? booleanTrue : booleanFalse;
    }

    JavascriptDate* JavascriptLibrary::CreateDate()
    {
        AssertMsg(dateType, "Where's dateType?");
        return RecyclerNew(this->GetRecycler(), JavascriptDate, 0, dateType);
    }

    JavascriptDate* JavascriptLibrary::CreateDate(double value)
    {
        AssertMsg(dateType, "Where's dateType?");
        return RecyclerNew(this->GetRecycler(), JavascriptDate, value, dateType);
    }

    JavascriptDate* JavascriptLibrary::CreateDate(SYSTEMTIME* pst)
    {
        AssertMsg(dateType, "Where's dateType?");
        double value = DateImplementation::TimeFromSt(pst);
        return CreateDate(value);
    }

    JavascriptMap* JavascriptLibrary::CreateMap()
    {
        AssertMsg(mapType, "Where's mapType?");
        return RecyclerNew(this->GetRecycler(), JavascriptMap, mapType);
    }

    JavascriptSet* JavascriptLibrary::CreateSet()
    {
        AssertMsg(setType, "Where's setType?");
        return RecyclerNew(this->GetRecycler(), JavascriptSet, setType);
    }

    JavascriptWeakMap* JavascriptLibrary::CreateWeakMap()
    {
        AssertMsg(weakMapType, "Where's weakMapType?");
        return RecyclerNewFinalized(this->GetRecycler(), JavascriptWeakMap, weakMapType);
    }

    JavascriptWeakSet* JavascriptLibrary::CreateWeakSet()
    {
        AssertMsg(weakSetType, "Where's weakSetType?");
        return RecyclerNewFinalized(this->GetRecycler(), JavascriptWeakSet, weakSetType);
    }

    JavascriptPromise* JavascriptLibrary::CreatePromise()
    {
        AssertMsg(promiseType, "Where's promiseType?");
        return RecyclerNew(this->GetRecycler(), JavascriptPromise, promiseType);
    }

    JavascriptAsyncSpawnStepFunction* JavascriptLibrary::CreateAsyncSpawnStepFunction(
        JavascriptMethod entryPoint,
        JavascriptGenerator* generator,
        Var argument,
        Var resolve,
        Var reject,
        bool isReject)
    {
        FunctionInfo* functionInfo = RecyclerNew(GetRecycler(), FunctionInfo, entryPoint);
        DynamicType* type = CreateDeferredPrototypeFunctionType(
            this->inDispatchProfileMode ? ProfileEntryThunk : entryPoint);

        return RecyclerNewEnumClass(
            GetRecycler(),
            EnumFunctionClass,
            JavascriptAsyncSpawnStepFunction,
            type,
            functionInfo,
            generator,
            argument,
            resolve,
            reject,
            isReject);
    }

    JavascriptGenerator* JavascriptLibrary::CreateGenerator(
        Arguments& args,
        ScriptFunction* scriptFunction,
        RecyclableObject* prototype)
    {
        Assert(scriptContext->GetConfig()->IsES6GeneratorsEnabled());
        return JavascriptGenerator::New(
            GetRecycler(),
            CreateGeneratorType(prototype),
            args,
            scriptFunction);
    }

    JavascriptAsyncGenerator* JavascriptLibrary::CreateAsyncGenerator(
        Arguments& args,
        ScriptFunction* scriptFunction,
        RecyclableObject* prototype)
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());
        return JavascriptAsyncGenerator::New(
            GetRecycler(),
            CreateAsyncGeneratorType(prototype),
            args,
            scriptFunction);
    }

    JavascriptError* JavascriptLibrary::CreateError()
    {
        AssertMsg(errorType, "Where's errorType?");
        JavascriptError *pError = RecyclerNew(this->GetRecycler(), JavascriptError, errorType);
        JavascriptError::SetErrorType(pError, kjstError);
        return pError;
    }

    JavascriptSymbol* JavascriptLibrary::CreateSymbol(JavascriptString* description)
    {
        return this->CreateSymbol(description->GetString(), (int)description->GetLength());
    }

    JavascriptSymbol* JavascriptLibrary::CreateSymbol(const char16* description, int descriptionLength)
    {
        ENTER_PINNED_SCOPE(const Js::PropertyRecord, propertyRecord);

        propertyRecord = this->scriptContext->GetThreadContext()->UncheckedAddPropertyId(description, descriptionLength, /*bind*/false, /*isSymbol*/true);

        LEAVE_PINNED_SCOPE();

        return this->CreateSymbol(propertyRecord);
    }

    JavascriptSymbol* JavascriptLibrary::CreateSymbol(const PropertyRecord* propertyRecord)
    {
        AssertMsg(symbolTypeStatic, "Where's symbolTypeStatic?");
        SymbolCacheMap* symbolMap = this->EnsureSymbolMap();
        JavascriptSymbol* symbol = RecyclerNew(this->GetRecycler(), JavascriptSymbol, propertyRecord, symbolTypeStatic);
#if ENABLE_WEAK_REFERENCE_REGIONS
        symbolMap->Item(propertyRecord->GetPropertyId(), symbol);
#else
        symbolMap->Item(propertyRecord->GetPropertyId(), recycler->CreateWeakReferenceHandle(symbol));
#endif
        return symbol;
    }

    JavascriptError* JavascriptLibrary::CreateExternalError(ErrorTypeEnum errorTypeEnum)
    {
        DynamicType* baseErrorType = NULL;
        switch (errorTypeEnum)
        {
        case kjstError:
        default:
            baseErrorType = errorType;
            break;
        case kjstEvalError:
            baseErrorType = evalErrorType;
            break;
        case kjstRangeError:
            baseErrorType = rangeErrorType;
            break;
        case kjstReferenceError:
            baseErrorType = referenceErrorType;
            break;
        case kjstSyntaxError:
            baseErrorType = syntaxErrorType;
            break;
        case kjstTypeError:
            baseErrorType = typeErrorType;
            break;
        case kjstURIError:
            baseErrorType = uriErrorType;
            break;
        case kjstAggregateError:
            baseErrorType = aggregateErrorType;
            break;
        case kjstWebAssemblyCompileError:
            baseErrorType = webAssemblyCompileErrorType;
            break;
        case kjstWebAssemblyRuntimeError:
            baseErrorType = webAssemblyRuntimeErrorType;
            break;
        case kjstWebAssemblyLinkError:
            baseErrorType = webAssemblyLinkErrorType;
            break;
        }

        JavascriptError *pError = RecyclerNew(recycler, JavascriptError, baseErrorType, TRUE);
        JavascriptError::SetErrorType(pError, errorTypeEnum);
        return pError;
    }

#define CREATE_ERROR(name, field, id) \
    JavascriptError* JavascriptLibrary::Create##name() \
    { \
        AssertMsg(field, "Where's field?"); \
        JavascriptError *pError = RecyclerNew(this->GetRecycler(), JavascriptError, field); \
        JavascriptError::SetErrorType(pError, id); \
        return pError; \
    }

    CREATE_ERROR(EvalError, evalErrorType, kjstEvalError);
    CREATE_ERROR(RangeError, rangeErrorType, kjstRangeError);
    CREATE_ERROR(ReferenceError, referenceErrorType, kjstReferenceError);
    CREATE_ERROR(SyntaxError, syntaxErrorType, kjstSyntaxError);
    CREATE_ERROR(TypeError, typeErrorType, kjstTypeError);
    CREATE_ERROR(URIError, uriErrorType, kjstURIError);
    CREATE_ERROR(AggregateError, aggregateErrorType, kjstAggregateError);
    CREATE_ERROR(WebAssemblyCompileError, webAssemblyCompileErrorType, kjstWebAssemblyCompileError);
    CREATE_ERROR(WebAssemblyRuntimeError, webAssemblyRuntimeErrorType, kjstWebAssemblyRuntimeError);
    CREATE_ERROR(WebAssemblyLinkError, webAssemblyLinkErrorType, kjstWebAssemblyLinkError);

#undef CREATE_ERROR

    JavascriptError* JavascriptLibrary::CreateStackOverflowError()
    {
#if DBG
        // If we are doing a heap enum, we need to be able to allocate the error object.
        Recycler::AutoAllowAllocationDuringHeapEnum autoAllowAllocationDuringHeapEnum(this->GetRecycler());
#endif

        JavascriptError* stackOverflowError = scriptContext->GetLibrary()->CreateError();
        JavascriptError::SetErrorMessage(stackOverflowError, VBSERR_OutOfStack, NULL, scriptContext);
        return stackOverflowError;
    }

    JavascriptError* JavascriptLibrary::CreateOutOfMemoryError()
    {
        JavascriptError* outOfMemoryError = scriptContext->GetLibrary()->CreateError();
        JavascriptError::SetErrorMessage(outOfMemoryError, VBSERR_OutOfMemory, NULL, scriptContext);
        return outOfMemoryError;
    }

    JavascriptFunction* JavascriptLibrary::CreateNonProfiledFunction(FunctionInfo * functionInfo)
    {
        Assert(functionInfo->GetAttributes() & FunctionInfo::DoNotProfile);
        return RecyclerNew(this->GetRecycler(), RuntimeFunction,
            CreateDeferredPrototypeFunctionTypeNoProfileThunk(functionInfo->GetOriginalEntryPoint()),
            functionInfo);
    }

    ScriptFunction* JavascriptLibrary::CreateScriptFunction(FunctionProxy * proxy)
    {
        ScriptFunctionType * type = proxy->IsClassConstructor() && proxy->GetUndeferredFunctionType() ? proxy->GetUndeferredFunctionType() : proxy->EnsureDeferredPrototypeType();
        FunctionInfo* functionInfo = proxy->GetFunctionInfo();
        if (functionInfo->HasComputedName() || functionInfo->HasHomeObj())
        {
            if (functionInfo->HasComputedName() && functionInfo->HasHomeObj())
            {
                return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, FunctionWithComputedName<FunctionWithHomeObj<ScriptFunction>>, proxy, type);
            }
            else if (functionInfo->HasHomeObj())
            {
                return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, FunctionWithHomeObj<ScriptFunction>, proxy, type);
            }

            // Has computed Name
            return RecyclerNewWithInfoBits(this->GetRecycler(), EnumFunctionClass, ScriptFunctionWithComputedName, proxy, type);
        }
        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, ScriptFunction, proxy, type);
    }

    AsmJsScriptFunction* JavascriptLibrary::CreateAsmJsScriptFunction(FunctionProxy * proxy)
    {
        ScriptFunctionType* deferredPrototypeType = proxy->EnsureDeferredPrototypeType();
        Assert(!proxy->GetFunctionInfo()->HasHomeObj());
        if (proxy->GetFunctionInfo()->HasComputedName())
        {
            return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, AsmJsScriptFunctionWithComputedName, proxy, deferredPrototypeType);
        }
        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, AsmJsScriptFunction, proxy, deferredPrototypeType);
    }

#ifdef ENABLE_WASM
    WasmScriptFunction * JavascriptLibrary::CreateWasmScriptFunction(FunctionProxy* proxy)
    {
        Assert(!proxy->GetFunctionInfo()->HasComputedName());
        Assert(!proxy->GetFunctionInfo()->HasHomeObj());
        ScriptFunctionType* deferredPrototypeType = proxy->EnsureDeferredPrototypeType();
        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, WasmScriptFunction, proxy, deferredPrototypeType);
    }
#endif

    ScriptFunctionWithInlineCache* JavascriptLibrary::CreateScriptFunctionWithInlineCache(FunctionProxy * proxy)
    {
        ScriptFunctionType* deferredPrototypeType = proxy->EnsureDeferredPrototypeType();
        FunctionInfo* functionInfo = proxy->GetFunctionInfo();
        if (functionInfo->HasComputedName() || functionInfo->HasHomeObj())
        {
            if (functionInfo->HasComputedName() && functionInfo->HasHomeObj())
            {
                return RecyclerNewWithInfoBits(this->GetRecycler(), (Memory::ObjectInfoBits)(EnumFunctionClass | Memory::FinalizableObjectBits), FunctionWithComputedName<FunctionWithHomeObj<ScriptFunctionWithInlineCache>>, proxy, deferredPrototypeType);
            }
            else if (functionInfo->HasHomeObj())
            {
                return RecyclerNewWithInfoBits(this->GetRecycler(), (Memory::ObjectInfoBits)(EnumFunctionClass | Memory::FinalizableObjectBits), FunctionWithHomeObj<ScriptFunctionWithInlineCache>, proxy, deferredPrototypeType);
            }
            return RecyclerNewWithInfoBits(this->GetRecycler(), (Memory::ObjectInfoBits)(EnumFunctionClass | Memory::FinalizableObjectBits), ScriptFunctionWithInlineCacheAndComputedName, proxy, deferredPrototypeType);
        }
        return RecyclerNewWithInfoBits(this->GetRecycler(), (Memory::ObjectInfoBits)(EnumFunctionClass | Memory::FinalizableObjectBits), ScriptFunctionWithInlineCache, proxy, deferredPrototypeType);
    }

    GeneratorVirtualScriptFunction* JavascriptLibrary::CreateGeneratorVirtualScriptFunction(FunctionProxy * proxy)
    {
        ScriptFunctionType* deferredPrototypeType = proxy->EnsureDeferredPrototypeType();
        FunctionInfo* functionInfo = proxy->GetFunctionInfo();
        if (functionInfo->HasComputedName() || functionInfo->HasHomeObj())
        {
            if (functionInfo->HasComputedName() && functionInfo->HasHomeObj())
            {
                return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, FunctionWithComputedName<FunctionWithHomeObj<GeneratorVirtualScriptFunction>>, proxy, deferredPrototypeType);
            }
            else if (functionInfo->HasHomeObj())
            {
                return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, FunctionWithHomeObj<GeneratorVirtualScriptFunction>, proxy, deferredPrototypeType);
            }
            return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, GeneratorVirtualScriptFunctionWithComputedName, proxy, deferredPrototypeType);
        }
        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, GeneratorVirtualScriptFunction, proxy, deferredPrototypeType);
    }

    DynamicType * JavascriptLibrary::CreateGeneratorType(RecyclableObject* prototype)
    {
        return DynamicType::New(scriptContext, TypeIds_Generator, prototype, nullptr, NullTypeHandler<false>::GetDefaultInstance());
    }

    DynamicType* JavascriptLibrary::CreateAsyncGeneratorType(RecyclableObject* prototype)
    {
        return DynamicType::New(
            scriptContext,
            TypeIds_AsyncGenerator,
            prototype,
            nullptr,
            NullTypeHandler<false>::GetDefaultInstance());
    }

    DynamicType * JavascriptLibrary::CreateAsyncFromSyncIteratorType()
    {
        return DynamicType::New(scriptContext, TypeIds_AsyncFromSyncIterator, asyncFromSyncIteratorProtototype, nullptr, NullTypeHandler<false>::GetDefaultInstance());
    }

    template <class MethodType>
    JavascriptExternalFunction* JavascriptLibrary::CreateIdMappedExternalFunction(MethodType entryPoint, DynamicType *pPrototypeType)
    {
        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptExternalFunction, entryPoint, pPrototypeType);
    }


    RuntimeFunction* JavascriptLibrary::EnsureAsyncFromSyncIteratorValueUnwrapFalseFunction()
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());
        if (asyncFromSyncIteratorValueUnwrapFalseFunction == nullptr)
        {
            JavascriptMethod entryPoint = JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorValueUnwrapFalseFunction;
            FunctionInfo* functionInfo = RecyclerNew(this->GetRecycler(), FunctionInfo, entryPoint);
            DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());

            asyncFromSyncIteratorValueUnwrapFalseFunction = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, RuntimeFunction, type, functionInfo);
        }

        return asyncFromSyncIteratorValueUnwrapFalseFunction;
    }

    RuntimeFunction* JavascriptLibrary::EnsureAsyncFromSyncIteratorValueUnwrapTrueFunction()
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());
        if (asyncFromSyncIteratorValueUnwrapTrueFunction == nullptr)
        {
            JavascriptMethod entryPoint = JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorValueUnwrapTrueFunction;
            FunctionInfo* functionInfo = RecyclerNew(this->GetRecycler(), FunctionInfo, entryPoint);
            DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());

            asyncFromSyncIteratorValueUnwrapTrueFunction = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, RuntimeFunction, type, functionInfo);
        }

        return asyncFromSyncIteratorValueUnwrapTrueFunction;
    }

    AsyncGeneratorCallbackFunction* JavascriptLibrary::CreateAsyncGeneratorCallbackFunction(
        JavascriptMethod entryPoint,
        JavascriptAsyncGenerator* generator)
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());

        auto* functionInfo = RecyclerNew(GetRecycler(), FunctionInfo, entryPoint);

        auto* type = DynamicType::New(
            scriptContext,
            TypeIds_Function,
            functionPrototype,
            entryPoint,
            GetDeferredAnonymousFunctionTypeHandler());

        return RecyclerNewEnumClass(
            GetRecycler(),
            EnumFunctionClass,
            AsyncGeneratorCallbackFunction,
            type,
            functionInfo,
            generator);
    }

    RuntimeFunction* JavascriptLibrary::CreateAsyncModuleCallbackFunction(
        JavascriptMethod entryPoint,
        SourceTextModuleRecord* module)
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());

        auto* functionInfo = RecyclerNew(GetRecycler(), FunctionInfo, entryPoint);

        auto* type = DynamicType::New(
            scriptContext,
            TypeIds_Function,
            functionPrototype,
            entryPoint,
            GetDeferredAnonymousFunctionTypeHandler());

        return RecyclerNewEnumClass(
            GetRecycler(),
            EnumFunctionClass,
            AsyncModuleCallbackFunction,
            type,
            functionInfo,
            module);
    }

    JavascriptAsyncGeneratorFunction* JavascriptLibrary::CreateAsyncGeneratorFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction)
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());

        DynamicType* type = CreateDeferredPrototypeAsyncGeneratorFunctionType(entryPoint, scriptFunction->IsAnonymousFunction());

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptAsyncGeneratorFunction, type, scriptFunction);
    }

    JavascriptAsyncFromSyncIterator* JavascriptLibrary::CreateAsyncFromSyncIterator(RecyclableObject* syncIterator)
    {
        Assert(scriptContext->GetConfig()->IsES2018AsyncIterationEnabled());

        DynamicType* type = CreateAsyncFromSyncIteratorType();

        return RecyclerNew(this->GetRecycler(), JavascriptAsyncFromSyncIterator, type, syncIterator);
    }

    JavascriptGeneratorFunction* JavascriptLibrary::CreateGeneratorFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction)
    {
        Assert(scriptContext->GetConfig()->IsES6GeneratorsEnabled());

        DynamicType* type = CreateDeferredPrototypeGeneratorFunctionType(entryPoint, scriptFunction->IsAnonymousFunction());

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptGeneratorFunction, type, scriptFunction);
    }

    JavascriptGeneratorFunction* JavascriptLibrary::CreateGeneratorFunction(JavascriptMethod entryPoint, bool isAnonymousFunction)
    {
        Assert(scriptContext->GetConfig()->IsES6GeneratorsEnabled());

        DynamicType* type = CreateDeferredPrototypeGeneratorFunctionType(entryPoint, isAnonymousFunction);

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptGeneratorFunction, type, nullptr);
    }

    JavascriptAsyncFunction* JavascriptLibrary::CreateAsyncFunction(JavascriptMethod entryPoint, GeneratorVirtualScriptFunction* scriptFunction)
    {
        DynamicType* type = CreateDeferredPrototypeAsyncFunctionType(entryPoint, scriptFunction->IsAnonymousFunction());

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptAsyncFunction, type, scriptFunction);
    }

    JavascriptAsyncFunction* JavascriptLibrary::CreateAsyncFunction(JavascriptMethod entryPoint, bool isAnonymousFunction)
    {
        DynamicType* type = CreateDeferredPrototypeAsyncFunctionType(entryPoint, isAnonymousFunction);

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptAsyncFunction, type, nullptr);
    }

    JavascriptExternalFunction* JavascriptLibrary::CreateStdCallExternalFunction(StdCallJavascriptMethod entryPoint, Var name, void *callbackState)
    {
        JavascriptExternalFunction* function = this->CreateIdMappedExternalFunction(entryPoint, stdCallFunctionWithDeferredPrototypeType);
        function->SetFunctionNameId(name);
        function->SetCallbackState(callbackState);
        return function;
    }

    JavascriptPromiseCapabilitiesExecutorFunction* JavascriptLibrary::CreatePromiseCapabilitiesExecutorFunction(JavascriptMethod entryPoint, JavascriptPromiseCapability* capability)
    {
        FunctionInfo* functionInfo = &Js::JavascriptPromise::EntryInfo::CapabilitiesExecutorFunction;
        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());
        JavascriptPromiseCapabilitiesExecutorFunction* function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseCapabilitiesExecutorFunction, type, functionInfo, capability);

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(2), PropertyConfigurable, nullptr);

        return function;
    }

    JavascriptPromiseResolveOrRejectFunction* JavascriptLibrary::CreatePromiseResolveOrRejectFunction(JavascriptMethod entryPoint, JavascriptPromise* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord)
    {
        FunctionInfo* functionInfo = &Js::JavascriptPromise::EntryInfo::ResolveOrRejectFunction;
        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());
        JavascriptPromiseResolveOrRejectFunction* function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseResolveOrRejectFunction, type, functionInfo, promise, isReject, alreadyResolvedRecord);

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);

        return function;
    }

    JavascriptPromiseReactionTaskFunction* JavascriptLibrary::CreatePromiseReactionTaskFunction(JavascriptMethod entryPoint, JavascriptPromiseReaction* reaction, Var argument)
    {
        FunctionInfo* functionInfo = RecyclerNew(this->GetRecycler(), FunctionInfo, entryPoint, FunctionInfo::Attributes::ErrorOnNew);
        DynamicType* type = CreateDeferredPrototypeFunctionType(entryPoint);

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseReactionTaskFunction, type, functionInfo, reaction, argument);
    }

    JavascriptPromiseResolveThenableTaskFunction* JavascriptLibrary::CreatePromiseResolveThenableTaskFunction(JavascriptMethod entryPoint, JavascriptPromise* promise, RecyclableObject* thenable, RecyclableObject* thenFunction)
    {
        FunctionInfo* functionInfo = RecyclerNew(this->GetRecycler(), FunctionInfo, entryPoint, FunctionInfo::Attributes::ErrorOnNew);
        DynamicType* type = CreateDeferredPrototypeFunctionType(entryPoint);

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseResolveThenableTaskFunction, type, functionInfo, promise, thenable, thenFunction);
    }

    JavascriptPromiseAllResolveElementFunction* JavascriptLibrary::CreatePromiseAllResolveElementFunction(JavascriptMethod entryPoint, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElements)
    {
        FunctionInfo* functionInfo = &Js::JavascriptPromise::EntryInfo::AllResolveElementFunction;
        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());
        JavascriptPromiseAllResolveElementFunction* function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseAllResolveElementFunction, type, functionInfo, index, values, capabilities, remainingElements);

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);

        return function;
    }

    JavascriptPromiseAnyRejectElementFunction* JavascriptLibrary::CreatePromiseAnyRejectElementFunction(JavascriptMethod entryPoint, uint32 index, JavascriptArray* errors, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElements, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper)
    {
        FunctionInfo* functionInfo = &Js::JavascriptPromise::EntryInfo::AnyRejectElementFunction;
        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());
        JavascriptPromiseAnyRejectElementFunction* function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseAnyRejectElementFunction, type, functionInfo, index, errors, capabilities, remainingElements, alreadyCalledWrapper);
        // Length should be 1 but not accessible from script.
        return function;
    }

    JavascriptPromiseAllSettledResolveOrRejectElementFunction* JavascriptLibrary::CreatePromiseAllSettledResolveOrRejectElementFunction(JavascriptMethod entryPoint, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElements, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper, bool isRejecting)
    {
        FunctionInfo* functionInfo = &Js::JavascriptPromise::EntryInfo::AllSettledResolveOrRejectElementFunction;
        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());
        JavascriptPromiseAllSettledResolveOrRejectElementFunction* function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseAllSettledResolveOrRejectElementFunction, type, functionInfo, index, values, capabilities, remainingElements, alreadyCalledWrapper, isRejecting);

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);

        return function;
    }

    JavascriptPromiseThenFinallyFunction* JavascriptLibrary::CreatePromiseThenFinallyFunction(JavascriptMethod entryPoint, RecyclableObject* OnFinally, RecyclableObject* Constructor, bool shouldThrow)
    {
        FunctionInfo* functionInfo = RecyclerNew(this->GetRecycler(), FunctionInfo, entryPoint, FunctionInfo::Attributes::ErrorOnNew);
        DynamicType* type = DynamicType::New(scriptContext, TypeIds_Function, functionPrototype, entryPoint, GetDeferredAnonymousFunctionTypeHandler());

        JavascriptPromiseThenFinallyFunction* function = RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseThenFinallyFunction, type, functionInfo, OnFinally, Constructor, shouldThrow);
        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(1), PropertyConfigurable, nullptr);

        return function;
    }

    JavascriptPromiseThunkFinallyFunction* JavascriptLibrary::CreatePromiseThunkFinallyFunction(JavascriptMethod entryPoint, Var value, bool shouldThrow)
    {
        FunctionInfo* functionInfo = RecyclerNew(this->GetRecycler(), FunctionInfo, entryPoint, FunctionInfo::Attributes::ErrorOnNew);
        DynamicType* type = CreateDeferredPrototypeFunctionType(entryPoint);

        return RecyclerNewEnumClass(this->GetRecycler(), EnumFunctionClass, JavascriptPromiseThunkFinallyFunction, type, functionInfo, value, shouldThrow);
    }

    JavascriptExternalFunction* JavascriptLibrary::CreateWrappedExternalFunction(JavascriptExternalFunction* wrappedFunction)
    {
        // The wrapped function will have profiling, so the wrapper function does not need it.
        JavascriptExternalFunction* function = RecyclerNew(this->GetRecycler(), JavascriptExternalFunction, wrappedFunction, wrappedFunctionWithDeferredPrototypeType);
        function->SetFunctionNameId(wrappedFunction->GetSourceString());

        return function;
    }

#if !FLOATVAR
    JavascriptNumber * JavascriptLibrary::CreateNumber(double value, RecyclerJavascriptNumberAllocator * numberAllocator)
    {
        AssertMsg(numberTypeStatic, "Where's numberTypeStatic?");
        return AllocatorNew(RecyclerJavascriptNumberAllocator, numberAllocator, JavascriptNumber, value, numberTypeStatic);
    }

#if ENABLE_NATIVE_CODEGEN
    JavascriptNumber* JavascriptLibrary::CreateCodeGenNumber(CodeGenNumberAllocator * alloc, double value)
    {
        AssertMsg(numberTypeStatic, "Where's numberTypeStatic?");
        return new (alloc->Alloc()) JavascriptNumber(value, numberTypeStatic);
    }
#endif

#endif

    DynamicObject* JavascriptLibrary::CreateGeneratorConstructorPrototypeObject()
    {
        AssertMsg(generatorConstructorPrototypeObjectType, "Where's generatorConstructorPrototypeObjectType?");
        DynamicObject * prototype = DynamicObject::New(this->GetRecycler(), generatorConstructorPrototypeObjectType);
        // Generator functions' prototype objects are not created with a .constructor property
        return prototype;
    }

    DynamicObject* JavascriptLibrary::CreateAsyncGeneratorConstructorPrototypeObject()
    {
        AssertMsg(asyncGeneratorConstructorPrototypeObjectType, "Where's asyncGeneratorConstructorPrototypeObjectType?");
        DynamicObject * prototype = DynamicObject::New(this->GetRecycler(), asyncGeneratorConstructorPrototypeObjectType);
        // Generator functions' prototype objects are not created with a .constructor property
        return prototype;
    }

    DynamicObject* JavascriptLibrary::CreateConstructorPrototypeObject(JavascriptFunction * constructor)
    {
        AssertMsg(constructorPrototypeObjectType, "Where's constructorPrototypeObjectType?");
        DynamicObject * prototype = DynamicObject::New(this->GetRecycler(), constructorPrototypeObjectType);
        AddMember(prototype, PropertyIds::constructor, constructor);
        return prototype;
    }

    DynamicObject* JavascriptLibrary::CreateClassPrototypeObject(RecyclableObject * protoParent)
    {
        // We can't share types of objects that are prototypes. If we gain the ability to do that, try using a shared type
        // with a PathTypeHandler for this object. (PathTypeHandler and not SimpleTypeHandler, because it will likely have
        // user-defined properties on it.) Until then, make a new type for each object and use a SimpleDictionaryTypeHandler.
        DynamicType * dynamicType = 
            DynamicType::New(scriptContext, TypeIds_Object, protoParent, nullptr, classPrototypeTypeHandler);
        dynamicType->SetHasNoEnumerableProperties(true);
        DynamicObject * proto = DynamicObject::New(this->GetRecycler(), dynamicType);
        return proto;
    }

    DynamicObject* JavascriptLibrary::CreateObject(
        const bool allowObjectHeaderInlining,
        const PropertyIndex requestedInlineSlotCapacity)
    {
        Assert(GetObjectType());
        Assert(GetObjectHeaderInlinedType());

        const bool useObjectHeaderInlining =
            allowObjectHeaderInlining && FunctionBody::DoObjectHeaderInliningForObjectLiteral(requestedInlineSlotCapacity);
        DynamicType *const type =
            useObjectHeaderInlining
            ? GetObjectHeaderInlinedLiteralType(requestedInlineSlotCapacity)
            : GetObjectLiteralType(requestedInlineSlotCapacity);
        return DynamicObject::New(GetRecycler(), type);
    }

    DynamicObject* JavascriptLibrary::CreateObject(DynamicTypeHandler * typeHandler)
    {
        return DynamicObject::New(this->GetRecycler(),
            Js::DynamicType::New(scriptContext, Js::TypeIds_Object, this->GetObjectPrototype(),
                RecyclableObject::DefaultEntryPoint, typeHandler, false, false));
    }

    DynamicType* JavascriptLibrary::CreateObjectType(RecyclableObject* prototype, Js::TypeId typeId, uint16 requestedInlineSlotCapacity)
    {
        const bool useObjectHeaderInlining = FunctionBody::DoObjectHeaderInliningForConstructor(requestedInlineSlotCapacity);
        const uint16 offsetOfInlineSlots =
            useObjectHeaderInlining
            ? DynamicTypeHandler::GetOffsetOfObjectHeaderInlineSlots()
            : sizeof(DynamicObject);

        DynamicType* dynamicType = nullptr;
        const bool useCache = prototype->GetScriptContext() == this->scriptContext;
#if DBG
        DynamicType* oldCachedType = nullptr;
        char16 reason[1024];
        swprintf_s(reason, 1024, _u("Cache not populated."));
#endif
        // Always use `TypeOfPrototypeObjectInlined` because we are creating DynamicType of TypeIds_Object
        AssertMsg(typeId == TypeIds_Object, "CreateObjectType() is used to create other objects. Please update cacheSlot for protoObjectCache first.");

        if (useCache &&
            prototype->GetInternalProperty(prototype, Js::InternalPropertyIds::TypeOfPrototypeObjectInlined, (Js::Var*) &dynamicType, nullptr, this->scriptContext))
        {
            //If the prototype is externalObject, then ExternalObject::Reinitialize can set all the properties to undefined in navigation scenario.
            //Check to make sure dynamicType which is stored as a Js::Var is not undefined.
            //See Blue 419324
            if (dynamicType != nullptr && (Js::Var)dynamicType != this->GetUndefined())
            {
                DynamicTypeHandler *const dynamicTypeHandler = dynamicType->GetTypeHandler();
                if (dynamicTypeHandler->IsObjectHeaderInlinedTypeHandler() == useObjectHeaderInlining &&
                    (
                        dynamicTypeHandler->GetInlineSlotCapacity() ==
                        (
                            useObjectHeaderInlining
                            ? DynamicTypeHandler::RoundUpObjectHeaderInlinedInlineSlotCapacity(requestedInlineSlotCapacity)
                            : DynamicTypeHandler::RoundUpInlineSlotCapacity(requestedInlineSlotCapacity)
                            )
                        ))
                {
                    Assert(dynamicType->GetIsShared());

                    if (PHASE_TRACE1(TypeShareForChangePrototypePhase))
                    {
#if DBG
                        if (PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
                        {
                            Output::Print(_u("TypeSharing: Reusing prototype [0x%p] object's InlineSlot cache 0x%p in CreateObject.\n"), prototype, dynamicType);
                        }
                        else
                        {
#endif
                            Output::Print(_u("TypeSharing: Reusing prototype object's InlineSlot cache in __proto__.\n"));
#if DBG
                        }
#endif
                        Output::Flush();
                    }

                    return dynamicType;
                }
#if DBG
                if (PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
                {
                    if (dynamicTypeHandler->IsObjectHeaderInlinedTypeHandler() != useObjectHeaderInlining)
                    {
                        swprintf_s(reason, 1024, _u("useObjectHeaderInlining mismatch."));
                    }
                    else
                    {
                        uint16 cachedCapacity = dynamicTypeHandler->GetInlineSlotCapacity();
                        uint16 requiredCapacity = useObjectHeaderInlining
                            ? DynamicTypeHandler::RoundUpObjectHeaderInlinedInlineSlotCapacity(requestedInlineSlotCapacity)
                            : DynamicTypeHandler::RoundUpInlineSlotCapacity(requestedInlineSlotCapacity);

                        swprintf_s(reason, 1024, _u("inlineSlotCapacity mismatch. Required = %d, Cached = %d"), requiredCapacity, cachedCapacity);
                    }
                }
#endif
            }

        }

#if DBG
        if (PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
        {
            if (dynamicType == nullptr)
            {
                swprintf_s(reason, 1024, _u("cached type was null"));
            }
            else if ((Js::Var)dynamicType == this->GetUndefined())
            {
                swprintf_s(reason, 1024, _u("cached type was undefined"));
            }
        }
        oldCachedType = dynamicType;
#endif
        PathTypeHandlerNoAttr* typeHandler = PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, requestedInlineSlotCapacity, offsetOfInlineSlots, true, true);
        dynamicType = DynamicType::New(scriptContext, typeId, prototype, RecyclableObject::DefaultEntryPoint, typeHandler, true, true);

        if (useCache)
        {
            prototype->SetInternalProperty(Js::InternalPropertyIds::TypeOfPrototypeObjectInlined, (Var)dynamicType, PropertyOperationFlags::PropertyOperation_Force, nullptr);
            if (PHASE_TRACE1(TypeShareForChangePrototypePhase))
            {
#if DBG
                if (PHASE_VERBOSE_TRACE1(TypeShareForChangePrototypePhase))
                {
                    Output::Print(_u("TypeSharing: Updating prototype [0x%p] object's InlineSlot cache from 0x%p to 0x%p in CreateObject. Reason = %s\n"), prototype, oldCachedType, dynamicType, reason);
                }
                else
                {
#endif
                    Output::Print(_u("TypeSharing: Updating prototype object's InlineSlot cache in CreateObject.\n"));
#if DBG
                }
#endif
                Output::Flush();
            }
        }

        return dynamicType;
    }

    DynamicType* JavascriptLibrary::CreateObjectTypeNoCache(RecyclableObject* prototype, Js::TypeId typeId)
    {
        return DynamicType::New(scriptContext, typeId, prototype, RecyclableObject::DefaultEntryPoint,
            PathTypeHandlerNoAttr::New(scriptContext, this->GetRootPath(), 0, 0, 0, true, true), true, true);
    }

    DynamicType* JavascriptLibrary::CreateObjectType(RecyclableObject* prototype, uint16 requestedInlineSlotCapacity)
    {
        // We can't reuse the type in objectType even if the prototype is the object prototype, because those has inline slot capacity fixed
        return CreateObjectType(prototype, TypeIds_Object, requestedInlineSlotCapacity);
    }

    DynamicObject* JavascriptLibrary::CreateObject(RecyclableObject* prototype, uint16 requestedInlineSlotCapacity)
    {
        Assert(JavascriptOperators::IsObjectOrNull(prototype));
        DynamicType* type = nullptr;
        // If requested capacity is 0, we can't shrink, so it is already fixed and we can reuse the cached types
        // For other inline slot capacities, we might want to shrink so we can't use the cached types (whose slot capacities are fixed)
        //
        // REVIEW: Do we really need non-fixed inline slot capacity? The obvious downside is it prevents type sharing with the cached types
        if (requestedInlineSlotCapacity == 0 && JavascriptOperators::IsNull(prototype))
        {
            type = GetNullPrototypeObjectType();
        }
        else if(requestedInlineSlotCapacity == 0 && prototype == GetObjectPrototype())
        {
            type = GetObjectType();
        }
        else
        {
            type = CreateObjectType(prototype, requestedInlineSlotCapacity);
        }
        return DynamicObject::New(this->GetRecycler(), type);
    }

    PropertyStringCacheMap* JavascriptLibrary::EnsurePropertyStringMap()
    {
        if (this->propertyStringMap == nullptr)
        {
            this->propertyStringMap = RecyclerNew(this->recycler, PropertyStringCacheMap, this->GetRecycler(), 71);
            this->scriptContext->RegisterWeakReferenceDictionary((JsUtil::IWeakReferenceDictionary*) this->propertyStringMap);
        }
        return this->propertyStringMap;
    }

    EnumeratorCache* JavascriptLibrary::GetObjectAssignCache(Type* type)
    {
        return GetEnumeratorCache<Cache::AssignCacheSize>(type, &this->cache.assignCache);
    }

    EnumeratorCache* JavascriptLibrary::GetCreateKeysCache(Type* type)
    {
        return GetEnumeratorCache<Cache::CreateKeysCacheSize>(type, &this->cache.createKeysCache);
    }

    EnumeratorCache* JavascriptLibrary::GetStringifyCache(Type* type)
    {
        return GetEnumeratorCache<Cache::StringifyCacheSize>(type, &this->cache.stringifyCache);
    }

    template<uint cacheSlotCount> EnumeratorCache* JavascriptLibrary::GetEnumeratorCache(Type* type, Field(EnumeratorCache*)* cacheSlots)
    {
        // Size must be power of 2 for cache indexing to work
        CompileAssert((cacheSlotCount & (cacheSlotCount - 1)) == 0);

        if (*cacheSlots == nullptr)
        {
            *cacheSlots = AllocatorNewArrayZ(CacheAllocator, scriptContext->GetEnumeratorAllocator(), EnumeratorCache, cacheSlotCount);
        }
        return &(*cacheSlots)[(((uintptr_t)type) >> PolymorphicInlineCacheShift) & (cacheSlotCount - 1)];
    }

    SymbolCacheMap* JavascriptLibrary::EnsureSymbolMap()
    {
        if (this->symbolMap == nullptr)
        {
            this->symbolMap = RecyclerNew(this->recycler, SymbolCacheMap, this->GetRecycler(), 71);
            this->scriptContext->RegisterWeakReferenceDictionary((JsUtil::IWeakReferenceDictionary*) this->symbolMap);
        }
        return this->symbolMap;
    }

    DynamicObject* JavascriptLibrary::CreateActivationObject()
    {
        AssertMsg(activationObjectType, "Where's activationObjectType?");
        return RecyclerNew(this->GetRecycler(), ActivationObject, activationObjectType);
    }

    DynamicObject* JavascriptLibrary::CreatePseudoActivationObject()
    {
        AssertMsg(activationObjectType, "Where's activationObjectType?");
        return RecyclerNew(this->GetRecycler(), PseudoActivationObject, activationObjectType);
    }

    DynamicObject* JavascriptLibrary::CreateBlockActivationObject()
    {
        AssertMsg(activationObjectType, "Where's activationObjectType?");
        return RecyclerNew(this->GetRecycler(), BlockActivationObject, activationObjectType);
    }

    DynamicObject* JavascriptLibrary::CreateConsoleScopeActivationObject()
    {
        AssertMsg(activationObjectType, "Where's activationObjectType?");
        return RecyclerNew(this->GetRecycler(), ConsoleScopeActivationObject, activationObjectType);
    }

    JavascriptString* JavascriptLibrary::GetEmptyString() const
    {
        AssertMsg(emptyString, "Where's emptyString?");
#ifdef PROFILE_STRINGS
        StringProfiler::RecordEmptyStringRequest(scriptContext);
#endif
        return emptyString;
    }

    PropertyString* JavascriptLibrary::CreatePropertyString(const Js::PropertyRecord* propertyRecord)
    {
        return PropertyString::New(GetStringTypeStatic(), propertyRecord, this->GetRecycler());
    }

    JavascriptBooleanObject* JavascriptLibrary::CreateBooleanObject()
    {
        AssertMsg(booleanTypeDynamic, "Where's booleanTypeDynamic?");
        return RecyclerNew(this->GetRecycler(), JavascriptBooleanObject, nullptr, booleanTypeDynamic);
    }

    JavascriptBooleanObject* JavascriptLibrary::CreateBooleanObject(BOOL value)
    {
        AssertMsg(booleanTypeDynamic, "Where's booleanTypeDynamic?");
        return RecyclerNew(this->GetRecycler(), JavascriptBooleanObject, CreateBoolean(value), booleanTypeDynamic);
    }

    JavascriptSymbolObject* JavascriptLibrary::CreateSymbolObject(JavascriptSymbol* value)
    {
        AssertMsg(symbolTypeDynamic, "Where's symbolTypeDynamic?");
        return RecyclerNew(this->GetRecycler(), JavascriptSymbolObject, value, symbolTypeDynamic);
    }

    JavascriptNumberObject* JavascriptLibrary::CreateNumberObject(Var number)
    {
        AssertMsg(numberTypeDynamic, "Where's numberTypeDynamic?");
        return RecyclerNew(this->GetRecycler(), JavascriptNumberObject, number, numberTypeDynamic);
    }

    JavascriptNumberObject* JavascriptLibrary::CreateNumberObjectWithCheck(double value)
    {
        return CreateNumberObject(JavascriptNumber::ToVarWithCheck(value, scriptContext));
    }

    JavascriptStringObject* JavascriptLibrary::CreateStringObject(JavascriptString* value)
    {
        AssertMsg(stringTypeDynamic, "Where's stringTypeDynamic?");
        return RecyclerNew(this->GetRecycler(), JavascriptStringObject, value, stringTypeDynamic);
    }

    JavascriptStringObject* JavascriptLibrary::CreateStringObject(const char16* value, charcount_t length)
    {
        AssertMsg(stringTypeDynamic, "Where's stringTypeDynamic?");
        return RecyclerNew(this->GetRecycler(), JavascriptStringObject,
            Js::JavascriptString::NewWithBuffer(value, length, scriptContext), stringTypeDynamic);
    }

    JavascriptRegExp* JavascriptLibrary::CreateRegExp(UnifiedRegex::RegexPattern* pattern)
    {
        AssertMsg(regexType, "Where's regexType?");
        return RecyclerNew(this->GetRecycler(), JavascriptRegExp, pattern, regexType);
    }

    JavascriptArrayIterator* JavascriptLibrary::CreateArrayIterator(Var iterable, JavascriptArrayIteratorKind kind)
    {
        AssertMsg(arrayIteratorType, "Where's arrayIteratorType");
        return RecyclerNew(this->GetRecycler(), JavascriptArrayIterator, arrayIteratorType, iterable, kind);
    }

    JavascriptMapIterator* JavascriptLibrary::CreateMapIterator(JavascriptMap* map, JavascriptMapIteratorKind kind)
    {
        AssertMsg(mapIteratorType, "Where's mapIteratorType");
        return RecyclerNew(this->GetRecycler(), JavascriptMapIterator, mapIteratorType, map, kind);
    }

    JavascriptSetIterator* JavascriptLibrary::CreateSetIterator(JavascriptSet* set, JavascriptSetIteratorKind kind)
    {
        AssertMsg(setIteratorType, "Where's setIteratorType");
        return RecyclerNew(this->GetRecycler(), JavascriptSetIterator, setIteratorType, set, kind);
    }

    JavascriptStringIterator* JavascriptLibrary::CreateStringIterator(JavascriptString* string)
    {
        AssertMsg(stringIteratorType, "Where's stringIteratorType");
        return RecyclerNew(this->GetRecycler(), JavascriptStringIterator, stringIteratorType, string);
    }

    DynamicObject* JavascriptLibrary::CreateIteratorResultObject(Var value, Var done)
    {
        auto* iteratorResult = DynamicObject::New(GetRecycler(), iteratorResultType);

        iteratorResult->SetSlot(SetSlotArguments(Js::PropertyIds::value, 0, value));
        iteratorResult->SetSlot(SetSlotArguments(Js::PropertyIds::done, 1, done));

        return iteratorResult;
    }

    DynamicObject* JavascriptLibrary::CreateIteratorResultObject(Var value, bool done)
    {
        return CreateIteratorResultObject(value, done ? GetTrue() : GetFalse());
    }

    DynamicObject* JavascriptLibrary::CreateIteratorResultObjectDone()
    {
        return CreateIteratorResultObject(GetUndefined(), GetTrue());
    }

    JavascriptListIterator* JavascriptLibrary::CreateListIterator(ListForListIterator* list)
    {
        JavascriptListIterator* iterator = RecyclerNew(this->GetRecycler(), JavascriptListIterator, listIteratorType, list);
        JavascriptFunction* nextFunction = DefaultCreateFunction(&JavascriptListIterator::EntryInfo::Next, 0, nullptr, nullptr, PropertyIds::next);
        AssertOrFailFast(VarIsCorrectType(nextFunction));
        JavascriptOperators::SetProperty(iterator, iterator, PropertyIds::next, nextFunction, GetScriptContext(), PropertyOperation_None);
        return iterator;
    }

    RecyclableObject* JavascriptLibrary::CreateThrowErrorObject(JavascriptError* error)
    {
        return ThrowErrorObject::New(this->throwErrorObjectType, error, this->GetRecycler());
    }

#if ENABLE_COPYONACCESS_ARRAY
    bool JavascriptLibrary::IsCopyOnAccessArrayCallSite(JavascriptLibrary *lib, ArrayCallSiteInfo *arrayInfo, uint32 length)
    {
        return
            lib->cacheForCopyOnAccessArraySegments
            && lib->cacheForCopyOnAccessArraySegments->IsNotOverHardLimit()
            && (
                PHASE_FORCE1(CopyOnAccessArrayPhase)  // -force:copyonaccessarray is only restricted by hard limit of the segment cache
                || (
                    !arrayInfo->isNotCopyOnAccessArray        // from profile
                    && !PHASE_OFF1(CopyOnAccessArrayPhase)
                    && lib->cacheForCopyOnAccessArraySegments->IsNotFull()  // cache size soft limit through -copyonaccessarraysegmentcachesize:<number>
                    && length <= (uint32)CONFIG_FLAG(MaxCopyOnAccessArrayLength)  // -maxcopyonaccessarraylength:<number>
                    && length >= (uint32)CONFIG_FLAG(MinCopyOnAccessArrayLength)  // -mincopyonaccessarraylength:<number>
                    )
                )
#if ENABLE_TTD
            && !lib->GetScriptContext()->GetThreadContext()->IsRuntimeInTTDMode()
#endif
            ;
    }

    bool JavascriptLibrary::IsCachedCopyOnAccessArrayCallSite(const JavascriptLibrary *lib, ArrayCallSiteInfo *arrayInfo)
    {
        return lib->cacheForCopyOnAccessArraySegments
            && lib->cacheForCopyOnAccessArraySegments->IsValidIndex(arrayInfo->copyOnAccessArrayCacheIndex);
    }
#endif

    // static
    bool JavascriptLibrary::IsTypedArrayConstructor(Var constructor, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();
        return constructor == library->GetInt8ArrayConstructor()
            || constructor == library->GetUint8ArrayConstructor()
            || constructor == library->GetUint8ClampedArrayConstructor()
            || constructor == library->GetInt16ArrayConstructor()
            || constructor == library->GetUint16ArrayConstructor()
            || constructor == library->GetInt32ArrayConstructor()
            || constructor == library->GetUint32ArrayConstructor()
            || constructor == library->GetFloat32ArrayConstructor()
            || constructor == library->GetFloat64ArrayConstructor();
    }

    Field(JavascriptFunction*)* JavascriptLibrary::GetBuiltinFunctions()
    {
        AssertMsg(this->builtinFunctions, "builtinFunctions table must've been initialized as part of library initialization!");
        return this->builtinFunctions;
    }

    void JavascriptLibrary::SetIsPRNGSeeded(bool val)
    {
        this->isPRNGSeeded = val;
#if ENABLE_NATIVE_CODEGEN
        if (JITManager::GetJITManager()->IsOOPJITEnabled() && JITManager::GetJITManager()->IsConnected())
        {
            PSCRIPTCONTEXT_HANDLE remoteScriptContext = this->scriptContext->GetRemoteScriptAddr(false);
            if (remoteScriptContext)
            {
                HRESULT hr = JITManager::GetJITManager()->SetIsPRNGSeeded(remoteScriptContext, val);
                JITManager::HandleServerCallResult(hr, RemoteCallType::StateUpdate);
            }
        }
#endif
    }
    INT_PTR* JavascriptLibrary::GetVTableAddresses()
    {
        AssertMsg(this->vtableAddresses, "vtableAddresses table must've been initialized as part of library initialization!");
        return this->vtableAddresses;
    }

#if ENABLE_NATIVE_CODEGEN
    //static
    BuiltinFunction JavascriptLibrary::GetBuiltInInlineCandidateId(OpCode opCode)
    {
        switch (opCode)
        {
        case OpCode::InlineMathAcos:
            return BuiltinFunction::Math_Acos;

        case OpCode::InlineMathAsin:
            return BuiltinFunction::Math_Asin;

        case OpCode::InlineMathAtan:
            return BuiltinFunction::Math_Atan;

        case OpCode::InlineMathAtan2:
            return BuiltinFunction::Math_Atan2;

        case OpCode::InlineMathCos:
            return BuiltinFunction::Math_Cos;

        case OpCode::InlineMathExp:
            return BuiltinFunction::Math_Exp;

        case OpCode::InlineMathLog:
            return BuiltinFunction::Math_Log;

        case OpCode::InlineMathPow:
            return BuiltinFunction::Math_Pow;

        case OpCode::InlineMathSin:
            return BuiltinFunction::Math_Sin;

        case OpCode::InlineMathSqrt:
            return BuiltinFunction::Math_Sqrt;

        case OpCode::InlineMathTan:
            return BuiltinFunction::Math_Tan;

        case OpCode::InlineMathAbs:
            return BuiltinFunction::Math_Abs;

        case OpCode::InlineMathClz:
            return BuiltinFunction::Math_Clz32;

        case OpCode::InlineMathCeil:
            return BuiltinFunction::Math_Ceil;

        case OpCode::InlineMathFloor:
            return BuiltinFunction::Math_Floor;

        case OpCode::InlineMathMax:
            return BuiltinFunction::Math_Max;

        case OpCode::InlineMathMin:
            return BuiltinFunction::Math_Min;

        case OpCode::InlineMathImul:
            return BuiltinFunction::Math_Imul;

        case OpCode::InlineMathRandom:
            return BuiltinFunction::Math_Random;

        case OpCode::InlineMathRound:
            return BuiltinFunction::Math_Round;

        case OpCode::InlineMathFround:
            return BuiltinFunction::Math_Fround;

        case OpCode::InlineStringCharAt:
            return BuiltinFunction::JavascriptString_CharAt;

        case OpCode::InlineStringCharCodeAt:
            return BuiltinFunction::JavascriptString_CharCodeAt;

        case OpCode::InlineStringCodePointAt:
            return BuiltinFunction::JavascriptString_CodePointAt;

        case OpCode::InlineArrayPop:
            return BuiltinFunction::JavascriptArray_Pop;

        case OpCode::InlineArrayPush:
            return BuiltinFunction::JavascriptArray_Push;

        case OpCode::InlineFunctionApply:
            return BuiltinFunction::JavascriptFunction_Apply;

        case OpCode::InlineFunctionCall:
            return BuiltinFunction::JavascriptFunction_Call;

        case OpCode::InlineRegExpExec:
            return BuiltinFunction::JavascriptRegExp_Exec;
        }

        return BuiltinFunction::None;
    }
#endif

    // Parses given flags and arg kind (dst or src1, or src2) returns the type the arg must be type-specialized to.
    // static
    BuiltInArgSpecializationType JavascriptLibrary::GetBuiltInArgType(BuiltInFlags flags, BuiltInArgShift argKind)
    {
        Assert(argKind == BuiltInArgShift::BIAS_Dst || BuiltInArgShift::BIAS_Src1 || BuiltInArgShift::BIAS_Src2);

        BuiltInArgSpecializationType type = static_cast<BuiltInArgSpecializationType>(
            (flags >> argKind) &              // Shift-out everything to the right of start of interesting area.
            ((1 << Js::BIAS_ArgSize) - 1));   // Mask-out everything to the left of interesting area.

        return type;
    }

    ModuleRecordList* JavascriptLibrary::EnsureModuleRecordList()
    {
        if (moduleRecordList == nullptr)
        {
            moduleRecordList = RecyclerNew(recycler, ModuleRecordList, recycler);
        }
        return moduleRecordList;
    }

    SourceTextModuleRecord* JavascriptLibrary::GetModuleRecord(uint moduleId)
    {
        Assert((moduleRecordList->Count() >= 0) && (moduleId < (uint)moduleRecordList->Count()));
        if (moduleId >= (uint)moduleRecordList->Count())
        {
            Js::Throw::FatalInternalError();
        }
        return moduleRecordList->Item(moduleId);
    }

    void JavascriptLibrary::BindReference(void * addr)
    {
        // The last void* is the linklist connecting to next block.
        if (bindRefChunkCurrent == bindRefChunkEnd)
        {
            Field(void*)* tmpBindRefChunk = RecyclerNewArrayZ(recycler,
                Field(void*), HeapConstants::ObjectGranularity / sizeof(void *));
            // reserve the last void* as the linklist node.
            bindRefChunkEnd = tmpBindRefChunk + (HeapConstants::ObjectGranularity / sizeof(void *) -1 );
            if (bindRefChunkBegin == nullptr)
            {
                bindRefChunkCurrent = tmpBindRefChunk;
                bindRefChunkBegin = bindRefChunkCurrent;
            }
            else
            {
                *bindRefChunkCurrent = tmpBindRefChunk;
                bindRefChunkCurrent = tmpBindRefChunk;
            }
        }
        Assert((bindRefChunkCurrent+1) <= bindRefChunkEnd);
        *bindRefChunkCurrent = addr;
        bindRefChunkCurrent++;
    }

    void JavascriptLibrary::CleanupForClose()
    {
        bindRefChunkCurrent = nullptr;
        bindRefChunkEnd = nullptr;
    }

    void JavascriptLibrary::BeginDynamicFunctionReferences()
    {
        if (this->dynamicFunctionReference == nullptr)
        {
            this->dynamicFunctionReference = RecyclerNew(this->recycler, FunctionReferenceList, this->recycler);
            this->dynamicFunctionReferenceDepth = 0;
        }

        this->dynamicFunctionReferenceDepth++;
    }

    void JavascriptLibrary::EndDynamicFunctionReferences()
    {
        Assert(this->dynamicFunctionReference != nullptr);

        this->dynamicFunctionReferenceDepth--;

        if (this->dynamicFunctionReferenceDepth == 0)
        {
            this->dynamicFunctionReference->Clear();
        }
    }

    void JavascriptLibrary::RegisterDynamicFunctionReference(FunctionProxy* func)
    {
        Assert(this->dynamicFunctionReferenceDepth > 0);
        this->dynamicFunctionReference->Push(func);
    }

#ifdef ENABLE_SCRIPT_PROFILING
    // Register for profiler
#define DEFINE_OBJECT_NAME(object) const char16 *pwszObjectName = _u(#object);

#define REGISTER_OBJECT(object)\
    if (FAILED(hr = this->ProfilerRegister##object()))\
    {\
    return hr; \
    }\

#define REG_LIB_FUNC_CORE(pwszObjectName, pwszFunctionName, functionPropertyId, entryPoint)\
    if (FAILED(hr = this->GetScriptContext()->RegisterLibraryFunction(pwszObjectName, pwszFunctionName, functionPropertyId, entryPoint)))\
    {\
    return hr; \
    }\

#define REG_OBJECTS_DYNAMIC_LIB_FUNC(pwszFunctionName, nFuncNameLen, entryPoint) {\
    Js::PropertyRecord const * propRecord; \
    this->GetScriptContext()->GetOrAddPropertyRecord(pwszFunctionName, nFuncNameLen, &propRecord); \
    REG_LIB_FUNC_CORE(pwszObjectName, pwszFunctionName, propRecord->GetPropertyId(), entryPoint)\
}

#define REG_LIB_FUNC(pwszObjectName, functionPropertyId, entryPoint)\
    REG_LIB_FUNC_CORE(pwszObjectName, _u(#functionPropertyId), PropertyIds::##functionPropertyId, entryPoint)\

#define REG_OBJECTS_LIB_FUNC(functionPropertyId, entryPoint)\
    REG_LIB_FUNC(pwszObjectName, functionPropertyId, entryPoint)\

#define REG_OBJECTS_LIB_FUNC2(functionPropertyId, pwszFunctionPropertyName, entryPoint)\
    REG_LIB_FUNC_CORE(pwszObjectName, pwszFunctionPropertyName, PropertyIds::##functionPropertyId, entryPoint)\

#define REG_GLOBAL_LIB_FUNC(functionPropertyId, entryPoint)\
    REG_LIB_FUNC(NULL, functionPropertyId, entryPoint)\

#define REG_GLOBAL_CONSTRUCTOR(functionPropertyId)\
    REG_GLOBAL_LIB_FUNC(functionPropertyId, Javascript##functionPropertyId##::NewInstance)\

#define REGISTER_ERROR_OBJECT(functionPropertyId)\
    REG_GLOBAL_LIB_FUNC(functionPropertyId, JavascriptError::New##functionPropertyId##Instance)\
    REG_LIB_FUNC(_u(#functionPropertyId), toString, JavascriptError::EntryToString)\

    HRESULT JavascriptLibrary::ProfilerRegisterBuiltIns()
    {
        HRESULT hr = S_OK;

        // Register functions directly in global scope
        REG_GLOBAL_LIB_FUNC(eval, GlobalObject::EntryEval);
        REG_GLOBAL_LIB_FUNC(parseInt, GlobalObject::EntryParseInt);
        REG_GLOBAL_LIB_FUNC(parseFloat, GlobalObject::EntryParseFloat);
        REG_GLOBAL_LIB_FUNC(isNaN, GlobalObject::EntryIsNaN);
        REG_GLOBAL_LIB_FUNC(isFinite, GlobalObject::EntryIsFinite);
        REG_GLOBAL_LIB_FUNC(decodeURI, GlobalObject::EntryDecodeURI);
        REG_GLOBAL_LIB_FUNC(decodeURIComponent, GlobalObject::EntryDecodeURIComponent);
        REG_GLOBAL_LIB_FUNC(encodeURI, GlobalObject::EntryEncodeURI);
        REG_GLOBAL_LIB_FUNC(encodeURIComponent, GlobalObject::EntryEncodeURIComponent);
        REG_GLOBAL_LIB_FUNC(escape, GlobalObject::EntryEscape);
        REG_GLOBAL_LIB_FUNC(unescape, GlobalObject::EntryUnEscape);

        REG_GLOBAL_LIB_FUNC(CollectGarbage, GlobalObject::EntryCollectGarbage);

        // Register constructors, prototypes and objects in global
        REGISTER_OBJECT(Object);
        REGISTER_OBJECT(Array);
        REGISTER_OBJECT(Boolean);
        REGISTER_OBJECT(Date);
        REGISTER_OBJECT(Function);
        REGISTER_OBJECT(Math);
        REGISTER_OBJECT(Number);
        REGISTER_OBJECT(String);
        REGISTER_OBJECT(RegExp);
        REGISTER_OBJECT(JSON);

        REGISTER_OBJECT(Map);
        REGISTER_OBJECT(Set);
        REGISTER_OBJECT(WeakMap);
        REGISTER_OBJECT(WeakSet);

        REGISTER_OBJECT(Symbol);

        REGISTER_OBJECT(Iterator);
        REGISTER_OBJECT(ArrayIterator);
        REGISTER_OBJECT(MapIterator);
        REGISTER_OBJECT(SetIterator);
        REGISTER_OBJECT(StringIterator);

        REGISTER_OBJECT(TypedArray);

        ScriptConfiguration const& config = *(scriptContext->GetConfig());

        REGISTER_OBJECT(Promise);

        if (config.IsES6ProxyEnabled())
        {
            REGISTER_OBJECT(Proxy);
            REGISTER_OBJECT(Reflect);
        }

#ifdef IR_VIEWER
        if (Js::Configuration::Global.flags.IsEnabled(Js::IRViewerFlag))
        {
            REGISTER_OBJECT(IRViewer);
        }
#endif /* IR_VIEWER */

        // Error Constructors and prototypes
        REGISTER_ERROR_OBJECT(Error);
        REGISTER_ERROR_OBJECT(EvalError);
        REGISTER_ERROR_OBJECT(RangeError);
        REGISTER_ERROR_OBJECT(ReferenceError);
        REGISTER_ERROR_OBJECT(SyntaxError);
        REGISTER_ERROR_OBJECT(TypeError);
        REGISTER_ERROR_OBJECT(URIError);
        REGISTER_ERROR_OBJECT(AggregateError);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterObject()
    {
        HRESULT hr = S_OK;

        REG_GLOBAL_CONSTRUCTOR(Object);

        DEFINE_OBJECT_NAME(Object);

        REG_OBJECTS_LIB_FUNC(defineProperty, JavascriptObject::EntryDefineProperty);
        REG_OBJECTS_LIB_FUNC(getOwnPropertyDescriptor, JavascriptObject::EntryGetOwnPropertyDescriptor);
        if (scriptContext->GetConfig()->IsESObjectGetOwnPropertyDescriptorsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(getOwnPropertyDescriptors, JavascriptObject::EntryGetOwnPropertyDescriptors);
        }

        REG_OBJECTS_LIB_FUNC(defineProperties, JavascriptObject::EntryDefineProperties);
        REG_OBJECTS_LIB_FUNC(create, JavascriptObject::EntryCreate);
        REG_OBJECTS_LIB_FUNC(seal, JavascriptObject::EntrySeal);
        REG_OBJECTS_LIB_FUNC(freeze, JavascriptObject::EntryFreeze);
        REG_OBJECTS_LIB_FUNC(preventExtensions, JavascriptObject::EntryPreventExtensions);
        REG_OBJECTS_LIB_FUNC(isSealed, JavascriptObject::EntryIsSealed);
        REG_OBJECTS_LIB_FUNC(isFrozen, JavascriptObject::EntryIsFrozen);
        REG_OBJECTS_LIB_FUNC(isExtensible, JavascriptObject::EntryIsExtensible);

        REG_OBJECTS_LIB_FUNC(getPrototypeOf, JavascriptObject::EntryGetPrototypeOf);
        REG_OBJECTS_LIB_FUNC(keys, JavascriptObject::EntryKeys);
        REG_OBJECTS_LIB_FUNC(getOwnPropertyNames, JavascriptObject::EntryGetOwnPropertyNames);

        REG_OBJECTS_LIB_FUNC(setPrototypeOf, JavascriptObject::EntrySetPrototypeOf);

        REG_OBJECTS_LIB_FUNC(getOwnPropertySymbols, JavascriptObject::EntryGetOwnPropertySymbols);

        REG_OBJECTS_LIB_FUNC(hasOwn, JavascriptObject::EntryHasOwn);
        REG_OBJECTS_LIB_FUNC(hasOwnProperty, JavascriptObject::EntryHasOwnProperty);
        REG_OBJECTS_LIB_FUNC(propertyIsEnumerable, JavascriptObject::EntryPropertyIsEnumerable);
        REG_OBJECTS_LIB_FUNC(isPrototypeOf, JavascriptObject::EntryIsPrototypeOf);
        REG_OBJECTS_LIB_FUNC(toLocaleString, JavascriptObject::EntryToLocaleString);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptObject::EntryToString);
        REG_OBJECTS_LIB_FUNC(valueOf, JavascriptObject::EntryValueOf);

        REG_OBJECTS_LIB_FUNC(__defineGetter__, JavascriptObject::EntryDefineGetter);
        REG_OBJECTS_LIB_FUNC(__defineSetter__, JavascriptObject::EntryDefineSetter);

        ScriptConfiguration const& config = *(scriptContext->GetConfig());
        if (config.IsES6ObjectExtensionsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(is, JavascriptObject::EntryIs);
            REG_OBJECTS_LIB_FUNC(assign, JavascriptObject::EntryAssign);
        }

        if (config.IsES7ValuesEntriesEnabled())
        {
            REG_OBJECTS_LIB_FUNC(values, JavascriptObject::EntryValues);
            REG_OBJECTS_LIB_FUNC(entries, JavascriptObject::EntryEntries);
        }

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterArray()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Array);

        DEFINE_OBJECT_NAME(Array);

        REG_OBJECTS_LIB_FUNC(isArray, JavascriptArray::EntryIsArray);
        REG_OBJECTS_LIB_FUNC(concat, JavascriptArray::EntryConcat);
        REG_OBJECTS_LIB_FUNC(join, JavascriptArray::EntryJoin);
        REG_OBJECTS_LIB_FUNC(pop, JavascriptArray::EntryPop);
        REG_OBJECTS_LIB_FUNC(push, JavascriptArray::EntryPush);
        REG_OBJECTS_LIB_FUNC(reverse, JavascriptArray::EntryReverse);
        REG_OBJECTS_LIB_FUNC(shift, JavascriptArray::EntryShift);
        REG_OBJECTS_LIB_FUNC(slice, JavascriptArray::EntrySlice);
        REG_OBJECTS_LIB_FUNC(sort, JavascriptArray::EntrySort);
        REG_OBJECTS_LIB_FUNC(splice, JavascriptArray::EntrySplice);
        REG_OBJECTS_LIB_FUNC(toLocaleString, JavascriptArray::EntryToLocaleString);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptArray::EntryToString);
        REG_OBJECTS_LIB_FUNC(unshift, JavascriptArray::EntryUnshift);
        REG_OBJECTS_LIB_FUNC(indexOf, JavascriptArray::EntryIndexOf);
        REG_OBJECTS_LIB_FUNC(every, JavascriptArray::EntryEvery);
        REG_OBJECTS_LIB_FUNC(filter, JavascriptArray::EntryFilter);
        REG_OBJECTS_LIB_FUNC(forEach, JavascriptArray::EntryForEach);
        REG_OBJECTS_LIB_FUNC(lastIndexOf, JavascriptArray::EntryLastIndexOf);
        REG_OBJECTS_LIB_FUNC(map, JavascriptArray::EntryMap);
        REG_OBJECTS_LIB_FUNC(reduce, JavascriptArray::EntryReduce);
        REG_OBJECTS_LIB_FUNC(reduceRight, JavascriptArray::EntryReduceRight);
        REG_OBJECTS_LIB_FUNC(some, JavascriptArray::EntrySome);

        ScriptConfiguration const& config = *(scriptContext->GetConfig());
        if (config.IsES6StringExtensionsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(find, JavascriptArray::EntryFind);
            REG_OBJECTS_LIB_FUNC(findIndex, JavascriptArray::EntryFindIndex);
        }

        // All iterator constructor functions are implemeted in Js when JsBuiltIns are enabled and their entrypoint will be the same
        // The profiler cannot distinguish between them
        if (!scriptContext->IsJsBuiltInEnabled())
        {
            REG_OBJECTS_LIB_FUNC(entries, JavascriptArray::EntryEntries)
            REG_OBJECTS_LIB_FUNC(keys, JavascriptArray::EntryKeys)
            REG_OBJECTS_LIB_FUNC(values, JavascriptArray::EntryValues)
                // _symbolIterator is just an alias for values on Array.prototype so do not register it as its own function
        }

        REG_OBJECTS_LIB_FUNC(fill, JavascriptArray::EntryFill)
        REG_OBJECTS_LIB_FUNC(copyWithin, JavascriptArray::EntryCopyWithin)
        REG_OBJECTS_LIB_FUNC(from, JavascriptArray::EntryFrom);
        REG_OBJECTS_LIB_FUNC(of, JavascriptArray::EntryOf);

        REG_OBJECTS_LIB_FUNC(includes, JavascriptArray::EntryIncludes);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterBoolean()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Boolean);

        DEFINE_OBJECT_NAME(Boolean);

        REG_OBJECTS_LIB_FUNC(valueOf, JavascriptBoolean::EntryValueOf);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptBoolean::EntryToString);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterBigInt()
    {
        if (!scriptContext->GetConfig()->IsESBigIntEnabled())
        {
            return E_FAIL;
        }
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(BigInt);

        //DEFINE_OBJECT_NAME(BigInt);
        
        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterDate()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Date);

        DEFINE_OBJECT_NAME(Date);

        REG_OBJECTS_LIB_FUNC(parse, JavascriptDate::EntryParse);
        REG_OBJECTS_LIB_FUNC(now, JavascriptDate::EntryNow);
        REG_OBJECTS_LIB_FUNC(UTC, JavascriptDate::EntryUTC);

        REG_OBJECTS_LIB_FUNC(getDate, JavascriptDate::EntryGetDate);
        REG_OBJECTS_LIB_FUNC(getDay, JavascriptDate::EntryGetDay);
        REG_OBJECTS_LIB_FUNC(getFullYear, JavascriptDate::EntryGetFullYear);
        REG_OBJECTS_LIB_FUNC(getHours, JavascriptDate::EntryGetHours);
        REG_OBJECTS_LIB_FUNC(getMilliseconds, JavascriptDate::EntryGetMilliseconds);
        REG_OBJECTS_LIB_FUNC(getMinutes, JavascriptDate::EntryGetMinutes);
        REG_OBJECTS_LIB_FUNC(getMonth, JavascriptDate::EntryGetMonth);
        REG_OBJECTS_LIB_FUNC(getSeconds, JavascriptDate::EntryGetSeconds);
        REG_OBJECTS_LIB_FUNC(getTime, JavascriptDate::EntryGetTime);
        REG_OBJECTS_LIB_FUNC(getTimezoneOffset, JavascriptDate::EntryGetTimezoneOffset);
        REG_OBJECTS_LIB_FUNC(getUTCDate, JavascriptDate::EntryGetUTCDate);
        REG_OBJECTS_LIB_FUNC(getUTCDay, JavascriptDate::EntryGetUTCDay);
        REG_OBJECTS_LIB_FUNC(getUTCFullYear, JavascriptDate::EntryGetUTCFullYear);
        REG_OBJECTS_LIB_FUNC(getUTCHours, JavascriptDate::EntryGetUTCHours);
        REG_OBJECTS_LIB_FUNC(getUTCMilliseconds, JavascriptDate::EntryGetUTCMilliseconds);
        REG_OBJECTS_LIB_FUNC(getUTCMinutes, JavascriptDate::EntryGetUTCMinutes);
        REG_OBJECTS_LIB_FUNC(getUTCMonth, JavascriptDate::EntryGetUTCMonth);
        REG_OBJECTS_LIB_FUNC(getUTCSeconds, JavascriptDate::EntryGetUTCSeconds);

        ScriptConfiguration const& config = *(scriptContext->GetConfig());
        REG_OBJECTS_LIB_FUNC(getYear, JavascriptDate::EntryGetYear);
        REG_OBJECTS_LIB_FUNC(setDate, JavascriptDate::EntrySetDate);
        REG_OBJECTS_LIB_FUNC(setFullYear, JavascriptDate::EntrySetFullYear);
        REG_OBJECTS_LIB_FUNC(setHours, JavascriptDate::EntrySetHours);
        REG_OBJECTS_LIB_FUNC(setMilliseconds, JavascriptDate::EntrySetMilliseconds);
        REG_OBJECTS_LIB_FUNC(setMinutes, JavascriptDate::EntrySetMinutes);
        REG_OBJECTS_LIB_FUNC(setMonth, JavascriptDate::EntrySetMonth);
        REG_OBJECTS_LIB_FUNC(setSeconds, JavascriptDate::EntrySetSeconds);
        REG_OBJECTS_LIB_FUNC(setTime, JavascriptDate::EntrySetTime);
        REG_OBJECTS_LIB_FUNC(setUTCDate, JavascriptDate::EntrySetUTCDate);
        REG_OBJECTS_LIB_FUNC(setUTCFullYear, JavascriptDate::EntrySetUTCFullYear);
        REG_OBJECTS_LIB_FUNC(setUTCHours, JavascriptDate::EntrySetUTCHours);
        REG_OBJECTS_LIB_FUNC(setUTCMilliseconds, JavascriptDate::EntrySetUTCMilliseconds);
        REG_OBJECTS_LIB_FUNC(setUTCMinutes, JavascriptDate::EntrySetUTCMinutes);
        REG_OBJECTS_LIB_FUNC(setUTCMonth, JavascriptDate::EntrySetUTCMonth);
        REG_OBJECTS_LIB_FUNC(setUTCSeconds, JavascriptDate::EntrySetUTCSeconds);
        REG_OBJECTS_LIB_FUNC(setYear, JavascriptDate::EntrySetYear);
        REG_OBJECTS_LIB_FUNC(toDateString, JavascriptDate::EntryToDateString);
        REG_OBJECTS_LIB_FUNC(toISOString, JavascriptDate::EntryToISOString);
        REG_OBJECTS_LIB_FUNC(toJSON, JavascriptDate::EntryToJSON);
        REG_OBJECTS_LIB_FUNC(toLocaleDateString, JavascriptDate::EntryToLocaleDateString);
        REG_OBJECTS_LIB_FUNC(toLocaleString, JavascriptDate::EntryToLocaleString);
        REG_OBJECTS_LIB_FUNC(toLocaleTimeString, JavascriptDate::EntryToLocaleTimeString);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptDate::EntryToString);
        REG_OBJECTS_LIB_FUNC(toTimeString, JavascriptDate::EntryToTimeString);
        REG_OBJECTS_LIB_FUNC(toUTCString, JavascriptDate::EntryToUTCString);
        REG_OBJECTS_LIB_FUNC(valueOf, JavascriptDate::EntryValueOf);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterFunction()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Function);

        DEFINE_OBJECT_NAME(Function);

        REG_OBJECTS_LIB_FUNC(apply, JavascriptFunction::EntryApply);
        REG_OBJECTS_LIB_FUNC(bind, JavascriptFunction::EntryBind);
        REG_OBJECTS_LIB_FUNC(call, JavascriptFunction::EntryCall);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptFunction::EntryToString);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterMath()
    {
        HRESULT hr = S_OK;

        DEFINE_OBJECT_NAME(Math);

        REG_OBJECTS_LIB_FUNC(abs, Math::Abs);
        REG_OBJECTS_LIB_FUNC(acos, Math::Acos);
        REG_OBJECTS_LIB_FUNC(asin, Math::Asin);
        REG_OBJECTS_LIB_FUNC(atan, Math::Atan);
        REG_OBJECTS_LIB_FUNC(atan2, Math::Atan2);
        REG_OBJECTS_LIB_FUNC(ceil, Math::Ceil);
        REG_OBJECTS_LIB_FUNC(cos, Math::Cos);
        REG_OBJECTS_LIB_FUNC(exp, Math::Exp);
        REG_OBJECTS_LIB_FUNC(floor, Math::Floor);
        REG_OBJECTS_LIB_FUNC(log, Math::Log);
        REG_OBJECTS_LIB_FUNC(max, Math::Max);
        REG_OBJECTS_LIB_FUNC(min, Math::Min);
        REG_OBJECTS_LIB_FUNC(pow, Math::Pow);
        REG_OBJECTS_LIB_FUNC(random, Math::Random);
        REG_OBJECTS_LIB_FUNC(round, Math::Round);
        REG_OBJECTS_LIB_FUNC(sin, Math::Sin);
        REG_OBJECTS_LIB_FUNC(sqrt, Math::Sqrt);
        REG_OBJECTS_LIB_FUNC(tan, Math::Tan);

        if (scriptContext->GetConfig()->IsES6MathExtensionsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(log10, Math::Log10);
            REG_OBJECTS_LIB_FUNC(log2, Math::Log2);
            REG_OBJECTS_LIB_FUNC(log1p, Math::Log1p);
            REG_OBJECTS_LIB_FUNC(expm1, Math::Expm1);
            REG_OBJECTS_LIB_FUNC(cosh, Math::Cosh);
            REG_OBJECTS_LIB_FUNC(sinh, Math::Sinh);
            REG_OBJECTS_LIB_FUNC(tanh, Math::Tanh);
            REG_OBJECTS_LIB_FUNC(acosh, Math::Acosh);
            REG_OBJECTS_LIB_FUNC(asinh, Math::Asinh);
            REG_OBJECTS_LIB_FUNC(atanh, Math::Atanh);
            REG_OBJECTS_LIB_FUNC(hypot, Math::Hypot);
            REG_OBJECTS_LIB_FUNC(trunc, Math::Trunc);
            REG_OBJECTS_LIB_FUNC(sign, Math::Sign);
            REG_OBJECTS_LIB_FUNC(cbrt, Math::Cbrt);
            REG_OBJECTS_LIB_FUNC(imul, Math::Imul);
            REG_OBJECTS_LIB_FUNC(clz32, Math::Clz32);
            REG_OBJECTS_LIB_FUNC(fround, Math::Fround);
        }

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterNumber()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Number);

        DEFINE_OBJECT_NAME(Number);

        if (scriptContext->GetConfig()->IsES6NumberExtensionsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(isNaN, JavascriptNumber::EntryIsNaN);
            REG_OBJECTS_LIB_FUNC(isFinite, JavascriptNumber::EntryIsFinite);
            REG_OBJECTS_LIB_FUNC(isInteger, JavascriptNumber::EntryIsInteger);
            REG_OBJECTS_LIB_FUNC(isSafeInteger, JavascriptNumber::EntryIsSafeInteger);
        }

        REG_OBJECTS_LIB_FUNC(toExponential, JavascriptNumber::EntryToExponential);
        REG_OBJECTS_LIB_FUNC(toFixed, JavascriptNumber::EntryToFixed);
        REG_OBJECTS_LIB_FUNC(toPrecision, JavascriptNumber::EntryToPrecision);
        REG_OBJECTS_LIB_FUNC(toLocaleString, JavascriptNumber::EntryToLocaleString);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptNumber::EntryToString);
        REG_OBJECTS_LIB_FUNC(valueOf, JavascriptNumber::EntryValueOf);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterString()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(String);

        DEFINE_OBJECT_NAME(String);

        REG_OBJECTS_LIB_FUNC(fromCharCode, JavascriptString::EntryFromCharCode);

        ScriptConfiguration const& config = *(scriptContext->GetConfig());
        if (config.IsES6UnicodeExtensionsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(fromCodePoint, JavascriptString::EntryFromCodePoint);
            REG_OBJECTS_LIB_FUNC(codePointAt, JavascriptString::EntryCodePointAt);
            REG_OBJECTS_LIB_FUNC(normalize, JavascriptString::EntryNormalize);
        }

        REG_OBJECTS_LIB_FUNC(indexOf, JavascriptString::EntryIndexOf);
        REG_OBJECTS_LIB_FUNC(lastIndexOf, JavascriptString::EntryLastIndexOf);
        REG_OBJECTS_LIB_FUNC(replace, JavascriptString::EntryReplace);
        REG_OBJECTS_LIB_FUNC(search, JavascriptString::EntrySearch);
        REG_OBJECTS_LIB_FUNC(slice, JavascriptString::EntrySlice);
        REG_OBJECTS_LIB_FUNC(charAt, JavascriptString::EntryCharAt);
        REG_OBJECTS_LIB_FUNC(charCodeAt, JavascriptString::EntryCharCodeAt);
        REG_OBJECTS_LIB_FUNC(concat, JavascriptString::EntryConcat);
        REG_OBJECTS_LIB_FUNC(localeCompare, JavascriptString::EntryLocaleCompare);
        REG_OBJECTS_LIB_FUNC(match, JavascriptString::EntryMatch);
        REG_OBJECTS_LIB_FUNC(split, JavascriptString::EntrySplit);
        REG_OBJECTS_LIB_FUNC(substring, JavascriptString::EntrySubstring);
        REG_OBJECTS_LIB_FUNC(substr, JavascriptString::EntrySubstr);
        REG_OBJECTS_LIB_FUNC(toLocaleLowerCase, JavascriptString::EntryToLocaleLowerCase);
        REG_OBJECTS_LIB_FUNC(toLocaleUpperCase, JavascriptString::EntryToLocaleUpperCase);
        REG_OBJECTS_LIB_FUNC(toLowerCase, JavascriptString::EntryToLowerCase);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptString::EntryToString);
        REG_OBJECTS_LIB_FUNC(toUpperCase, JavascriptString::EntryToUpperCase);
        REG_OBJECTS_LIB_FUNC(trim, JavascriptString::EntryTrim);
        REG_OBJECTS_LIB_FUNC(valueOf, JavascriptString::EntryValueOf);

            REG_OBJECTS_LIB_FUNC(anchor, JavascriptString::EntryAnchor);
            REG_OBJECTS_LIB_FUNC(big, JavascriptString::EntryBig);
            REG_OBJECTS_LIB_FUNC(blink, JavascriptString::EntryBlink);
            REG_OBJECTS_LIB_FUNC(bold, JavascriptString::EntryBold);
            REG_OBJECTS_LIB_FUNC(fixed, JavascriptString::EntryFixed);
            REG_OBJECTS_LIB_FUNC(fontcolor, JavascriptString::EntryFontColor);
            REG_OBJECTS_LIB_FUNC(fontsize, JavascriptString::EntryFontSize);
            REG_OBJECTS_LIB_FUNC(italics, JavascriptString::EntryItalics);
            REG_OBJECTS_LIB_FUNC(link, JavascriptString::EntryLink);
            REG_OBJECTS_DYNAMIC_LIB_FUNC(_u("small"), 5, JavascriptString::EntrySmall);
            REG_OBJECTS_LIB_FUNC(strike, JavascriptString::EntryStrike);
            REG_OBJECTS_LIB_FUNC(sub, JavascriptString::EntrySub);
            REG_OBJECTS_LIB_FUNC(sup, JavascriptString::EntrySup);

        if (config.IsES6StringExtensionsEnabled())
        {
            REG_OBJECTS_LIB_FUNC(repeat, JavascriptString::EntryRepeat);
            REG_OBJECTS_LIB_FUNC(startsWith, JavascriptString::EntryStartsWith);
            REG_OBJECTS_LIB_FUNC(endsWith, JavascriptString::EntryEndsWith);
            REG_OBJECTS_LIB_FUNC(includes, JavascriptString::EntryIncludes);
            REG_OBJECTS_LIB_FUNC(trimLeft, JavascriptString::EntryTrimStart);
            REG_OBJECTS_LIB_FUNC(trimRight, JavascriptString::EntryTrimEnd);

        }

        REG_OBJECTS_LIB_FUNC(raw, JavascriptString::EntryRaw);

        REG_OBJECTS_LIB_FUNC2(_symbolIterator, _u("[Symbol.iterator]"), JavascriptString::EntrySymbolIterator);

        REG_OBJECTS_LIB_FUNC(padStart, JavascriptString::EntryPadStart);
        REG_OBJECTS_LIB_FUNC(padEnd, JavascriptString::EntryPadEnd);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterRegExp()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(RegExp);

        DEFINE_OBJECT_NAME(RegExp);

        REG_OBJECTS_LIB_FUNC(exec, JavascriptRegExp::EntryExec);
        REG_OBJECTS_LIB_FUNC(test, JavascriptRegExp::EntryTest);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptRegExp::EntryToString);
        // Note: This is deprecated
        REG_OBJECTS_LIB_FUNC(compile, JavascriptRegExp::EntryCompile);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterJSON()
    {
        HRESULT hr = S_OK;

        DEFINE_OBJECT_NAME(JSON);

        REG_OBJECTS_LIB_FUNC(stringify, JSON::Stringify);
        REG_OBJECTS_LIB_FUNC(parse, JSON::Parse);
        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterAtomics()
    {
        HRESULT hr = S_OK;

        DEFINE_OBJECT_NAME(Atomics);

        REG_OBJECTS_LIB_FUNC(add, AtomicsObject::EntryAdd);
        REG_OBJECTS_LIB_FUNC(and_, AtomicsObject::EntryAnd);
        REG_OBJECTS_LIB_FUNC(compareExchange, AtomicsObject::EntryCompareExchange);
        REG_OBJECTS_LIB_FUNC(exchange, AtomicsObject::EntryExchange);
        REG_OBJECTS_LIB_FUNC(isLockFree, AtomicsObject::EntryIsLockFree);
        REG_OBJECTS_LIB_FUNC(load, AtomicsObject::EntryLoad);
        REG_OBJECTS_LIB_FUNC(or_, AtomicsObject::EntryOr);
        REG_OBJECTS_LIB_FUNC(store, AtomicsObject::EntryStore);
        REG_OBJECTS_LIB_FUNC(sub, AtomicsObject::EntrySub);
        REG_OBJECTS_LIB_FUNC(wait, AtomicsObject::EntryWait);
        REG_OBJECTS_LIB_FUNC(notify, AtomicsObject::EntryNotify);
        REG_OBJECTS_LIB_FUNC(xor_, AtomicsObject::EntryXor);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterWeakMap()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(WeakMap);

        DEFINE_OBJECT_NAME(WeakMap);

        REG_OBJECTS_LIB_FUNC2(delete_, _u("delete"), JavascriptWeakMap::EntryDelete);
        REG_OBJECTS_LIB_FUNC(get, JavascriptWeakMap::EntryGet);
        REG_OBJECTS_LIB_FUNC(has, JavascriptWeakMap::EntryHas);
        REG_OBJECTS_LIB_FUNC(set, JavascriptWeakMap::EntrySet);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterWeakSet()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(WeakSet);

        DEFINE_OBJECT_NAME(WeakSet);

        REG_OBJECTS_LIB_FUNC(add, JavascriptWeakSet::EntryAdd);
        REG_OBJECTS_LIB_FUNC2(delete_, _u("delete"), JavascriptWeakSet::EntryDelete);
        REG_OBJECTS_LIB_FUNC(has, JavascriptWeakSet::EntryHas);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterMap()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Map);

        DEFINE_OBJECT_NAME(Map);

        REG_OBJECTS_LIB_FUNC(clear, JavascriptMap::EntryClear);
        REG_OBJECTS_LIB_FUNC2(delete_, _u("delete"), JavascriptMap::EntryDelete);
        REG_OBJECTS_LIB_FUNC(forEach, JavascriptMap::EntryForEach);
        REG_OBJECTS_LIB_FUNC(get, JavascriptMap::EntryGet);
        REG_OBJECTS_LIB_FUNC(has, JavascriptMap::EntryHas);
        REG_OBJECTS_LIB_FUNC(set, JavascriptMap::EntrySet);

        REG_OBJECTS_LIB_FUNC(entries, JavascriptMap::EntryEntries);
        REG_OBJECTS_LIB_FUNC(keys, JavascriptMap::EntryKeys);
        REG_OBJECTS_LIB_FUNC(values, JavascriptMap::EntryValues);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterSet()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Set);

        DEFINE_OBJECT_NAME(Set);

        REG_OBJECTS_LIB_FUNC(add, JavascriptSet::EntryAdd);
        REG_OBJECTS_LIB_FUNC(clear, JavascriptSet::EntryClear);
        REG_OBJECTS_LIB_FUNC2(delete_, _u("delete"), JavascriptSet::EntryDelete);
        REG_OBJECTS_LIB_FUNC(forEach, JavascriptSet::EntryForEach);
        REG_OBJECTS_LIB_FUNC(has, JavascriptSet::EntryHas);

        REG_OBJECTS_LIB_FUNC(entries, JavascriptSet::EntryEntries);
        REG_OBJECTS_LIB_FUNC(values, JavascriptSet::EntryValues);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterSymbol()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Symbol);

        DEFINE_OBJECT_NAME(Symbol);

        REG_OBJECTS_LIB_FUNC(valueOf, JavascriptSymbol::EntryValueOf);
        REG_OBJECTS_LIB_FUNC(toString, JavascriptSymbol::EntryToString);
        REG_OBJECTS_LIB_FUNC2(for_, _u("for"), JavascriptSymbol::EntryFor);
        REG_OBJECTS_LIB_FUNC(keyFor, JavascriptSymbol::EntryKeyFor);
        REG_OBJECTS_LIB_FUNC(description, JavascriptSymbol::EntryDescription);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterIterator()
    {
        HRESULT hr = S_OK;
        // Array Iterator has no global constructor

        DEFINE_OBJECT_NAME(Iterator);

        REG_OBJECTS_LIB_FUNC2(_symbolIterator, _u("[Symbol.iterator]"), JavascriptIterator::EntrySymbolIterator);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterArrayIterator()
    {
        HRESULT hr = S_OK;
        // Array Iterator has no global constructor
#if defined(ENABLE_JS_BUILTINS)
        if (!scriptContext->IsJsBuiltInEnabled())
        {
            DEFINE_OBJECT_NAME(Array Iterator);

            REG_OBJECTS_LIB_FUNC(next, JavascriptArrayIterator::EntryNext);
        }
#endif
        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterMapIterator()
    {
        HRESULT hr = S_OK;
        // Map Iterator has no global constructor

        DEFINE_OBJECT_NAME(Map Iterator);

        REG_OBJECTS_LIB_FUNC(next, JavascriptMapIterator::EntryNext);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterSetIterator()
    {
        HRESULT hr = S_OK;
        // Set Iterator has no global constructor

        DEFINE_OBJECT_NAME(Set Iterator);

        REG_OBJECTS_LIB_FUNC(next, JavascriptSetIterator::EntryNext);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterStringIterator()
    {
        HRESULT hr = S_OK;
        // String Iterator has no global constructor

        DEFINE_OBJECT_NAME(String Iterator);

        REG_OBJECTS_LIB_FUNC(next, JavascriptStringIterator::EntryNext);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterTypedArray()
    {
        HRESULT hr = S_OK;
        // TypedArray has no named global constructor

        DEFINE_OBJECT_NAME(TypedArray);

        REG_OBJECTS_LIB_FUNC(from, TypedArrayBase::EntryFrom);
        REG_OBJECTS_LIB_FUNC(of, TypedArrayBase::EntryOf);
        REG_OBJECTS_LIB_FUNC(set, TypedArrayBase::EntrySet);
        REG_OBJECTS_LIB_FUNC(subarray, TypedArrayBase::EntrySubarray);
        REG_OBJECTS_LIB_FUNC(copyWithin, TypedArrayBase::EntryCopyWithin);
        REG_OBJECTS_LIB_FUNC(every, TypedArrayBase::EntryEvery);
        REG_OBJECTS_LIB_FUNC(fill, TypedArrayBase::EntryFill);
        REG_OBJECTS_LIB_FUNC(filter, TypedArrayBase::EntryFilter);
        REG_OBJECTS_LIB_FUNC(find, TypedArrayBase::EntryFind);
        REG_OBJECTS_LIB_FUNC(findIndex, TypedArrayBase::EntryFindIndex);
        REG_OBJECTS_LIB_FUNC(forEach, TypedArrayBase::EntryForEach);
        REG_OBJECTS_LIB_FUNC(indexOf, TypedArrayBase::EntryIndexOf);
        REG_OBJECTS_LIB_FUNC(join, TypedArrayBase::EntryJoin);
        REG_OBJECTS_LIB_FUNC(lastIndexOf, TypedArrayBase::EntryLastIndexOf);
        REG_OBJECTS_LIB_FUNC(map, TypedArrayBase::EntryMap);
        REG_OBJECTS_LIB_FUNC(reduce, TypedArrayBase::EntryReduce);
        REG_OBJECTS_LIB_FUNC(reduceRight, TypedArrayBase::EntryReduceRight);
        REG_OBJECTS_LIB_FUNC(reverse, TypedArrayBase::EntryReverse);
        REG_OBJECTS_LIB_FUNC(slice, TypedArrayBase::EntrySlice);
        REG_OBJECTS_LIB_FUNC(some, TypedArrayBase::EntrySome);
        REG_OBJECTS_LIB_FUNC(sort, TypedArrayBase::EntrySort);
        REG_OBJECTS_LIB_FUNC(includes, TypedArrayBase::EntryIncludes);


        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterPromise()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Promise);

        DEFINE_OBJECT_NAME(Promise);

        REG_OBJECTS_LIB_FUNC(all, JavascriptPromise::EntryAll);
        REG_OBJECTS_LIB_FUNC2(catch_, _u("catch"), JavascriptPromise::EntryCatch);
        REG_OBJECTS_LIB_FUNC(race, JavascriptPromise::EntryRace);
        REG_OBJECTS_LIB_FUNC(resolve, JavascriptPromise::EntryResolve);
        REG_OBJECTS_LIB_FUNC(then, JavascriptPromise::EntryThen);

        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterProxy()
    {
        HRESULT hr = S_OK;
        REG_GLOBAL_CONSTRUCTOR(Proxy);

        DEFINE_OBJECT_NAME(Proxy);

        REG_OBJECTS_LIB_FUNC(revocable, JavascriptProxy::EntryRevocable);
        return hr;
    }

    HRESULT JavascriptLibrary::ProfilerRegisterReflect()
    {
        HRESULT hr = S_OK;
        DEFINE_OBJECT_NAME(Reflect);

        REG_OBJECTS_LIB_FUNC(defineProperty, JavascriptReflect::EntryDefineProperty);
        REG_OBJECTS_LIB_FUNC(deleteProperty, JavascriptReflect::EntryDeleteProperty);
        REG_OBJECTS_LIB_FUNC(get, JavascriptReflect::EntryGet);
        REG_OBJECTS_LIB_FUNC(getOwnPropertyDescriptor, JavascriptReflect::EntryGetOwnPropertyDescriptor);
        REG_OBJECTS_LIB_FUNC(getPrototypeOf, JavascriptReflect::EntryGetPrototypeOf);
        REG_OBJECTS_LIB_FUNC(has, JavascriptReflect::EntryHas);
        REG_OBJECTS_LIB_FUNC(isExtensible, JavascriptReflect::EntryIsExtensible);
        REG_OBJECTS_LIB_FUNC(ownKeys, JavascriptReflect::EntryOwnKeys);
        REG_OBJECTS_LIB_FUNC(preventExtensions, JavascriptReflect::EntryPreventExtensions);
        REG_OBJECTS_LIB_FUNC(set, JavascriptReflect::EntrySet);
        REG_OBJECTS_LIB_FUNC(setPrototypeOf, JavascriptReflect::EntrySetPrototypeOf);
        REG_OBJECTS_LIB_FUNC(apply, JavascriptReflect::EntryApply);
        REG_OBJECTS_LIB_FUNC(construct, JavascriptReflect::EntryConstruct);
        return hr;
    }


    HRESULT JavascriptLibrary::ProfilerRegisterGenerator()
    {
        HRESULT hr = S_OK;
        DEFINE_OBJECT_NAME(Generator);

        REG_OBJECTS_LIB_FUNC(next, JavascriptGenerator::EntryNext);
        REG_OBJECTS_LIB_FUNC(return_, JavascriptGenerator::EntryReturn);
        REG_OBJECTS_LIB_FUNC(throw_, JavascriptGenerator::EntryThrow);

        return hr;
    }
#endif // ENABLE_SCRIPT_PROFILING

#if DBG
    void JavascriptLibrary::DumpLibraryByteCode()
    {
#if defined(ENABLE_JS_BUILTINS) || defined(ENABLE_INTL_OBJECT)
        // We aren't going to be passing in a number to check range of -dump:LibInit, that will be done by Intl/Promise
        // This is just to force init Intl code if dump:LibInit has been passed
        if (CONFIG_ISENABLED(DumpFlag) && Js::Configuration::Global.flags.Dump.IsEnabled(Js::JsLibInitPhase))
        {
            for (uint i = 0; i <= MaxEngineInterfaceExtensionKind; i++)
            {
                EngineExtensionObjectBase* engineExtension = this->GetEngineInterfaceObject()->GetEngineExtension((Js::EngineInterfaceExtensionKind)i);
                if (engineExtension != nullptr && engineExtension->GetHasByteCode())
                {
                    engineExtension->DumpByteCode();
                }
            }
        }
#endif
    }
#endif
#ifdef IR_VIEWER
    HRESULT JavascriptLibrary::ProfilerRegisterIRViewer()
    {
        HRESULT hr = S_OK;

        DEFINE_OBJECT_NAME(IRViewer);

        // TODO (t-doilij) move GlobalObject::EntryParseIR to JavascriptIRViewer::EntryParseIR
        REG_LIB_FUNC_CORE(pwszObjectName, _u("parseIR"), scriptContext->GetOrAddPropertyIdTracked(_u("parseIR")), GlobalObject::EntryParseIR);
        REG_LIB_FUNC_CORE(pwszObjectName, _u("functionList"), scriptContext->GetOrAddPropertyIdTracked(_u("functionList")), GlobalObject::EntryFunctionList);
        REG_LIB_FUNC_CORE(pwszObjectName, _u("rejitFunction"), scriptContext->GetOrAddPropertyIdTracked(_u("rejitFunction")), GlobalObject::EntryRejitFunction);

        return hr;
    }
#endif /* IR_VIEWER */
}
