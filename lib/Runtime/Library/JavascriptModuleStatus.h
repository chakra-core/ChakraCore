//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    enum class JavascriptModuleStatusStage {
        Invalid,
        Fetch,
        Translate,
        Instantiate
    };

    class JavascriptModuleStatusStageRecord : FinalizableObject
    {
    private:
        JavascriptModuleStatusStageRecord(JavascriptModuleStatusStage stage, JavascriptPromise* result) : stage(stage), result(result) { }

    public:
        static JavascriptModuleStatusStageRecord* New(JavascriptModuleStatusStage stage, JavascriptPromise* result, ScriptContext* scriptContext);

        JavascriptPromise* result;
        JavascriptModuleStatusStage stage;

    public:
        // Finalizable support
        virtual void Finalize(bool isShutdown)
        {
        }

        virtual void Dispose(bool isShutdown)
        {
        }

        virtual void Mark(Recycler *recycler) override
        {
            AssertMsg(false, "Mark called on object that isnt TrackableObject");
        }

#if ENABLE_TTD
    public:
        //Do any additional marking that is needed for a TT snapshotable object
        void MarkVisitPtrs(TTD::SnapshotExtractor* extractor);

        //Do the extraction 
        void ExtractSnapPromiseCapabilityInto(TTD::NSSnapValues::SnapPromiseCapabilityInfo* snapPromiseCapability, JsUtil::List<TTD_PTR_ID, HeapAllocator>& depOnList, TTD::SlabAllocator& alloc);
#endif
    };

    typedef SList<JavascriptModuleStatusStageRecord,Recycler> JavascriptModuleStatusStageRecordList;

    class JavascriptModuleStatusErrorHandlerFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptModuleStatusErrorHandlerFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptModuleStatusErrorHandlerFunction);

    public:
        JavascriptModuleStatusErrorHandlerFunction(DynamicType* type);
        JavascriptModuleStatusErrorHandlerFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptModuleStatus* entry);

        inline static bool Is(Var var);
        inline static JavascriptModuleStatusErrorHandlerFunction* FromVar(Var var);

        JavascriptModuleStatus* GetModuleStatus();

    private:
        JavascriptModuleStatus* entry;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    class JavascriptModuleStatusFulfillmentHandlerFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptModuleStatusFulfillmentHandlerFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptModuleStatusFulfillmentHandlerFunction);

    public:
        JavascriptModuleStatusFulfillmentHandlerFunction(DynamicType* type);
        JavascriptModuleStatusFulfillmentHandlerFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, Var value);

        inline static bool Is(Var var);
        inline static JavascriptModuleStatusFulfillmentHandlerFunction* FromVar(Var var);

        JavascriptModuleStatus* GetModuleStatus();
        JavascriptModuleStatusStage GetStage();
        Var GetValue();
        Var GetInstantiateSet();

        void SetInstantiateSet(Var value);

    private:
        JavascriptModuleStatus* entry;
        Var value;
        union
        {
            Var instantiateSet;
            JavascriptModuleStatusStage stage;
        };

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    class JavascriptModuleStatus : public DynamicObject
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptModuleStatus, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptModuleStatus);

        JavascriptLoader* loader;
        JavascriptString* key;
        JavascriptModuleStatusStageRecordList* pipeline;
        Var module;
        bool error;

    public:
        JavascriptModuleStatus(DynamicType* type);

        void Initialize(JavascriptLoader* loader, JavascriptString* key, Var ns, ScriptContext* scriptContext);

        static JavascriptModuleStatus* New(JavascriptLoader* loader, JavascriptString* key, Var ns, ScriptContext* scriptContext);

        static bool Is(Var aValue);
        static JavascriptModuleStatus* FromVar(Var aValue);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        static bool IsValidStageValue(Var stage, ScriptContext* scriptContext, JavascriptModuleStatusStage* result);
        static bool GetStage(JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, ScriptContext* scriptContext, JavascriptModuleStatusStageRecord** result);
        static JavascriptModuleStatusStageRecord* GetCurrentStage(JavascriptModuleStatus* entry, ScriptContext* scriptContext);
        static void UpgradeToStage(JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, ScriptContext* scriptContext);

        static Var LoadModule(JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, ScriptContext* scriptContext);

        static JavascriptPromise* RequestFetch(JavascriptModuleStatus* entry, ScriptContext* scriptContext);
        static JavascriptPromise* RequestTranslate(JavascriptModuleStatus* entry, ScriptContext* scriptContext);
        static JavascriptPromise* RequestInstantiate(JavascriptModuleStatus* entry, Var instantiateSet, ScriptContext* scriptContext);
        static JavascriptPromise* SatisfyInstance(JavascriptModuleStatus* entry, Var optionalInstance, Var source, Var instantiateSet, ScriptContext* scriptContext);

        static JavascriptModuleStatus* EnsureRegistered(JavascriptLoader* loader, Var key, ScriptContext* scriptContext);
        static Var EnsureEvaluated(JavascriptModuleStatus* entry, ScriptContext* scriptContext);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo GetterStage;
            static FunctionInfo GetterOriginalKey;
            static FunctionInfo GetterModule;
            static FunctionInfo GetterError;
            static FunctionInfo GetterDependencies;
            static FunctionInfo Load;
            static FunctionInfo Result;
            static FunctionInfo Resolve;
            static FunctionInfo Reject;

            static FunctionInfo ResolveFulfillmentHandler;
            static FunctionInfo RejectFulfillmentHandler;
            static FunctionInfo ErrorHandler;
            static FunctionInfo SatisfyInstanceWrapperFulfillmentHandler;
            static FunctionInfo PostSatisfyInstanceFulfillmentHandler;
            static FunctionInfo PostSatisfyInstanceSimpleFulfillmentHandler;
            static FunctionInfo UpgradeToStageFulfillmentHandler;
            static FunctionInfo RequestTranslateOrInstantiateFulfillmentHandler;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterStage(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterOriginalKey(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterModule(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterError(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterDependencies(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLoad(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryResult(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryResolve(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReject(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryResolveFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryRejectFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryErrorHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryPostSatisfyInstanceFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryPostSatisfyInstanceSimpleFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySatisfyInstanceWrapperFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryUpgradeToStageFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryRequestTranslateOrInstantiateFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
    };
}
