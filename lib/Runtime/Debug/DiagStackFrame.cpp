//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#ifdef ENABLE_SCRIPT_DEBUGGING
#include "Language/JavascriptFunctionArgIndex.h"
#include "Language/InterpreterStackFrame.h"
#include "Language/JavascriptStackWalker.h"

namespace Js
{
    DiagStackFrame::DiagStackFrame():
        isTopFrame(false)
    {
    }

    // Returns whether or not this frame is on the top of the callstack.
    bool DiagStackFrame::IsTopFrame()
    {
        return this->isTopFrame && GetScriptContext()->GetDebugContext()->GetProbeContainer()->IsPrimaryBrokenToDebuggerContext();
    }

    void DiagStackFrame::SetIsTopFrame()
    {
        this->isTopFrame = true;
    }

    ScriptFunction* DiagStackFrame::GetScriptFunction()
    {
        return ScriptFunction::FromVar(GetJavascriptFunction());
    }

    FunctionBody* DiagStackFrame::GetFunction()
    {
        return GetJavascriptFunction()->GetFunctionBody();
    }

    ScriptContext* DiagStackFrame::GetScriptContext()
    {
        return GetJavascriptFunction()->GetScriptContext();
    }

    PCWSTR DiagStackFrame::GetDisplayName()
    {
        return GetFunction()->GetExternalDisplayName();
    }

    bool DiagStackFrame::IsInterpreterFrame()
    {
        return false;
    }

    InterpreterStackFrame* DiagStackFrame::AsInterpreterFrame()
    {
        AssertMsg(FALSE, "AsInterpreterFrame called for non-interpreter frame.");
        return nullptr;
    }

    ArenaAllocator * DiagStackFrame::GetArena()
    {
        Assert(GetScriptContext() != NULL);
        return GetScriptContext()->GetThreadContext()->GetDebugManager()->GetDiagnosticArena()->Arena();
    }

    FrameDisplay * DiagStackFrame::GetFrameDisplay()
    {
        FrameDisplay *display = NULL;

        Assert(this->GetFunction() != NULL);
        RegSlot frameDisplayReg = this->GetFunction()->GetFrameDisplayRegister();

        if (frameDisplayReg != Js::Constants::NoRegister && frameDisplayReg != 0)
        {
            display = (FrameDisplay*)this->GetNonVarRegValue(frameDisplayReg);
        }
        else
        {
            display = this->GetScriptFunction()->GetEnvironment();
        }
 
        return display;
    }

    Var DiagStackFrame::GetScopeObjectFromFrameDisplay(uint index)
    {
        FrameDisplay * display = GetFrameDisplay();
        return (display != NULL && display->GetLength() > index) ? display->GetItem(index) : NULL;
    }

    Var DiagStackFrame::GetRootObject()
    {
        Assert(this->GetFunction());
        return this->GetFunction()->LoadRootObject();
    }

    BOOL DiagStackFrame::IsStrictMode()
    {
        Js::JavascriptFunction* scopeFunction = this->GetJavascriptFunction();
        return scopeFunction->IsStrictMode();
    }

    BOOL DiagStackFrame::IsThisAvailable()
    {
        Js::JavascriptFunction* scopeFunction = this->GetJavascriptFunction();
        return !scopeFunction->IsLambda() || scopeFunction->GetParseableFunctionInfo()->GetCapturesThis();
    }

    Js::Var DiagStackFrame::GetThisFromFrame(Js::IDiagObjectAddress ** ppOutAddress, Js::IDiagObjectModelWalkerBase * localsWalker)
    {
        Js::ScriptContext* scriptContext = this->GetScriptContext();
        Js::JavascriptFunction* scopeFunction = this->GetJavascriptFunction();
        Js::ModuleID moduleId = scopeFunction->IsScriptFunction() ? scopeFunction->GetFunctionBody()->GetModuleID() : 0;
        Js::Var varThis = scriptContext->GetLibrary()->GetNull();

        if (!scopeFunction->IsLambda())
        {
            Js::JavascriptStackWalker::GetThis(&varThis, moduleId, scopeFunction, scriptContext);
        }
        else
        {
            if (!scopeFunction->GetParseableFunctionInfo()->GetCapturesThis())
            {
                return nullptr;
            }
            else
            {
                // Emulate Js::JavascriptOperators::OP_GetThisScoped using a locals walker and assigning moduleId object if not found by locals walker
                if (localsWalker == nullptr)
                {
                    ArenaAllocator *arena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena()->Arena();
                    localsWalker = Anew(arena, Js::LocalsWalker, this, Js::FrameWalkerFlags::FW_EnumWithScopeAlso | Js::FrameWalkerFlags::FW_AllowLexicalThis);
                }

                bool unused = false;
                Js::IDiagObjectAddress* address = localsWalker->FindPropertyAddress(Js::PropertyIds::_lexicalThisSlotSymbol, unused);

                if (ppOutAddress != nullptr)
                {
                    *ppOutAddress = address;
                }

                if (address != nullptr)
                {
                    varThis = address->GetValue(FALSE);
                }
                else if (moduleId == kmodGlobal)
                {
                    varThis = Js::JavascriptOperators::OP_LdRoot(scriptContext)->ToThis();
                }
                else
                {
                    varThis = (Var)Js::JavascriptOperators::GetModuleRoot(moduleId, scriptContext);
                }
            }
        }

        Js::GlobalObject::UpdateThisForEval(varThis, moduleId, scriptContext, this->IsStrictMode());

        return varThis;
    }

    void DiagStackFrame::TryFetchValueAndAddress(const char16 *source, int sourceLength, Js::ResolvedObject * pOutResolvedObj)
    {
        Assert(source);
        Assert(pOutResolvedObj);

        Js::ScriptContext* scriptContext = this->GetScriptContext();
        Js::JavascriptFunction* scopeFunction = this->GetJavascriptFunction();

        // Do fast path for 'this', fields on slot, TODO : literals (integer,string)

        if (sourceLength == 4 && wcsncmp(source, _u("this"), 4) == 0)
        {
            pOutResolvedObj->obj = this->GetThisFromFrame(&pOutResolvedObj->address);
            if (pOutResolvedObj->obj == nullptr)
            {
                // TODO: Throw exception; this was not captured by the lambda
                Assert(scopeFunction->IsLambda());
                Assert(!scopeFunction->GetParseableFunctionInfo()->GetCapturesThis());
            }
        }
        else
        {
            Js::PropertyRecord const * propRecord;
            scriptContext->FindPropertyRecord(source, sourceLength, &propRecord);
            if (propRecord != nullptr)
            {
                ArenaAllocator *arena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena()->Arena();

                Js::IDiagObjectModelWalkerBase * localsWalker = Anew(arena, Js::LocalsWalker, this, Js::FrameWalkerFlags::FW_EnumWithScopeAlso);

                bool isConst = false;
                pOutResolvedObj->address = localsWalker->FindPropertyAddress(propRecord->GetPropertyId(), isConst);
                if (pOutResolvedObj->address != nullptr)
                {
                    pOutResolvedObj->obj = pOutResolvedObj->address->GetValue(FALSE);
                    pOutResolvedObj->isConst = isConst;
                }
            }
        }

    }

    Js::ScriptFunction* DiagStackFrame::TryGetFunctionForEval(Js::ScriptContext* scriptContext, const char16 *source, int sourceLength, BOOL isLibraryCode /* = FALSE */)
    {
        // TODO: pass the real length of the source code instead of wcslen
        uint32 grfscr = fscrReturnExpression | fscrEval | fscrEvalCode | fscrGlobalCode | fscrConsoleScopeEval;
        if (!this->IsThisAvailable())
        {
            grfscr |= fscrDebuggerErrorOnGlobalThis;
        }
        if (isLibraryCode)
        {
            grfscr |= fscrIsLibraryCode;
        }
        return scriptContext->GetGlobalObject()->EvalHelper(scriptContext, source, sourceLength, kmodGlobal, grfscr, Js::Constants::EvalCode, FALSE, FALSE, this->IsStrictMode());
    }

    void DiagStackFrame::EvaluateImmediate(const char16 *source, int sourceLength, BOOL isLibraryCode, Js::ResolvedObject * resolvedObject)
    {
        this->TryFetchValueAndAddress(source, sourceLength, resolvedObject);

        if (resolvedObject->obj == nullptr)
        {
            Js::ScriptFunction* pfuncScript = this->TryGetFunctionForEval(this->GetScriptContext(), source, sourceLength, isLibraryCode);
            if (pfuncScript != nullptr)
            {
                // Passing the nonuser code state from the enclosing function to the current function.
                // Treat native library frame (no function body) as non-user code.
                Js::FunctionBody* body = this->GetFunction();
                if (!body || body->IsNonUserCode())
                {
                    Js::FunctionBody *pCurrentFuncBody = pfuncScript->GetFunctionBody();
                    if (pCurrentFuncBody != nullptr)
                    {
                        pCurrentFuncBody->SetIsNonUserCode(true);
                    }
                }
                OUTPUT_TRACE(Js::ConsoleScopePhase, _u("EvaluateImmediate strict = %d, libraryCode = %d, source = '%s'\n"),
                    this->IsStrictMode(), isLibraryCode, source);
                resolvedObject->obj = this->DoEval(pfuncScript);
            }
        }
    }

#ifdef ENABLE_MUTATION_BREAKPOINT
    static void SetConditionalMutationBreakpointVariables(Js::DynamicObject * activeScopeObject, Js::ScriptContext * scriptContext)
    {
        // For Conditional Object Mutation Breakpoint user can access the new value, changing property name and mutation type using special variables
        // $newValue$, $propertyName$ and $mutationType$. Add this variables to activation object.
        Js::DebugManager* debugManager = scriptContext->GetDebugContext()->GetProbeContainer()->GetDebugManager();
        Js::MutationBreakpoint *mutationBreakpoint = debugManager->GetActiveMutationBreakpoint();
        if (mutationBreakpoint != nullptr)
        {
            if (Js::Constants::NoProperty == debugManager->mutationNewValuePid)
            {
                debugManager->mutationNewValuePid = scriptContext->GetOrAddPropertyIdTracked(_u("$newValue$"), 10);
            }
            if (Js::Constants::NoProperty == debugManager->mutationPropertyNamePid)
            {
                debugManager->mutationPropertyNamePid = scriptContext->GetOrAddPropertyIdTracked(_u("$propertyName$"), 14);
            }
            if (Js::Constants::NoProperty == debugManager->mutationTypePid)
            {
                debugManager->mutationTypePid = scriptContext->GetOrAddPropertyIdTracked(_u("$mutationType$"), 14);
            }

            AssertMsg(debugManager->mutationNewValuePid != Js::Constants::NoProperty, "Should have a valid mutationNewValuePid");
            AssertMsg(debugManager->mutationPropertyNamePid != Js::Constants::NoProperty, "Should have a valid mutationPropertyNamePid");
            AssertMsg(debugManager->mutationTypePid != Js::Constants::NoProperty, "Should have a valid mutationTypePid");

            Js::Var newValue = mutationBreakpoint->GetBreakNewValueVar();

            // Incase of MutationTypeDelete we won't have new value
            if (nullptr != newValue)
            {
                activeScopeObject->SetProperty(debugManager->mutationNewValuePid,
                    mutationBreakpoint->GetBreakNewValueVar(),
                    Js::PropertyOperationFlags::PropertyOperation_None,
                    nullptr);
            }
            else
            {
                activeScopeObject->SetProperty(debugManager->mutationNewValuePid,
                    scriptContext->GetLibrary()->GetUndefined(),
                    Js::PropertyOperationFlags::PropertyOperation_None,
                    nullptr);
            }

            // User should not be able to change $propertyName$ and $mutationType$ variables
            // Since we don't have address for $propertyName$ and $mutationType$ even if user change these varibales it won't be reflected after eval
            // But declaring these as const to prevent accidental typos by user so that we throw error in case user changes these variables

            Js::PropertyOperationFlags flags = static_cast<Js::PropertyOperationFlags>(Js::PropertyOperation_SpecialValue | Js::PropertyOperation_AllowUndecl);

            activeScopeObject->SetPropertyWithAttributes(debugManager->mutationPropertyNamePid,
                Js::JavascriptString::NewCopySz(mutationBreakpoint->GetBreakPropertyName(), scriptContext),
                PropertyConstDefaults, nullptr, flags);

            activeScopeObject->SetPropertyWithAttributes(debugManager->mutationTypePid,
                Js::JavascriptString::NewCopySz(mutationBreakpoint->GetMutationTypeForConditionalEval(mutationBreakpoint->GetBreakMutationType()), scriptContext),
                PropertyConstDefaults, nullptr, flags);
        }
    }
#endif

    Js::Var DiagStackFrame::DoEval(Js::ScriptFunction* pfuncScript)
    {
        Js::Var varResult = nullptr;

        Js::JavascriptFunction* scopeFunction = this->GetJavascriptFunction();
        Js::ScriptContext* scriptContext = this->GetScriptContext();

        ArenaAllocator *arena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena()->Arena();
        Js::LocalsWalker *localsWalker = Anew(arena, Js::LocalsWalker, this, 
            Js::FrameWalkerFlags::FW_EnumWithScopeAlso | Js::FrameWalkerFlags::FW_AllowLexicalThis | Js::FrameWalkerFlags::FW_AllowSuperReference | Js::FrameWalkerFlags::FW_DontAddGlobalsDirectly);

        // Store the diag address of a var to the map so that it will be used for editing the value.
        typedef JsUtil::BaseDictionary<Js::PropertyId, Js::IDiagObjectAddress*, ArenaAllocator, PrimeSizePolicy> PropIdToDiagAddressMap;
        PropIdToDiagAddressMap * propIdtoDiagAddressMap = Anew(arena, PropIdToDiagAddressMap, arena);

        // Create one scope object and init all scope properties in it, and push this object in front of the environment.
        Js::DynamicObject * activeScopeObject = localsWalker->CreateAndPopulateActivationObject(scriptContext, [propIdtoDiagAddressMap](Js::ResolvedObject& resolveObject)
        {
            if (!resolveObject.isConst)
            {
                propIdtoDiagAddressMap->AddNew(resolveObject.propId, resolveObject.address);
            }
        });
        if (!activeScopeObject)
        {
            activeScopeObject = scriptContext->GetLibrary()->CreateActivationObject();
        }

#ifdef ENABLE_MUTATION_BREAKPOINT
        SetConditionalMutationBreakpointVariables(activeScopeObject, scriptContext);
#endif

#if DBG
        uint32 countForVerification = activeScopeObject->GetPropertyCount();
#endif

        // Dummy scope object in the front, so that no new variable will be added to the scope.
        Js::DynamicObject * dummyObject = scriptContext->GetLibrary()->CreateActivationObject();

        // Remove its prototype object so that those item will not be visible to the expression evaluation.
        dummyObject->SetPrototype(scriptContext->GetLibrary()->GetNull());
        Js::DebugManager* debugManager = scriptContext->GetDebugContext()->GetProbeContainer()->GetDebugManager();
        Js::FrameDisplay* env = debugManager->GetFrameDisplay(scriptContext, dummyObject, activeScopeObject);
        pfuncScript->SetEnvironment(env);

        Js::Var varThis = this->GetThisFromFrame(nullptr, localsWalker);
        if (varThis == nullptr)
        {
            Assert(scopeFunction->IsLambda());
            Assert(!scopeFunction->GetParseableFunctionInfo()->GetCapturesThis());
            varThis = scriptContext->GetLibrary()->GetNull();
        }

        Js::Arguments args(1, (Js::Var*) &varThis);
        varResult = pfuncScript->CallFunction(args);

        debugManager->UpdateConsoleScope(dummyObject, scriptContext);

        // We need to find out the edits have been done to the dummy scope object during the eval. We need to apply those mutations to the actual vars.
        uint32 count = activeScopeObject->GetPropertyCount();

#if DBG
        Assert(countForVerification == count);
#endif

        for (uint32 i = 0; i < count; i++)
        {
            Js::PropertyId propertyId = activeScopeObject->GetPropertyId((Js::PropertyIndex)i);
            if (propertyId != Js::Constants::NoProperty)
            {
                Js::Var value = nullptr;
                if (Js::JavascriptOperators::GetProperty(activeScopeObject, propertyId, &value, scriptContext))
                {
                    Js::IDiagObjectAddress * pAddress = nullptr;
                    if (propIdtoDiagAddressMap->TryGetValue(propertyId, &pAddress))
                    {
                        Assert(pAddress);
                        if (pAddress->GetValue(FALSE) != value)
                        {
                            pAddress->Set(value);
                        }
                    }
                }
            }
        }

        return varResult;
    }

    Var DiagStackFrame::GetInnerScopeFromRegSlot(RegSlot location)
    {
        return GetNonVarRegValue(location);
    }

    DiagInterpreterStackFrame::DiagInterpreterStackFrame(InterpreterStackFrame* frame) :
        m_interpreterFrame(frame)
    {
        Assert(m_interpreterFrame != NULL);
        AssertMsg(m_interpreterFrame->GetScriptContext() && m_interpreterFrame->GetScriptContext()->IsScriptContextInDebugMode(),
            "This only supports interpreter stack frames running in debug mode.");
    }

    JavascriptFunction* DiagInterpreterStackFrame::GetJavascriptFunction()
    {
        return m_interpreterFrame->GetJavascriptFunction();
    }

    ScriptContext* DiagInterpreterStackFrame::GetScriptContext()
    {
        return m_interpreterFrame->GetScriptContext();
    }

    int DiagInterpreterStackFrame::GetByteCodeOffset()
    {
        return m_interpreterFrame->GetReader()->GetCurrentOffset();
    }

    // Address on stack that belongs to current frame.
    // Currently we only use this to determine which of given frames is above/below another one.
    DWORD_PTR DiagInterpreterStackFrame::GetStackAddress()
    {
        return m_interpreterFrame->GetStackAddress();
    }

    bool DiagInterpreterStackFrame::IsInterpreterFrame()
    {
        return true;
    }

    InterpreterStackFrame* DiagInterpreterStackFrame::AsInterpreterFrame()
    {
        return m_interpreterFrame;
    }

    Var DiagInterpreterStackFrame::GetRegValue(RegSlot slotId, bool allowTemp)
    {
        return m_interpreterFrame->GetReg(slotId);
    }

    Var DiagInterpreterStackFrame::GetNonVarRegValue(RegSlot slotId)
    {
        return m_interpreterFrame->GetNonVarReg(slotId);
    }

    void DiagInterpreterStackFrame::SetRegValue(RegSlot slotId, Var value)
    {
        m_interpreterFrame->SetReg(slotId, value);
    }

    Var DiagInterpreterStackFrame::GetArgumentsObject()
    {
        return m_interpreterFrame->GetArgumentsObject();
    }

    Var DiagInterpreterStackFrame::CreateHeapArguments()
    {
        return m_interpreterFrame->CreateHeapArguments(GetScriptContext());
    }

    FrameDisplay * DiagInterpreterStackFrame::GetFrameDisplay()
    {
        return m_interpreterFrame->GetFrameDisplayForNestedFunc();
    }

    Var DiagInterpreterStackFrame::GetInnerScopeFromRegSlot(RegSlot location)
    {
        return m_interpreterFrame->InnerScopeFromRegSlot(location);
    }

#if ENABLE_NATIVE_CODEGEN
    DiagNativeStackFrame::DiagNativeStackFrame(
        ScriptFunction* function,
        int byteCodeOffset,
        void* stackAddr,
        void *codeAddr) :
        m_function(function),
        m_byteCodeOffset(byteCodeOffset),
        m_stackAddr(stackAddr),
        m_localVarSlotsOffset(InvalidOffset),
        m_localVarChangedOffset(InvalidOffset)
    {
        Assert(m_stackAddr != NULL);
        AssertMsg(m_function && m_function->GetScriptContext() && m_function->GetScriptContext()->IsScriptContextInDebugMode(),
            "This only supports functions in debug mode.");

        FunctionEntryPointInfo * entryPointInfo = GetFunction()->GetEntryPointFromNativeAddress((DWORD_PTR)codeAddr);
        if (entryPointInfo)
        {
            m_localVarSlotsOffset = entryPointInfo->localVarSlotsOffset;
            m_localVarChangedOffset = entryPointInfo->localVarChangedOffset;
        }
        else
        {
            AssertMsg(FALSE, "Failed to get entry point for native address. Most likely the frame is old/gone.");
        }
        OUTPUT_TRACE(Js::DebuggerPhase, _u("DiagNativeStackFrame::DiagNativeStackFrame: e.p(addr %p)=%p varOff=%d changedOff=%d\n"), codeAddr, entryPointInfo, m_localVarSlotsOffset, m_localVarChangedOffset);
    }

    JavascriptFunction* DiagNativeStackFrame::GetJavascriptFunction()
    {
        return m_function;
    }

    ScriptContext* DiagNativeStackFrame::GetScriptContext()
    {
        return m_function->GetScriptContext();
    }

    int DiagNativeStackFrame::GetByteCodeOffset()
    {
        return m_byteCodeOffset;
    }

    // Address on stack that belongs to current frame.
    // Currently we only use this to determine which of given frames is above/below another one.
    DWORD_PTR DiagNativeStackFrame::GetStackAddress()
    {
        return reinterpret_cast<DWORD_PTR>(m_stackAddr);
    }

    Var DiagNativeStackFrame::GetRegValue(RegSlot slotId, bool allowTemp)
    {
        Js::Var *varPtr = GetSlotOffsetLocation(slotId, allowTemp);
        return (varPtr != NULL) ? *varPtr : NULL;
    }

    Var * DiagNativeStackFrame::GetSlotOffsetLocation(RegSlot slotId, bool allowTemp)
    {
        Assert(GetFunction() != NULL);

        int32 slotOffset;
        if (GetFunction()->GetSlotOffset(slotId, &slotOffset, allowTemp))
        {
            Assert(m_localVarSlotsOffset != InvalidOffset);
            slotOffset = m_localVarSlotsOffset + slotOffset;

            // We will have the var offset only (which is always the Var size. With TypeSpecialization, below will change to accommodate double offset.
            return (Js::Var *)(((char *)m_stackAddr) + slotOffset);
        }

        Assert(false);
        return NULL;
    }

    Var DiagNativeStackFrame::GetNonVarRegValue(RegSlot slotId)
    {
        return GetRegValue(slotId);
    }

    void DiagNativeStackFrame::SetRegValue(RegSlot slotId, Var value)
    {
        Js::Var *varPtr = GetSlotOffsetLocation(slotId);
        Assert(varPtr != NULL);

        // First assign the value
        *varPtr = value;

        Assert(m_localVarChangedOffset != InvalidOffset);

        // Now change the bit in the stack which tells that current stack values got changed.
        char *stackOffset = (((char *)m_stackAddr) + m_localVarChangedOffset);

        Assert(*stackOffset == 0 || *stackOffset == FunctionBody::LocalsChangeDirtyValue);

        *stackOffset = FunctionBody::LocalsChangeDirtyValue;
    }

    Var DiagNativeStackFrame::GetArgumentsObject()
    {
        return (Var)((void **)m_stackAddr)[JavascriptFunctionArgIndex_ArgumentsObject];
    }

    Var DiagNativeStackFrame::CreateHeapArguments()
    {
        // We would be creating the arguments object if there is no default arguments object present.
        Assert(GetArgumentsObject() == NULL);

        CallInfo const * callInfo  = (CallInfo const *)&(((void **)m_stackAddr)[JavascriptFunctionArgIndex_CallInfo]);

        // At the least we will have 'this' by default.
        Assert(callInfo->Count > 0);

        // Get the passed parameter's position (which is starting from 'this')
        Var * inParams = (Var *)&(((void **)m_stackAddr)[JavascriptFunctionArgIndex_This]);

        return JavascriptOperators::LoadHeapArguments(
                                        m_function,
                                        callInfo->Count - 1,
                                        &inParams[1],
                                        GetScriptContext()->GetLibrary()->GetNull(),
                                        (PropertyId*)GetScriptContext()->GetLibrary()->GetNull(),
                                        GetScriptContext(),
                                        /* formalsAreLetDecls */ false);
    }
#endif


    DiagRuntimeStackFrame::DiagRuntimeStackFrame(JavascriptFunction* function, PCWSTR displayName, void* stackAddr):
        m_function(function),
        m_displayName(displayName),
        m_stackAddr(stackAddr)
    {
    }

    JavascriptFunction* DiagRuntimeStackFrame::GetJavascriptFunction()
    {
        return m_function;
    }

    PCWSTR DiagRuntimeStackFrame::GetDisplayName()
    {
        return m_displayName;
    }

    DWORD_PTR DiagRuntimeStackFrame::GetStackAddress()
    {
        return reinterpret_cast<DWORD_PTR>(m_stackAddr);
    }

    int DiagRuntimeStackFrame::GetByteCodeOffset()
    {
        return 0;
    }

    Var DiagRuntimeStackFrame::GetRegValue(RegSlot slotId, bool allowTemp)
    {
        return nullptr;
    }

    Var DiagRuntimeStackFrame::GetNonVarRegValue(RegSlot slotId)
    {
        return nullptr;
    }

    void DiagRuntimeStackFrame::SetRegValue(RegSlot slotId, Var value)
    {
    }

    Var DiagRuntimeStackFrame::GetArgumentsObject()
    {
        return nullptr;
    }

    Var DiagRuntimeStackFrame::CreateHeapArguments()
    {
        return nullptr;
    }

}  // namespace Js
#endif