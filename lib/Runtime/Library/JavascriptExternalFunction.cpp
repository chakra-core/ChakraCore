//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Types/DeferredTypeHandler.h"
namespace Js
{
    // This is a wrapper class for javascript functions that are added directly to the JS engine via BuildDirectFunction
    // or CreateConstructor. We add a thunk before calling into user's direct C++ methods, with additional checks:
    // . check the script site is still alive.
    // . convert globalObject to hostObject
    // . leavescriptstart/end
    // . wrap the result value with potential cross site access

    JavascriptExternalFunction::JavascriptExternalFunction(ExternalMethod entryPoint, DynamicType* type)
        : RuntimeFunction(type, &EntryInfo::ExternalFunctionThunk), nativeMethod(entryPoint), signature(nullptr), callbackState(nullptr), initMethod(nullptr),
        oneBit(1), typeSlots(0), hasAccessors(0), flags(0), deferredLength(0)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptExternalFunction::JavascriptExternalFunction(ExternalMethod entryPoint, DynamicType* type, InitializeMethod method, unsigned short deferredSlotCount, bool accessors)
        : RuntimeFunction(type, &EntryInfo::ExternalFunctionThunk), nativeMethod(entryPoint), signature(nullptr), callbackState(nullptr), initMethod(method),
        oneBit(1), typeSlots(deferredSlotCount), hasAccessors(accessors), flags(0), deferredLength(0)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptExternalFunction::JavascriptExternalFunction(DynamicType* type, InitializeMethod method, unsigned short deferredSlotCount, bool accessors)
        : RuntimeFunction(type, &EntryInfo::DefaultExternalFunctionThunk), nativeMethod(nullptr), signature(nullptr), callbackState(nullptr), initMethod(method),
        oneBit(1), typeSlots(deferredSlotCount), hasAccessors(accessors), flags(0), deferredLength(0)
    {
        DebugOnly(VerifyEntryPoint());
    }


    JavascriptExternalFunction::JavascriptExternalFunction(JavascriptExternalFunction* entryPoint, DynamicType* type)
        : RuntimeFunction(type, &EntryInfo::WrappedFunctionThunk), wrappedMethod(entryPoint), callbackState(nullptr), initMethod(nullptr),
        oneBit(1), typeSlots(0), hasAccessors(0), flags(0), deferredLength(0)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptExternalFunction::JavascriptExternalFunction(StdCallJavascriptMethod entryPoint, DynamicType* type)
        : RuntimeFunction(type, &EntryInfo::StdCallExternalFunctionThunk), stdCallNativeMethod(entryPoint), signature(nullptr), callbackState(nullptr), initMethod(nullptr),
        oneBit(1), typeSlots(0), hasAccessors(0), flags(0), deferredLength(0)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptExternalFunction::JavascriptExternalFunction(DynamicType *type)
        : RuntimeFunction(type, &EntryInfo::ExternalFunctionThunk), nativeMethod(nullptr), signature(nullptr), callbackState(nullptr), initMethod(nullptr),
        oneBit(1), typeSlots(0), hasAccessors(0), flags(0), deferredLength(0)
    {
        DebugOnly(VerifyEntryPoint());
    }

    bool __cdecl JavascriptExternalFunction::DeferredLengthInitializer(DynamicObject * instance, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        Js::JavascriptLibrary::InitializeFunction<true, true, true>(instance, typeHandler, mode);

        JavascriptExternalFunction* object = static_cast<JavascriptExternalFunction*>(instance);

        object->UndeferLength(instance->GetScriptContext());

        return true;
    }

    // Note: non-constructors will probably use JavascriptFunction::InitiailizeFunction for undeferral.
    bool __cdecl JavascriptExternalFunction::DeferredConstructorInitializer(DynamicObject* instance, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode)
    {
        JavascriptExternalFunction* object = static_cast<JavascriptExternalFunction*>(instance);
        HRESULT hr = E_FAIL;

        ScriptContext* scriptContext = object->GetScriptContext();
        AnalysisAssert(scriptContext);
        // Don't call the implicit call if disable implicit call
        if (scriptContext->GetThreadContext()->IsDisableImplicitCall())
        {
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_External);
            //we will return if we get call further into implicitcalls.
            return false;
        }

        if (scriptContext->IsClosed() || scriptContext->IsInvalidatedForHostObjects())
        {
            Js::JavascriptError::MapAndThrowError(scriptContext, E_ACCESSDENIED);
        }
        ThreadContext* threadContext = scriptContext->GetThreadContext();

        typeHandler->Convert(instance, mode, object->typeSlots, object->hasAccessors);

        BEGIN_LEAVE_SCRIPT_INTERNAL(scriptContext)
        {
            ASYNC_HOST_OPERATION_START(threadContext);

            hr = object->initMethod(instance);

            ASYNC_HOST_OPERATION_END(threadContext);
        }
        END_LEAVE_SCRIPT_INTERNAL(scriptContext);

        if (FAILED(hr))
        {
            Js::JavascriptError::MapAndThrowError(scriptContext, hr);
        }

        JavascriptString * functionName = nullptr;
        if (object->GetFunctionName(&functionName))
        {
            object->SetPropertyWithAttributes(PropertyIds::name, functionName, PropertyConfigurable, nullptr);
        }
        return true;
    }

    void JavascriptExternalFunction::PrepareExternalCall(Js::Arguments * args)
    {
        ScriptContext * scriptContext = this->type->GetScriptContext();
        Assert(!scriptContext->GetThreadContext()->IsDisableImplicitException());
        scriptContext->VerifyAlive();

        Assert(scriptContext->GetThreadContext()->IsScriptActive());

        if (args->Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined);
        }

        Var &thisVar = args->Values[0];

        Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(thisVar);

        Js::RecyclableObject* directHostObject = nullptr;
        switch(typeId)
        {
        case TypeIds_Integer:
#if FLOATVAR
        case TypeIds_Number:
#endif // FLOATVAR
            Assert(!Js::VarIs<Js::RecyclableObject>(thisVar));
            break;
        default:
            {
                Assert(Js::VarIs<Js::RecyclableObject>(thisVar));

                ScriptContext* scriptContextThisVar = Js::VarTo<Js::RecyclableObject>(thisVar)->GetScriptContext();
                // We need to verify "this" pointer is active as well. The problem is that DOM prototype functions are
                // the same across multiple frames, and caller can do function.call(closedthis)
                Assert(!scriptContext->GetThreadContext()->IsDisableImplicitException());
                scriptContextThisVar->VerifyAlive();

                // translate direct host for fastDOM.
                switch(typeId)
                {
                case Js::TypeIds_GlobalObject:
                    {
                        Js::GlobalObject* srcGlobalObject = (Js::GlobalObject*)(void*)(thisVar);
                        directHostObject = srcGlobalObject->GetDirectHostObject();
                        // For jsrt, direct host object can be null. If thats the case don't change it.
                        if (directHostObject != nullptr)
                        {
                            thisVar = directHostObject;
                        }

                    }
                    break;
                case Js::TypeIds_Undefined:
                case Js::TypeIds_Null:
                    {
                        // Call to DOM function with this as "undefined" or "null"
                        // This should be converted to Global object
                        Js::GlobalObject* srcGlobalObject = scriptContextThisVar->GetGlobalObject() ;
                        directHostObject = srcGlobalObject->GetDirectHostObject();
                        // For jsrt, direct host object can be null. If thats the case don't change it.
                        if (directHostObject != nullptr)
                        {
                            thisVar = directHostObject;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    Var JavascriptExternalFunction::ExternalFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        JavascriptExternalFunction* externalFunction = static_cast<JavascriptExternalFunction*>(function);

        ScriptContext * scriptContext = externalFunction->type->GetScriptContext();

#ifdef ENABLE_DIRECTCALL_TELEMETRY
        DirectCallTelemetry::AutoLogger logger(scriptContext, externalFunction, &args);
#endif

        externalFunction->PrepareExternalCall(&args);

#if ENABLE_TTD
        Var result = nullptr;

        if(scriptContext->ShouldPerformRecordOrReplayAction())
        {
            result = JavascriptExternalFunction::HandleRecordReplayExternalFunction_Thunk(externalFunction, callInfo, args, scriptContext);
        }
        else
        {
            BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext)
            {
                // Don't do stack probe since BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION does that for us already
                result = externalFunction->nativeMethod(function, callInfo, args.Values);
            }
            END_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext);
        }
#else
        Var result = nullptr;
        BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext)
        {
            // Don't do stack probe since BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION does that for us already
            result = externalFunction->nativeMethod(function, callInfo, args.Values);
        }
        END_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext);
#endif

        if (result == nullptr)
        {
#pragma warning(push)
#pragma warning(disable:6011) // scriptContext cannot be null here
            result = scriptContext->GetLibrary()->GetUndefined();
#pragma warning(pop)
        }
        else
        {
            result = CrossSite::MarshalVar(scriptContext, result);
        }

        return result;
    }

    Var JavascriptExternalFunction::WrappedFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        JavascriptExternalFunction* externalFunction = static_cast<JavascriptExternalFunction*>(function);
        ScriptContext* scriptContext = externalFunction->type->GetScriptContext();
        Assert(!scriptContext->GetThreadContext()->IsDisableImplicitException());
        scriptContext->VerifyAlive();
        Assert(scriptContext->GetThreadContext()->IsScriptActive());

        // Make sure the callee knows we are a wrapped function thunk
        args.Info.Flags = (Js::CallFlags) (((int32) args.Info.Flags) | CallFlags_Wrapped);

        // don't need to leave script here, ExternalFunctionThunk will
        Assert(externalFunction->wrappedMethod->GetFunctionInfo()->GetOriginalEntryPoint() == JavascriptExternalFunction::ExternalFunctionThunk);
        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return JavascriptFunction::CallFunction<true>(externalFunction->wrappedMethod, externalFunction->wrappedMethod->GetEntryPoint(), args);
        }
        END_SAFE_REENTRANT_CALL
    }

    Var JavascriptExternalFunction::DefaultExternalFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        TypeId typeId = function->GetTypeId();
        rtErrors err = typeId <= TypeIds_UndefinedOrNull ? JSERR_NeedObject : JSERR_NeedFunction;
        JavascriptError::ThrowTypeError(function->GetScriptContext(), err);
    }

    Var JavascriptExternalFunction::StdCallExternalFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        JavascriptExternalFunction* externalFunction = static_cast<JavascriptExternalFunction*>(function);

        externalFunction->PrepareExternalCall(&args);

        ScriptContext * scriptContext = externalFunction->type->GetScriptContext();
        AnalysisAssert(scriptContext);

        if (args.Info.Count > USHORT_MAX)
        {
            // Due to compat reasons, stdcall external functions expect a ushort count of args.
            // To support more than this we will need a new API.
            Js::JavascriptError::ThrowTypeError(scriptContext, JSERR_ArgListTooLarge);
        }

        Var result = nullptr;
        Assert(callInfo.Count > 0);

        StdCallJavascriptMethodInfo info = {
            args[0],
            args.HasNewTarget() ? args.GetNewTarget() : args.IsNewCall() ? function : scriptContext->GetLibrary()->GetUndefined(),
            args.IsNewCall()
        };

#if ENABLE_TTD
        if(scriptContext->ShouldPerformRecordOrReplayAction())
        {
            result = JavascriptExternalFunction::HandleRecordReplayExternalFunction_StdThunk(function, callInfo, args, scriptContext);
        }
        else
        {
            BEGIN_LEAVE_SCRIPT(scriptContext)
            {
                result = externalFunction->stdCallNativeMethod(function, args.Values, static_cast<USHORT>(args.Info.Count), &info, externalFunction->callbackState);
            }
            END_LEAVE_SCRIPT(scriptContext);
        }
#else
        BEGIN_LEAVE_SCRIPT(scriptContext)
        {
            result = externalFunction->stdCallNativeMethod(function, args.Values, static_cast<USHORT>(args.Info.Count), &info, externalFunction->callbackState);
        }
        END_LEAVE_SCRIPT(scriptContext);
#endif

        bool marshallingMayBeNeeded = false;
        if (result != nullptr)
        {
            marshallingMayBeNeeded = Js::VarIs<Js::RecyclableObject>(result);
            if (marshallingMayBeNeeded)
            {
            Js::RecyclableObject * obj = Js::VarTo<Js::RecyclableObject>(result);

            // For JSRT, we could get result marshalled in different context.
            bool isJSRT = scriptContext->GetThreadContext()->IsJSRT();
                marshallingMayBeNeeded = obj->GetScriptContext() != scriptContext;
                if (!isJSRT && marshallingMayBeNeeded)
            {
                Js::Throw::InternalError();
            }
        }
        }

        if (scriptContext->HasRecordedException())
        {
            bool considerPassingToDebugger = false;
            JavascriptExceptionObject* recordedException = scriptContext->GetAndClearRecordedException(&considerPassingToDebugger);
            if (recordedException != nullptr)
            {
                // If this is script termination, then throw ScriptAbortExceptio, else throw normal Exception object.
                if (recordedException == scriptContext->GetThreadContext()->GetPendingTerminatedErrorObject())
                {
                    throw Js::ScriptAbortException();
                }
                else
                {
                    JavascriptExceptionOperators::RethrowExceptionObject(recordedException, scriptContext, considerPassingToDebugger);
                }
            }
        }

        if (result == nullptr)
        {
            result = scriptContext->GetLibrary()->GetUndefined();
        }
        else if (marshallingMayBeNeeded)
        {
            result = CrossSite::MarshalVar(scriptContext, result);
        }

        return result;
    }

    BOOL JavascriptExternalFunction::SetLengthProperty(Var length)
    {
        return DynamicObject::SetPropertyWithAttributes(PropertyIds::length, length, PropertyConfigurable, NULL, PropertyOperation_None, SideEffects_None);
    }

    void JavascriptExternalFunction::UndeferLength(ScriptContext *scriptContext)
    {
        if (deferredLength > 0)
        {
            SetLengthProperty(Js::JavascriptNumber::ToVar(deferredLength, scriptContext));
            deferredLength = 0;
        }
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType JavascriptExternalFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapExternalFunctionObject;
    }

    void JavascriptExternalFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::TTDVar fnameId = TTD_CONVERT_JSVAR_TO_TTDVAR(this->functionNameId);
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::TTDVar, TTD::NSSnapObjects::SnapObjectType::SnapExternalFunctionObject>(objData, fnameId);
    }

    Var JavascriptExternalFunction::HandleRecordReplayExternalFunction_Thunk(Js::JavascriptFunction* function, CallInfo& callInfo, Arguments& args, ScriptContext* scriptContext)
    {
        JavascriptExternalFunction* externalFunction = static_cast<JavascriptExternalFunction*>(function);

        Var result = nullptr;

        if(scriptContext->ShouldPerformReplayAction())
        {
            TTD::TTDNestingDepthAutoAdjuster logPopper(scriptContext->GetThreadContext());
            scriptContext->GetThreadContext()->TTDLog->ReplayExternalCallEvent(externalFunction, args, &result);
        }
        else
        {
            TTDAssert(scriptContext->ShouldPerformRecordAction(), "Check either record/replay before calling!!!");

            TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;

            TTD::TTDNestingDepthAutoAdjuster logPopper(scriptContext->GetThreadContext());
            TTD::NSLogEvents::EventLogEntry* callEvent = elog->RecordExternalCallEvent(externalFunction, scriptContext->GetThreadContext()->TTDRootNestingCount, args, false);

            BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext)
            {
                // Don't do stack probe since BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION does that for us already
                result = externalFunction->nativeMethod(function, callInfo, args.Values);
            }
            END_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext);

            //Exceptions should be prohibited so no need to do extra work
            elog->RecordExternalCallEvent_Complete(externalFunction, callEvent, result);
        }

        return result;
    }

    Var JavascriptExternalFunction::HandleRecordReplayExternalFunction_StdThunk(Js::RecyclableObject* function, CallInfo& callInfo, Arguments& args, ScriptContext* scriptContext)
    {
        JavascriptExternalFunction* externalFunction = static_cast<JavascriptExternalFunction*>(function);

        Var result = nullptr;

        if(scriptContext->ShouldPerformReplayAction())
        {
            TTD::TTDNestingDepthAutoAdjuster logPopper(scriptContext->GetThreadContext());
            scriptContext->GetThreadContext()->TTDLog->ReplayExternalCallEvent(externalFunction, args, &result);
        }
        else
        {
            if (args.Info.Count > USHORT_MAX)
            {
                // Due to compat reasons, stdcall external functions expect a ushort count of args.
                // To support more than this we will need a new API.
                Js::JavascriptError::ThrowTypeError(scriptContext, JSERR_ArgListTooLarge);
            }

            TTDAssert(scriptContext->ShouldPerformRecordAction(), "Check either record/replay before calling!!!");

            TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;

            TTD::TTDNestingDepthAutoAdjuster logPopper(scriptContext->GetThreadContext());
            TTD::NSLogEvents::EventLogEntry* callEvent = elog->RecordExternalCallEvent(externalFunction, scriptContext->GetThreadContext()->TTDRootNestingCount, args, true);

            StdCallJavascriptMethodInfo info = {
                args[0],
                args.HasNewTarget() ? args.GetNewTarget() : args.IsNewCall() ? function : scriptContext->GetLibrary()->GetUndefined(),
                args.IsNewCall()
            };

            BEGIN_LEAVE_SCRIPT(scriptContext)
            {
                result = externalFunction->stdCallNativeMethod(function, args.Values, static_cast<ushort>(args.Info.Count), &info, externalFunction->callbackState);
            }
            END_LEAVE_SCRIPT(scriptContext);

            elog->RecordExternalCallEvent_Complete(externalFunction, callEvent, result);
        }

        return result;
    }

    Var __stdcall JavascriptExternalFunction::TTDReplayDummyExternalMethod(Var callee, Var *args, USHORT cargs, StdCallJavascriptMethodInfo *info, void *callbackState)
    {
        JavascriptExternalFunction* externalFunction = static_cast<JavascriptExternalFunction*>(callee);

        ScriptContext* scriptContext = externalFunction->type->GetScriptContext();
        TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;
        TTDAssert(elog != nullptr, "How did this get created then???");

        //If this flag is set then this is ok (the debugger may be evaluating this so just return undef -- otherwise this is an error
        if(!elog->IsDebugModeFlagSet())
        {
            TTDAssert(false, "This should never be reached in pure replay mode!!!");
            return nullptr;
        }

        return scriptContext->GetLibrary()->GetUndefined();
    }
#endif
}
