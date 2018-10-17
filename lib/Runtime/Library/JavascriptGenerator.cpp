//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Language/InterpreterStackFrame.h"

namespace Js
{
    JavascriptGenerator::JavascriptGenerator(DynamicType* type, Arguments &args, ScriptFunction* scriptFunction)
        : DynamicObject(type), frame(nullptr), state(GeneratorState::Suspended), args(args), scriptFunction(scriptFunction)
    {
    }

    JavascriptGenerator* JavascriptGenerator::New(Recycler* recycler, DynamicType* generatorType, Arguments& args, ScriptFunction* scriptFunction)
    {
#if GLOBAL_ENABLE_WRITE_BARRIER
        if (CONFIG_FLAG(ForceSoftwareWriteBarrier))
        {
            JavascriptGenerator* obj = RecyclerNewFinalized(
                recycler, JavascriptGenerator, generatorType, args, scriptFunction);
            if (obj->args.Values != nullptr)
            {
                recycler->RegisterPendingWriteBarrierBlock(obj->args.Values, obj->args.Info.Count * sizeof(Var));
                recycler->RegisterPendingWriteBarrierBlock(&obj->args.Values, sizeof(Var*));
            }
            return obj;
        }
        else
#endif
        {
            return RecyclerNew(recycler, JavascriptGenerator, generatorType, args, scriptFunction);
        }
    }

    JavascriptGenerator *JavascriptGenerator::New(Recycler *recycler, DynamicType *generatorType, Arguments &args,
        Js::JavascriptGenerator::GeneratorState generatorState)
    {
        JavascriptGenerator *obj = JavascriptGenerator::New(recycler, generatorType, args, nullptr);
        obj->SetState(generatorState);
        return obj;
    }

    template <> bool VarIsImpl<JavascriptGenerator>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Generator;
    }

    void JavascriptGenerator::SetFrame(InterpreterStackFrame* frame, size_t bytes)
    {
        Assert(this->frame == nullptr);
        this->frame = frame;
#if GLOBAL_ENABLE_WRITE_BARRIER
        if (CONFIG_FLAG(ForceSoftwareWriteBarrier))
        {
            this->GetScriptContext()->GetRecycler()->RegisterPendingWriteBarrierBlock(frame, bytes);
        }
#endif
    }

    void JavascriptGenerator::SetFrameSlots(Js::RegSlot slotCount, Field(Var)* frameSlotArray)
    {
        AssertMsg(this->frame->GetFunctionBody()->GetLocalsCount() == slotCount, "Unexpected mismatch in frame slot count for generated.");
        for (Js::RegSlot i = 0; i < slotCount; i++)
        {
            this->GetFrame()->m_localSlots[i] = frameSlotArray[i];
        }
    }

#if GLOBAL_ENABLE_WRITE_BARRIER
    void JavascriptGenerator::Finalize(bool isShutdown)
    {
        if (CONFIG_FLAG(ForceSoftwareWriteBarrier) && !isShutdown)
        {
            if (this->frame)
            {
                this->GetScriptContext()->GetRecycler()->UnRegisterPendingWriteBarrierBlock(this->frame);
            }
            if (this->args.Values)
            {
                this->GetScriptContext()->GetRecycler()->UnRegisterPendingWriteBarrierBlock(this->args.Values);
            }
        }
    }
#endif

    Var JavascriptGenerator::CallGenerator(ResumeYieldData* yieldData, const char16* apiNameForErrorMessage)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var result = nullptr;

        if (this->IsExecuting())
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_GeneratorAlreadyExecuting, apiNameForErrorMessage);
        }

        {
            // RAII helper to set the state of the generator to completed if an exception is thrown
            // or if the save state InterpreterStackFrame is never created implying the generator
            // is JITed and returned without ever yielding.
            class GeneratorStateHelper
            {
                JavascriptGenerator* g;
                bool didThrow;
            public:
                GeneratorStateHelper(JavascriptGenerator* g) : g(g), didThrow(true) { g->SetState(GeneratorState::Executing); }
                ~GeneratorStateHelper() { g->SetState(didThrow || g->frame == nullptr ? GeneratorState::Completed : GeneratorState::Suspended); }
                void DidNotThrow() { didThrow = false; }
            } helper(this);

            Var thunkArgs[] = { this, yieldData };
            Arguments arguments(_countof(thunkArgs), thunkArgs);

            JavascriptExceptionObject *exception = nullptr;

            try
            {
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    result = JavascriptFunction::CallFunction<1>(this->scriptFunction, this->scriptFunction->GetEntryPoint(), arguments);
                }
                END_SAFE_REENTRANT_CALL
                helper.DidNotThrow();
            }
            catch (const JavascriptException& err)
            {
                exception = err.GetAndClear();
            }

            if (exception != nullptr)
            {
                if (!exception->IsGeneratorReturnException())
                {
                    JavascriptExceptionOperators::DoThrowCheckClone(exception, scriptContext);
                }
                result = exception->GetThrownObject(nullptr);
            }
        }

        if (!this->IsCompleted())
        {
            int nextOffset = this->frame->GetReader()->GetCurrentOffset();
            int endOffset = this->frame->GetFunctionBody()->GetByteCode()->GetLength();

            if (nextOffset != endOffset - 1)
            {
                // Yielded values are already wrapped in an IteratorResult object, so we don't need to wrap them.
                return result;
            }
        }

        result = library->CreateIteratorResultObject(result, library->GetTrue());
        this->SetState(GeneratorState::Completed);

        return result;
    }

    Var JavascriptGenerator::EntryNext(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Generator.prototype.next"));

        if (!VarIs<JavascriptGenerator>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Generator.prototype.next"), _u("Generator"));
        }

        JavascriptGenerator* generator = VarTo<JavascriptGenerator>(args[0]);
        Var input = args.Info.Count > 1 ? args[1] : library->GetUndefined();

        if (generator->IsCompleted())
        {
            return library->CreateIteratorResultObjectUndefinedTrue();
        }

        ResumeYieldData yieldData(input, nullptr);
        return generator->CallGenerator(&yieldData, _u("Generator.prototype.next"));
    }

    Var JavascriptGenerator::EntryReturn(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Generator.prototype.return"));

        if (!VarIs<JavascriptGenerator>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Generator.prototype.return"), _u("Generator"));
        }

        JavascriptGenerator* generator = VarTo<JavascriptGenerator>(args[0]);
        Var input = args.Info.Count > 1 ? args[1] : library->GetUndefined();

        if (generator->IsSuspendedStart())
        {
            generator->SetState(GeneratorState::Completed);
        }

        if (generator->IsCompleted())
        {
            return library->CreateIteratorResultObject(input, library->GetTrue());
        }

        ResumeYieldData yieldData(input, RecyclerNew(scriptContext->GetRecycler(), GeneratorReturnExceptionObject, input, scriptContext));
        return generator->CallGenerator(&yieldData, _u("Generator.prototype.return"));
    }

    Var JavascriptGenerator::EntryThrow(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Generator.prototype.throw"));

        if (!VarIs<JavascriptGenerator>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Generator.prototype.throw"), _u("Generator"));
        }

        JavascriptGenerator* generator = VarTo<JavascriptGenerator>(args[0]);
        Var input = args.Info.Count > 1 ? args[1] : library->GetUndefined();

        if (generator->IsSuspendedStart())
        {
            generator->SetState(GeneratorState::Completed);
        }

        if (generator->IsCompleted())
        {
            JavascriptExceptionOperators::OP_Throw(input, scriptContext);
        }

        ResumeYieldData yieldData(input, RecyclerNew(scriptContext->GetRecycler(), JavascriptExceptionObject, input, scriptContext, nullptr));
        return generator->CallGenerator(&yieldData, _u("Generator.prototype.throw"));
    }

#if ENABLE_TTD

    void JavascriptGenerator::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        if (this->scriptFunction != nullptr)
        {
            extractor->MarkVisitVar(this->scriptFunction);
        }

        // frame is null when generator has been completed
        if (this->frame != nullptr)
        {
            // mark slot variables for traversal
            Js::RegSlot slotCount = this->frame->GetFunctionBody()->GetLocalsCount();
            for (Js::RegSlot i = 0; i < slotCount; i++)
            {
                Js::Var curr = this->frame->m_localSlots[i];
                if (curr != nullptr)
                {
                    extractor->MarkVisitVar(curr);
                }
            }
        }

        // args.Values is null when generator has been completed
        if (this->args.Values != nullptr)
        {
            // mark argument variables for traversal
            uint32 argCount = this->args.GetArgCountWithExtraArgs();
            for (uint32 i = 0; i < argCount; i++)
            {
                Js::Var curr = this->args[i];
                if (curr != nullptr)
                {
                    extractor->MarkVisitVar(curr);
                }
            }
        }
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptGenerator::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapGenerator;
    }

    void JavascriptGenerator::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapGeneratorInfo* gi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapGeneratorInfo>();

        // TODO: BUGBUG - figure out how to determine what the prototype was
        gi->generatorPrototype = 0;
        //if (this->GetPrototype() == this->GetScriptContext()->GetLibrary()->GetNull())
        //{
        //    gi->generatorPrototype = 1;
        //}
        //else if (this->GetType() == this->GetScriptContext()->GetLibrary()->GetGeneratorConstructorPrototypeObjectType())
        //{
        //    // check type here, not prototype, since type is static across generators
        //    gi->generatorPrototype = 2;
        //}
        //else
        //{
        //    //TTDAssert(false, "unexpected prototype found JavascriptGenerator");
        //}

        gi->scriptFunction = TTD_CONVERT_VAR_TO_PTR_ID(this->scriptFunction);
        gi->state = static_cast<uint32>(this->state);


        // grab slot info from InterpreterStackFrame
        gi->frame_slotCount = 0;
        gi->frame_slotArray = nullptr;
        if (this->frame != nullptr)
        {
            gi->frame_slotCount = this->frame->GetFunctionBody()->GetLocalsCount();
            if (gi->frame_slotCount > 0)
            {
                gi->frame_slotArray = alloc.SlabAllocateArray<TTD::TTDVar>(gi->frame_slotCount);
            }
            for (Js::RegSlot i = 0; i < gi->frame_slotCount; i++)
            {
                gi->frame_slotArray[i] = this->frame->m_localSlots[i];
            }
        }

        // grab arguments
        TTD_PTR_ID* depArray = nullptr;
        uint32 depCount = 0;

        if (this->args.Values == nullptr)
        {
            gi->arguments_count = 0;
        }
        else
        {
            gi->arguments_count = this->args.GetArgCountWithExtraArgs();
        }

        gi->arguments_values = nullptr;
        if (gi->arguments_count > 0)
        {
            gi->arguments_values = alloc.SlabAllocateArray<TTD::TTDVar>(gi->arguments_count);
            depArray = alloc.SlabReserveArraySpace<TTD_PTR_ID>(gi->arguments_count);
        }

        for (uint32 i = 0; i < gi->arguments_count; i++)
        {
            gi->arguments_values[i] = this->args[i];
            if (gi->arguments_values[i] != nullptr && TTD::JsSupport::IsVarComplexKind(gi->arguments_values[i]))
            {
                depArray[depCount] = TTD_CONVERT_VAR_TO_PTR_ID(gi->arguments_values[i]);
                depCount++;
            }
        }

        if (depCount > 0)
        {
            alloc.SlabCommitArraySpace<TTD_PTR_ID>(depCount, gi->arguments_count);
        }
        else if (gi->arguments_count > 0)
        {
            alloc.SlabAbortArraySpace<TTD_PTR_ID>(gi->arguments_count);
        }

        if (this->frame != nullptr)
        {
            gi->byteCodeReader_offset = this->frame->GetReader()->GetCurrentOffset();
        }
        else
        {
            gi->byteCodeReader_offset = 0;
        }

        // Copy the CallInfo data into the struct
        gi->arguments_callInfo_count = this->args.Info.Count;
        gi->arguments_callInfo_flags = this->args.Info.Flags;

        // TODO:  understand why there's a mis-match between args.Info.Count and GetArgCountWithExtraArgs
        // TTDAssert(this->args.Info.Count == gi->arguments_count, "mismatched count between args.Info and GetArgCountWithExtraArgs");

        if (depCount == 0)
        {
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapGeneratorInfo*, TTD::NSSnapObjects::SnapObjectType::SnapGenerator>(objData, gi);
        }
        else
        {
            TTDAssert(depArray != nullptr, "depArray should be non-null if depCount is > 0");
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapGeneratorInfo*, TTD::NSSnapObjects::SnapObjectType::SnapGenerator>(objData, gi, alloc, depCount, depArray);
        }

    }
#endif
}
