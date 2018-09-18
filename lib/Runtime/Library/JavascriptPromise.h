//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    struct JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper
    {
        Field(bool) alreadyResolved;
    };

    class JavascriptPromiseResolveOrRejectFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseResolveOrRejectFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseResolveOrRejectFunction);

    public:
        JavascriptPromiseResolveOrRejectFunction(DynamicType* type);
        JavascriptPromiseResolveOrRejectFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromise* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord);

        JavascriptPromise* GetPromise();
        bool IsRejectFunction();
        bool IsAlreadyResolved();
        void SetAlreadyResolved(bool is);

    private:
        Field(JavascriptPromise*) promise;
        Field(bool) isReject;
        Field(JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper*) alreadyResolvedWrapper;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseResolveOrRejectFunction>(RecyclableObject* obj);

    class JavascriptPromiseAsyncSpawnExecutorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseAsyncSpawnExecutorFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseAsyncSpawnExecutorFunction);

    public:
        JavascriptPromiseAsyncSpawnExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptGenerator* generator, Var target);

        JavascriptGenerator* GetGenerator();
        Var GetTarget();

    private:
        Field(JavascriptGenerator*) generator;
        Field(Var) target; // this

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseAsyncSpawnExecutorFunction>(RecyclableObject* obj);

    class JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction);

    public:
        JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptGenerator* generator, Var argument, Var resolve = nullptr, Var reject = nullptr, bool isReject = false);

        JavascriptGenerator* GetGenerator();
        Var GetReject();
        Var GetResolve();
        bool GetIsReject();
        Var GetArgument();

    private:
        Field(JavascriptGenerator*) generator;
        Field(Var) reject;
        Field(Var) resolve;
        Field(bool) isReject;
        Field(Var) argument;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction>(RecyclableObject* obj);

    class JavascriptPromiseCapabilitiesExecutorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseCapabilitiesExecutorFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseCapabilitiesExecutorFunction);

    public:
        JavascriptPromiseCapabilitiesExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromiseCapability* capability);

        JavascriptPromiseCapability* GetCapability();

    private:
        Field(JavascriptPromiseCapability*) capability;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseCapabilitiesExecutorFunction>(RecyclableObject* obj);

    class JavascriptPromiseResolveThenableTaskFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseResolveThenableTaskFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseResolveThenableTaskFunction);

    public:
        JavascriptPromiseResolveThenableTaskFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromise* promise, RecyclableObject* thenable, RecyclableObject* thenFunction)
            : RuntimeFunction(type, functionInfo), promise(promise), thenable(thenable), thenFunction(thenFunction)
        { }

        JavascriptPromise* GetPromise();
        RecyclableObject* GetThenable();
        RecyclableObject* GetThenFunction();


    private:
        Field(JavascriptPromise*) promise;
        Field(RecyclableObject*) thenable;
        Field(RecyclableObject*) thenFunction;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseResolveThenableTaskFunction>(RecyclableObject* obj);

    class JavascriptPromiseReactionTaskFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseReactionTaskFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseReactionTaskFunction);

    public:
        JavascriptPromiseReactionTaskFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromiseReaction* reaction, Var argument)
            : RuntimeFunction(type, functionInfo), reaction(reaction), argument(argument)
        { }

        JavascriptPromiseReaction* GetReaction();
        Var GetArgument();

    private:
        Field(JavascriptPromiseReaction*) reaction;
        Field(Var) argument;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseReactionTaskFunction>(RecyclableObject* obj);

    class JavascriptPromiseThenFinallyFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseThenFinallyFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseThenFinallyFunction);

    public:
        JavascriptPromiseThenFinallyFunction(DynamicType* type, FunctionInfo* functionInfo, RecyclableObject* OnFinally, RecyclableObject* Constructor, bool shouldThrow)
            : RuntimeFunction(type, functionInfo), OnFinally(OnFinally), Constructor(Constructor), shouldThrow(shouldThrow)
        { }

        inline bool GetShouldThrow() { return this->shouldThrow; }
        inline RecyclableObject* GetOnFinally() { return this->OnFinally; }
        inline RecyclableObject* GetConstructor() { return this->Constructor; }

    private:
        Field(RecyclableObject*) OnFinally;
        Field(RecyclableObject*) Constructor;
        Field(bool) shouldThrow;
    };

    template <> bool VarIsImpl<JavascriptPromiseThenFinallyFunction>(RecyclableObject* obj);

    class JavascriptPromiseThunkFinallyFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseThunkFinallyFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseThunkFinallyFunction);

    public:
        JavascriptPromiseThunkFinallyFunction(DynamicType* type, FunctionInfo* functionInfo, Var value, bool shouldThrow)
            : RuntimeFunction(type, functionInfo), value(value), shouldThrow(shouldThrow)
        { }

        inline bool GetShouldThrow() { return this->shouldThrow; }
        inline Var GetValue() { return this->value; }

    private:
        Field(Var) value;
        Field(bool) shouldThrow;
    };

    template <> bool VarIsImpl<JavascriptPromiseThunkFinallyFunction>(RecyclableObject* obj);

    struct JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper
    {
        Field(uint32) remainingElements;
    };

    class JavascriptPromiseAllResolveElementFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseAllResolveElementFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseAllResolveElementFunction);

    public:
        JavascriptPromiseAllResolveElementFunction(DynamicType* type);
        JavascriptPromiseAllResolveElementFunction(DynamicType* type, FunctionInfo* functionInfo, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper);

        JavascriptPromiseCapability* GetCapabilities();
        uint32 GetIndex();
        uint32 GetRemainingElements();
        JavascriptArray* GetValues();
        bool IsAlreadyCalled() const;
        void SetAlreadyCalled(const bool is);

        uint32 DecrementRemainingElements();

    private:
        Field(JavascriptPromiseCapability*) capabilities;
        Field(uint32) index;
        Field(JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper*) remainingElementsWrapper;
        Field(JavascriptArray*) values;
        Field(bool) alreadyCalled;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> bool VarIsImpl<JavascriptPromiseAllResolveElementFunction>(RecyclableObject* obj);

    class JavascriptPromiseCapability : FinalizableObject
    {
    private:
        JavascriptPromiseCapability(Var promise, Var resolve, Var reject)
            : promise(promise), resolve(resolve), reject(reject)
        { }

    public:
        static JavascriptPromiseCapability* New(Var promise, Var resolve, Var reject, ScriptContext* scriptContext);

        Var GetResolve();
        Var GetReject();
        Var GetPromise();
        void SetPromise(Var);

        void SetResolve(Var resolve);
        void SetReject(Var reject);

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

    private:
        Field(Var) promise;
        Field(Var) resolve;
        Field(Var) reject;

#if ENABLE_TTD
    public:
        //Do any additional marking that is needed for a TT snapshotable object
        void MarkVisitPtrs(TTD::SnapshotExtractor* extractor);

        //Do the extraction
        void ExtractSnapPromiseCapabilityInto(TTD::NSSnapValues::SnapPromiseCapabilityInfo* snapPromiseCapability, JsUtil::List<TTD_PTR_ID, HeapAllocator>& depOnList, TTD::SlabAllocator& alloc);
#endif
    };

    class JavascriptPromiseReaction : FinalizableObject
    {
    private:
        JavascriptPromiseReaction(JavascriptPromiseCapability* capabilities, RecyclableObject* handler)
            : capabilities(capabilities), handler(handler)
        { }

    public:
        static JavascriptPromiseReaction* New(JavascriptPromiseCapability* capabilities, RecyclableObject* handler, ScriptContext* scriptContext);

        JavascriptPromiseCapability* GetCapabilities();
        RecyclableObject* GetHandler();

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

    private:
        Field(JavascriptPromiseCapability*) capabilities;
        Field(RecyclableObject*) handler;

#if ENABLE_TTD
    public:
        //Do any additional marking that is needed for a TT snapshotable object
        void MarkVisitPtrs(TTD::SnapshotExtractor* extractor);

        //Do the extraction
        void ExtractSnapPromiseReactionInto(TTD::NSSnapValues::SnapPromiseReactionInfo* snapPromiseReaction, JsUtil::List<TTD_PTR_ID, HeapAllocator>& depOnList, TTD::SlabAllocator& alloc);
#endif
    };

    struct JavascriptPromiseReactionPair
    {
        JavascriptPromiseReaction* resolveReaction;
        JavascriptPromiseReaction* rejectReaction;
    };

    typedef SList<Js::JavascriptPromiseReactionPair, Recycler> JavascriptPromiseReactionList;

    class JavascriptPromise : public DynamicObject
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptPromise, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromise);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;

            static FunctionInfo All;
            static FunctionInfo Catch;
            static FunctionInfo Race;
            static FunctionInfo Reject;
            static FunctionInfo Resolve;
            static FunctionInfo Then;
            static FunctionInfo Finally;

            static FunctionInfo Identity;
            static FunctionInfo Thrower;

            static FunctionInfo FinallyValueFunction;
            static FunctionInfo ThenFinallyFunction;
            static FunctionInfo ResolveOrRejectFunction;
            static FunctionInfo CapabilitiesExecutorFunction;
            static FunctionInfo AllResolveElementFunction;

            static FunctionInfo GetterSymbolSpecies;
        };

        JavascriptPromise(DynamicType * type);

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryAll(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryCatch(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryRace(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReject(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryResolve(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryThen(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFinally(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryThunkFinallyFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryThenFinallyFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryCapabilitiesExecutorFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryResolveOrRejectFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReactionTaskFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryResolveThenableTaskFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIdentityFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryThrowerFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAllResolveElementFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryJavascriptPromiseAsyncSpawnExecutorFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJavascriptPromiseAsyncSpawnStepNextExecutorFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJavascriptPromiseAsyncSpawnStepThrowExecutorFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryJavascriptPromiseAsyncSpawnCallStepExecutorFunction(RecyclableObject* function, CallInfo callInfo, ...);

        static Var CreateRejectedPromise(Var resolution, ScriptContext* scriptContext, Var promiseConstructor = nullptr);
        static Var CreateResolvedPromise(Var resolution, ScriptContext* scriptContext, Var promiseConstructor = nullptr);
        static Var CreatePassThroughPromise(JavascriptPromise* sourcePromise, ScriptContext* scriptContext);
        static Var CreateThenPromise(JavascriptPromise* sourcePromise, RecyclableObject* fulfillmentHandler, RecyclableObject* rejectionHandler, ScriptContext* scriptContext);

        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        JavascriptPromiseReactionList* GetReactions();


        static JavascriptPromiseCapability* NewPromiseCapability(Var constructor, ScriptContext* scriptContext);
        static JavascriptPromiseCapability* CreatePromiseCapabilityRecord(RecyclableObject* constructor, ScriptContext* scriptContext);
        static Var TriggerPromiseReactions(JavascriptPromiseReactionList* reactions, bool isReject, Var resolution, ScriptContext* scriptContext);
        static void EnqueuePromiseReactionTask(JavascriptPromiseReaction* reaction, Var resolution, ScriptContext* scriptContext);

        static void InitializePromise(JavascriptPromise* promise, JavascriptPromiseResolveOrRejectFunction** resolve, JavascriptPromiseResolveOrRejectFunction** reject, ScriptContext* scriptContext);
        static Var TryCallResolveOrRejectHandler(Var handler, Var value, ScriptContext* scriptContext);
        static Var TryRejectWithExceptionObject(JavascriptExceptionObject* exceptionObject, Var handler, ScriptContext* scriptContext);

        static JavascriptPromise* CreateEnginePromise(ScriptContext *scriptContext);

        Var Resolve(Var resolution, ScriptContext* scriptContext);
        Var Reject(Var resolution, ScriptContext* scriptContext);

        enum PromiseStatus : unsigned char
        {
            PromiseStatusCode_Undefined     = 0x00,
            PromiseStatusCode_Unresolved    = 0x01,
            PromiseStatusCode_HasResolution = 0x02,
            PromiseStatusCode_HasRejection  = 0x03
        };


        bool GetIsHandled() const { return this->isHandled; }
        void SetIsHandled() { this->isHandled = true; }

        PromiseStatus GetStatus() const { return this->status; }
        void SetStatus(const PromiseStatus newStatus) { this->status = newStatus; }

        Var GetResult() const { return result; }

    protected:
        Var ResolveHelper(Var resolution, bool isRejecting, ScriptContext* scriptContext);

    protected:
        Field(Var) result;
        Field(JavascriptPromiseReactionList*) reactions;

        // we could pack status & isHandled into a single byte, but the compiler is aligning this on address-size
        // boundaries, so we don't save anything.  Leaving these separate fields for clarity
        Field(PromiseStatus) status;
        Field(bool) isHandled;

    private :
        static void AsyncSpawnStep(JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction* nextFunction, JavascriptGenerator* gen, Var resolve, Var reject);
        bool WillRejectionBeUnhandled();

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        static JavascriptPromise* InitializePromise_TTD(ScriptContext* scriptContext, uint32 status, bool isHandled, Var result, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& resolveReactions, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& rejectReactions);
#endif
    };

    template <> inline bool VarIsImpl<JavascriptPromise>(RecyclableObject* obj)
    {
        return Js::JavascriptOperators::GetTypeId(obj) == TypeIds_Promise;
    }
}
