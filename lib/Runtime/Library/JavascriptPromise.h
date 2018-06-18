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

        inline static bool Is(Var var);
        inline static JavascriptPromiseResolveOrRejectFunction* FromVar(Var var);
        inline static JavascriptPromiseResolveOrRejectFunction* UnsafeFromVar(Var var);

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

    class JavascriptPromiseAsyncSpawnExecutorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseAsyncSpawnExecutorFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseAsyncSpawnExecutorFunction);

    public:
        JavascriptPromiseAsyncSpawnExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptGenerator* generator, Var target);

        inline static bool Is(Var var);
        inline static JavascriptPromiseAsyncSpawnExecutorFunction* FromVar(Var var);
        inline static JavascriptPromiseAsyncSpawnExecutorFunction* UnsafeFromVar(Var var);

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

    class JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction);

    public:
        JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptGenerator* generator, Var argument, Var resolve = nullptr, Var reject = nullptr, bool isReject = false);

        inline static bool Is(Var var);
        inline static JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction* FromVar(Var var);
        inline static JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction* UnsafeFromVar(Var var);

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

    class JavascriptPromiseCapabilitiesExecutorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseCapabilitiesExecutorFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseCapabilitiesExecutorFunction);

    public:
        JavascriptPromiseCapabilitiesExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromiseCapability* capability);

        inline static bool Is(Var var);
        inline static JavascriptPromiseCapabilitiesExecutorFunction* FromVar(Var var);
        inline static JavascriptPromiseCapabilitiesExecutorFunction* UnsafeFromVar(Var var);

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

    class JavascriptPromiseResolveThenableTaskFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseResolveThenableTaskFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseResolveThenableTaskFunction);

    public:
        JavascriptPromiseResolveThenableTaskFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromise* promise, RecyclableObject* thenable, RecyclableObject* thenFunction)
            : RuntimeFunction(type, functionInfo), promise(promise), thenable(thenable), thenFunction(thenFunction)
        { }

        inline static bool Is(Var var)
        {
            if (JavascriptFunction::Is(var))
            {
                JavascriptFunction* obj = JavascriptFunction::UnsafeFromVar(var);

                return VirtualTableInfo<JavascriptPromiseResolveThenableTaskFunction>::HasVirtualTable(obj)
                    || VirtualTableInfo<CrossSiteObject<JavascriptPromiseResolveThenableTaskFunction>>::HasVirtualTable(obj);
            }

            return false;
        }

        inline static JavascriptPromiseResolveThenableTaskFunction* FromVar(Var var)
        {
            AssertOrFailFast(JavascriptPromiseResolveThenableTaskFunction::Is(var));

            return static_cast<JavascriptPromiseResolveThenableTaskFunction*>(var);
        }

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

    class JavascriptPromiseReactionTaskFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseReactionTaskFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseReactionTaskFunction);

    public:
        JavascriptPromiseReactionTaskFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromiseReaction* reaction, Var argument)
            : RuntimeFunction(type, functionInfo), reaction(reaction), argument(argument)
        { }

        inline static bool Is(Var var)
        {
            if (JavascriptFunction::Is(var))
            {
                JavascriptFunction* obj = JavascriptFunction::UnsafeFromVar(var);

                return VirtualTableInfo<JavascriptPromiseReactionTaskFunction>::HasVirtualTable(obj)
                    || VirtualTableInfo<CrossSiteObject<JavascriptPromiseReactionTaskFunction>>::HasVirtualTable(obj);
            }

            return false;
        }

        inline static JavascriptPromiseReactionTaskFunction* FromVar(Var var)
        {
            AssertOrFailFast(JavascriptPromiseReactionTaskFunction::Is(var));

            return static_cast<JavascriptPromiseReactionTaskFunction*>(var);
        }

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

    class JavascriptPromiseThenFinallyFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseThenFinallyFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseThenFinallyFunction);

    public:
        JavascriptPromiseThenFinallyFunction(DynamicType* type, FunctionInfo* functionInfo, RecyclableObject* OnFinally, RecyclableObject* Constructor, bool shouldThrow)
            : RuntimeFunction(type, functionInfo), OnFinally(OnFinally), Constructor(Constructor), shouldThrow(shouldThrow)
        { }

        inline static bool Is(Var var)
        {
            if (JavascriptFunction::Is(var))
            {
                JavascriptFunction* obj = JavascriptFunction::UnsafeFromVar(var);

                return VirtualTableInfo<JavascriptPromiseThenFinallyFunction>::HasVirtualTable(obj)
                    || VirtualTableInfo<CrossSiteObject<JavascriptPromiseThenFinallyFunction>>::HasVirtualTable(obj);
            }

            return false;
        }

        inline static JavascriptPromiseThenFinallyFunction* FromVar(Var var)
        {
            AssertOrFailFast(JavascriptPromiseThenFinallyFunction::Is(var));

            return static_cast<JavascriptPromiseThenFinallyFunction*>(var);
        }

        inline bool GetShouldThrow() { return this->shouldThrow; }
        inline RecyclableObject* GetOnFinally() { return this->OnFinally; }
        inline RecyclableObject* GetConstructor() { return this->Constructor; }

    private:
        Field(RecyclableObject*) OnFinally;
        Field(RecyclableObject*) Constructor;
        Field(bool) shouldThrow;
    };

    class JavascriptPromiseThunkFinallyFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptPromiseThunkFinallyFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptPromiseThunkFinallyFunction);

    public:
        JavascriptPromiseThunkFinallyFunction(DynamicType* type, FunctionInfo* functionInfo, Var value, bool shouldThrow)
            : RuntimeFunction(type, functionInfo), value(value), shouldThrow(shouldThrow)
        { }

        inline static bool Is(Var var)
        {
            if (JavascriptFunction::Is(var))
            {
                JavascriptFunction* obj = JavascriptFunction::UnsafeFromVar(var);

                return VirtualTableInfo<JavascriptPromiseThunkFinallyFunction>::HasVirtualTable(obj)
                    || VirtualTableInfo<CrossSiteObject<JavascriptPromiseThunkFinallyFunction>>::HasVirtualTable(obj);
            }
            return false;
        }

        inline static JavascriptPromiseThunkFinallyFunction* FromVar(Var var)
        {
            AssertOrFailFast(JavascriptPromiseThunkFinallyFunction::Is(var));

            return static_cast<JavascriptPromiseThunkFinallyFunction*>(var);
        }

        inline bool GetShouldThrow() { return this->shouldThrow; }
        inline Var GetValue() { return this->value; }

    private:
        Field(Var) value;
        Field(bool) shouldThrow;
    };

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

        inline static bool Is(Var var);
        inline static JavascriptPromiseAllResolveElementFunction* FromVar(Var var);
        inline static JavascriptPromiseAllResolveElementFunction* UnsafeFromVar(Var var);

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

    typedef JsUtil::List<Js::JavascriptPromiseCapability*> JavascriptPromiseCapabilityList;

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

    typedef JsUtil::List<Js::JavascriptPromiseReaction*> JavascriptPromiseReactionList;

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

        static bool Is(Var aValue);
        static JavascriptPromise* FromVar(Js::Var aValue);
        static JavascriptPromise* UnsafeFromVar(Js::Var aValue);

        static Var CreateRejectedPromise(Var resolution, ScriptContext* scriptContext, Var promiseConstructor = nullptr);
        static Var CreateResolvedPromise(Var resolution, ScriptContext* scriptContext, Var promiseConstructor = nullptr);
        static Var CreatePassThroughPromise(JavascriptPromise* sourcePromise, ScriptContext* scriptContext);
        static Var CreateThenPromise(JavascriptPromise* sourcePromise, RecyclableObject* fulfillmentHandler, RecyclableObject* rejectionHandler, ScriptContext* scriptContext);

        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        JavascriptPromiseReactionList* GetResolveReactions();
        JavascriptPromiseReactionList* GetRejectReactions();

        static JavascriptPromiseCapability* NewPromiseCapability(Var constructor, ScriptContext* scriptContext);
        static JavascriptPromiseCapability* CreatePromiseCapabilityRecord(RecyclableObject* constructor, ScriptContext* scriptContext);
        static Var TriggerPromiseReactions(JavascriptPromiseReactionList* reactions, Var resolution, ScriptContext* scriptContext);
        static void EnqueuePromiseReactionTask(JavascriptPromiseReaction* reaction, Var resolution, ScriptContext* scriptContext);

        static void InitializePromise(JavascriptPromise* promise, JavascriptPromiseResolveOrRejectFunction** resolve, JavascriptPromiseResolveOrRejectFunction** reject, ScriptContext* scriptContext);
        static Var TryCallResolveOrRejectHandler(Var handler, Var value, ScriptContext* scriptContext);
        static Var TryRejectWithExceptionObject(JavascriptExceptionObject* exceptionObject, Var handler, ScriptContext* scriptContext);

        static JavascriptPromise* CreateEnginePromise(ScriptContext *scriptContext);

        Var Resolve(Var resolution, ScriptContext* scriptContext);
        Var Reject(Var resolution, ScriptContext* scriptContext);

        enum PromiseStatus
        {
            PromiseStatusCode_Undefined,
            PromiseStatusCode_Unresolved,
            PromiseStatusCode_HasResolution,
            PromiseStatusCode_HasRejection
        };

        bool GetIsHandled() { return isHandled; }
        void SetIsHandled() { isHandled = true; }
        PromiseStatus GetStatus() const { return status; }
        Var GetResult() const { return result; }

    protected:
        Var ResolveHelper(Var resolution, bool isRejecting, ScriptContext* scriptContext);

    protected:
        Field(PromiseStatus) status;
        Field(bool) isHandled;
        Field(Var) result;
        Field(JavascriptPromiseReactionList*) resolveReactions;
        Field(JavascriptPromiseReactionList*) rejectReactions;

    private :
        static void AsyncSpawnStep(JavascriptPromiseAsyncSpawnStepArgumentExecutorFunction* nextFunction, JavascriptGenerator* gen, Var resolve, Var reject);
        bool WillRejectionBeUnhandled();

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        static JavascriptPromise* InitializePromise_TTD(ScriptContext* scriptContext, uint32 status, Var result, JsUtil::List<Js::JavascriptPromiseReaction*, HeapAllocator>& resolveReactions, JsUtil::List<Js::JavascriptPromiseReaction*, HeapAllocator>& rejectReactions);
#endif
    };
}
