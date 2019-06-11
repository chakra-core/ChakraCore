//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // Helper struct used to communicate to a yield point whether it was resumed via next(), return(), or throw()
    // and provide the data necessary for the corresponding action taken (see OP_ResumeYield)
    struct ResumeYieldData
    {
        Var data;
        JavascriptExceptionObject* exceptionObj;
        JavascriptGenerator* generator = nullptr;

        ResumeYieldData(Var data, JavascriptExceptionObject* exceptionObj) : data(data), exceptionObj(exceptionObj) { }
    };

    struct AsyncGeneratorRequest
    {
        Field(Var) data;
        Field(JavascriptExceptionObject*) exceptionObj;
        Field(JavascriptPromise*) promise;

        AsyncGeneratorRequest(Var data, JavascriptExceptionObject* exceptionObj, JavascriptPromise* promise)
             : data(data), exceptionObj(exceptionObj), promise(promise) { }

    };

    typedef JsUtil::List<AsyncGeneratorRequest*, Recycler> AsyncGeneratorQueue;

    class JavascriptGenerator : public DynamicObject
    {
    public:
        enum class GeneratorState
        {
            SuspendedStart,
            Suspended,
            Executing,
            Completed,
            AwaitingReturn
        };

        static uint32 GetFrameOffset() { return offsetof(JavascriptGenerator, frame); }
        static uint32 GetCallInfoOffset() { return offsetof(JavascriptGenerator, args) + Arguments::GetCallInfoOffset(); }
        static uint32 GetArgsPtrOffset() { return offsetof(JavascriptGenerator, args) + Arguments::GetValuesOffset(); }

        void SetState(GeneratorState state) {
            this->state = state;
            if(state == GeneratorState::Completed)
            {
                frame = nullptr;
                args.Values = nullptr;
                scriptFunction = nullptr;
            }
        }

    private:
        Field(InterpreterStackFrame*) frame;
        Field(GeneratorState) state;
        Field(Arguments) args;
        Field(ScriptFunction*) scriptFunction;
        Field(AsyncGeneratorQueue*) asyncGeneratorQueue;
        Field(RuntimeFunction*) awaitNextFunction;
        Field(RuntimeFunction*) awaitThrowFunction;
        Field(RuntimeFunction*) awaitYieldFunction;
        Field(RuntimeFunction*) awaitYieldStarFunction;
        Field(bool) isAsync = false;
        Field(int) queuePosition = 0;
        Field(int) queueLength = 0;

        DEFINE_VTABLE_CTOR_MEMBER_INIT(JavascriptGenerator, DynamicObject, args);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptGenerator);

        Var CallGenerator(ResumeYieldData* yieldData, const char16* apiNameForErrorMessage);
        JavascriptGenerator(DynamicType* type, Arguments& args, ScriptFunction* scriptFunction);

    public:
        static JavascriptGenerator* New(Recycler* recycler, DynamicType* generatorType, Arguments& args, ScriptFunction* scriptFunction);

        static JavascriptGenerator *New(Recycler *recycler, DynamicType *generatorType, Arguments &args, Js::JavascriptGenerator::GeneratorState generatorState);

        bool IsExecuting() const { return state == GeneratorState::Executing; }
        bool IsSuspended() const { return state == GeneratorState::Suspended; }
        bool IsCompleted() const { return state == GeneratorState::Completed; }
        bool IsAwaitingReturn() const { return state == GeneratorState::AwaitingReturn; }
        bool IsSuspendedStart() const { return state == GeneratorState::SuspendedStart || (state == GeneratorState::Suspended && this->frame == nullptr); }
        void EnqueueRequest(AsyncGeneratorRequest* request)
        {
            asyncGeneratorQueue->Add(request);
            ++queueLength;
        }
        AsyncGeneratorRequest* GetRequest (bool pop)
        {
            Assert(HasRequests());
            AsyncGeneratorRequest* request = asyncGeneratorQueue->Item(queuePosition);
            if (pop)
            {
                ++queuePosition;
            }
            return request;
        }
        bool HasRequests() { return queuePosition < queueLength; }

        void SetIsAsync() { isAsync = true; }
        bool GetIsAsync() const { return isAsync; }
        RuntimeFunction* GetAwaitNextFunction() { return awaitNextFunction; }
        RuntimeFunction* GetAwaitThrowFunction() { return awaitThrowFunction; }
        RuntimeFunction* GetAwaitYieldFunction() { return awaitYieldFunction; }
        RuntimeFunction* EnsureAwaitYieldStarFunction();
        void ProcessAsyncGeneratorReturn(Var value, ScriptContext* scriptContext);
        void AsyncGeneratorResumeNext();
        void AsyncGeneratorReject(Var reason);
        void AsyncGeneratorResolve(Var value, bool done);
        void CallAsyncGenerator(ResumeYieldData* yieldData);
        void InitialiseAsyncGenerator(ScriptContext* scriptContext);

        void SetScriptFunction(ScriptFunction* sf)
        {
            this->scriptFunction = sf;
        }

        void SetFrame(InterpreterStackFrame* frame, size_t bytes);
        InterpreterStackFrame* GetFrame() const { return frame; }
        void SetFrameSlots(uint slotCount, Field(Var)* frameSlotArray);

#if GLOBAL_ENABLE_WRITE_BARRIER
        virtual void Finalize(bool isShutdown) override;
#endif

        const Arguments& GetArguments() const { return args; }

        class EntryInfo
        {
        public:
            static FunctionInfo Next;
            static FunctionInfo Return;
            static FunctionInfo Throw;
            static FunctionInfo AsyncNext;
            static FunctionInfo AsyncReturn;
            static FunctionInfo AsyncThrow;
        };
        static Var EntryNext(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReturn(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryThrow(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryAsyncNext(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncReturn(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncThrow(RecyclableObject* function, CallInfo callInfo, ...);
        static Var AsyncGeneratorEnqueue(Var thisValue, ScriptContext* scriptContext, Var input, JavascriptExceptionObject* exceptionObj, const char16* apiNameForErrorMessage);
        static Var CreateTypeError(HRESULT hr, ScriptContext* scriptContext, ...);
        static Var EntryAsyncGeneratorResumeNextReturnProcessorReject(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncGeneratorResumeNextReturnProcessorResolve(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncGeneratorAwaitReject(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncGeneratorAwaitRevolve(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncGeneratorAwaitYield(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncGeneratorAwaitYieldStar(RecyclableObject* function, CallInfo callInfo, ...);
#if ENABLE_TTD
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
        //virtual void ProcessCorePaths() override;
#endif
    };

    template <> bool VarIsImpl<JavascriptGenerator>(RecyclableObject* obj);

    class AsyncGeneratorNextProcessor : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(AsyncGeneratorNextProcessor, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(AsyncGeneratorNextProcessor);

    public:
        AsyncGeneratorNextProcessor(DynamicType* type, FunctionInfo* functionInfo, JavascriptGenerator* generator)
            : RuntimeFunction(type, functionInfo), generator(generator) { }

        inline JavascriptGenerator* GetGenerator() { return this->generator; }

    private:
        Field(JavascriptGenerator*) generator;
    };

    template <> bool VarIsImpl<AsyncGeneratorNextProcessor>(RecyclableObject* obj);
}
