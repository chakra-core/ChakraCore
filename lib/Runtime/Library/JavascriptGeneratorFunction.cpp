//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    FunctionInfo JavascriptGeneratorFunction::functionInfo(
        FORCE_NO_WRITE_BARRIER_TAG(JavascriptGeneratorFunction::EntryGeneratorFunctionImplementation),
        (FunctionInfo::Attributes)(FunctionInfo::DoNotProfile | FunctionInfo::ErrorOnNew));
    FunctionInfo JavascriptAsyncFunction::functionInfo(
        FORCE_NO_WRITE_BARRIER_TAG(JavascriptGeneratorFunction::EntryAsyncFunctionImplementation),
        (FunctionInfo::Attributes)(FunctionInfo::DoNotProfile | FunctionInfo::ErrorOnNew));

    JavascriptGeneratorFunction::JavascriptGeneratorFunction(DynamicType* type)
        : ScriptFunctionBase(type, &functionInfo),
        scriptFunction(nullptr)
    {
        // Constructor used during copy on write.
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptGeneratorFunction::JavascriptGeneratorFunction(DynamicType* type, GeneratorVirtualScriptFunction* scriptFunction)
        : ScriptFunctionBase(type, &functionInfo),
        scriptFunction(scriptFunction)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptGeneratorFunction::JavascriptGeneratorFunction(DynamicType* type, FunctionInfo* functionInfo, GeneratorVirtualScriptFunction* scriptFunction)
        : ScriptFunctionBase(type, functionInfo),
        scriptFunction(scriptFunction)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptAsyncFunction::JavascriptAsyncFunction(DynamicType* type, GeneratorVirtualScriptFunction* scriptFunction)
        : JavascriptGeneratorFunction(type, &functionInfo, scriptFunction)
    {
        DebugOnly(VerifyEntryPoint());
    }

    JavascriptAsyncFunction* JavascriptAsyncFunction::New(ScriptContext* scriptContext, GeneratorVirtualScriptFunction* scriptFunction)
    {
        return scriptContext->GetLibrary()->CreateAsyncFunction(functionInfo.GetOriginalEntryPoint(), scriptFunction);
    }

    bool JavascriptGeneratorFunction::Is(Var var)
    {
        if (JavascriptFunction::Is(var))
        {
            JavascriptFunction* obj = JavascriptFunction::FromVar(var);

            return VirtualTableInfo<JavascriptGeneratorFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptGeneratorFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptGeneratorFunction* JavascriptGeneratorFunction::FromVar(Var var)
    {
        Assert(JavascriptGeneratorFunction::Is(var) || JavascriptAsyncFunction::Is(var));

        return static_cast<JavascriptGeneratorFunction*>(var);
    }

    bool JavascriptAsyncFunction::Is(Var var)
    {
        if (JavascriptFunction::Is(var))
        {
            JavascriptFunction* obj = JavascriptFunction::FromVar(var);

            return VirtualTableInfo<JavascriptAsyncFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptAsyncFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptAsyncFunction* JavascriptAsyncFunction::FromVar(Var var)
    {
        Assert(JavascriptAsyncFunction::Is(var));

        return static_cast<JavascriptAsyncFunction*>(var);
    }

    JavascriptGeneratorFunction* JavascriptGeneratorFunction::OP_NewScGenFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef)
    {
        FunctionProxy* functionProxy = (*infoRef)->GetFunctionProxy();
        ScriptContext* scriptContext = functionProxy->GetScriptContext();

        bool hasSuperReference = functionProxy->HasSuperReference();

        GeneratorVirtualScriptFunction* scriptFunction = scriptContext->GetLibrary()->CreateGeneratorVirtualScriptFunction(functionProxy);
        scriptFunction->SetEnvironment(environment);
        scriptFunction->SetHasSuperReference(hasSuperReference);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_FUNCTION(scriptFunction, EtwTrace::GetFunctionId(functionProxy)));

        JavascriptGeneratorFunction* genFunc =
            functionProxy->IsAsync()
            ? JavascriptAsyncFunction::New(scriptContext, scriptFunction)
            : scriptContext->GetLibrary()->CreateGeneratorFunction(functionInfo.GetOriginalEntryPoint(), scriptFunction);

        scriptFunction->SetRealGeneratorFunction(genFunc);

        return genFunc;
    }

    Var JavascriptGeneratorFunction::EntryGeneratorFunctionImplementation(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(stackArgs, callInfo);

        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptGeneratorFunction* generatorFunction = JavascriptGeneratorFunction::FromVar(function);

        // InterpreterStackFrame takes a pointer to the args, so copy them to the recycler heap
        // and use that buffer for this InterpreterStackFrame.
        Field(Var)* argsHeapCopy = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), stackArgs.Info.Count);
        CopyArray(argsHeapCopy, stackArgs.Info.Count, stackArgs.Values, stackArgs.Info.Count);
        Arguments heapArgs(callInfo, (Var*)argsHeapCopy);

        DynamicObject* prototype = scriptContext->GetLibrary()->CreateGeneratorConstructorPrototypeObject();
        JavascriptGenerator* generator = scriptContext->GetLibrary()->CreateGenerator(heapArgs, generatorFunction->scriptFunction, prototype);
        // Set the prototype from constructor
        JavascriptOperators::OrdinaryCreateFromConstructor(function, generator, prototype, scriptContext);

        // Call a next on the generator to execute till the beginning of the body
        CALL_ENTRYPOINT(scriptContext->GetThreadContext(), generator->EntryNext, function, CallInfo(CallFlags_Value, 1), generator);

        return generator;
    }

    Var JavascriptGeneratorFunction::EntryAsyncFunctionImplementation(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(stackArgs, callInfo);

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        RecyclableObject* prototype = scriptContext->GetLibrary()->GetNull();

        // InterpreterStackFrame takes a pointer to the args, so copy them to the recycler heap
        // and use that buffer for this InterpreterStackFrame.
        Field(Var)* argsHeapCopy = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), stackArgs.Info.Count);
        CopyArray(argsHeapCopy, stackArgs.Info.Count, stackArgs.Values, stackArgs.Info.Count);
        Arguments heapArgs(callInfo, (Var*)argsHeapCopy);

        JavascriptExceptionObject* e = nullptr;
        JavascriptPromiseResolveOrRejectFunction* resolve;
        JavascriptPromiseResolveOrRejectFunction* reject;
        JavascriptPromiseAsyncSpawnExecutorFunction* executor =
            library->CreatePromiseAsyncSpawnExecutorFunction(
                JavascriptPromise::EntryJavascriptPromiseAsyncSpawnExecutorFunction,
                scriptContext->GetLibrary()->CreateGenerator(heapArgs, JavascriptAsyncFunction::FromVar(function)->GetGeneratorVirtualScriptFunction(), prototype),
                stackArgs[0]);

        JavascriptPromise* promise = library->CreatePromise();
        JavascriptPromise::InitializePromise(promise, &resolve, &reject, scriptContext);

        try
        {
            CALL_FUNCTION(scriptContext->GetThreadContext(), executor, CallInfo(CallFlags_Value, 3), library->GetUndefined(), resolve, reject);
        }
        catch (const JavascriptException& err)
        {
            e = err.GetAndClear();
        }

        if (e != nullptr)
        {
            JavascriptPromise::TryRejectWithExceptionObject(e, reject, scriptContext);
        }

        return promise;
    }

    Var JavascriptGeneratorFunction::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Get called when creating a new generator function through the constructor (e.g. gf.__proto__.constructor) and sets EntryGeneratorFunctionImplementation as the entrypoint
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return JavascriptFunction::NewInstanceHelper(function->GetScriptContext(), function, callInfo, args, FunctionKind::Generator);
    }

    Var JavascriptGeneratorFunction::NewInstanceRestrictedMode(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        scriptContext->CheckEvalRestriction();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return JavascriptFunction::NewInstanceHelper(scriptContext, function, callInfo, args, FunctionKind::Generator);
    }

    JavascriptString* JavascriptGeneratorFunction::GetDisplayNameImpl() const
    {
        return scriptFunction->GetDisplayNameImpl();
    }

    Var JavascriptGeneratorFunction::GetHomeObj() const
    {
        return scriptFunction->GetHomeObj();
    }

    void JavascriptGeneratorFunction::SetHomeObj(Var homeObj)
    {
        scriptFunction->SetHomeObj(homeObj);
    }

    void JavascriptGeneratorFunction::SetComputedNameVar(Var computedNameVar)
    {
        scriptFunction->SetComputedNameVar(computedNameVar);
    }

    Var JavascriptGeneratorFunction::GetComputedNameVar() const
    {
        return scriptFunction->GetComputedNameVar();
    }

    bool JavascriptGeneratorFunction::IsAnonymousFunction() const
    {
        return scriptFunction->IsAnonymousFunction();
    }

    Var JavascriptGeneratorFunction::GetSourceString() const
    {
        return scriptFunction->GetSourceString();
    }

    Var JavascriptGeneratorFunction::EnsureSourceString()
    {
        return scriptFunction->EnsureSourceString();
    }

    PropertyQueryFlags JavascriptGeneratorFunction::HasPropertyQuery(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return PropertyQueryFlags::Property_Found;
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::HasPropertyQuery(propertyId);
        }

        return JavascriptFunction::HasPropertyQuery(propertyId);
    }

    PropertyQueryFlags JavascriptGeneratorFunction::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        if (GetPropertyBuiltIns(originalInstance, propertyId, value, info, requestContext, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
        }

        return JavascriptFunction::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags JavascriptGeneratorFunction::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr)
        {
            BOOL result;
            if (GetPropertyBuiltIns(originalInstance, propertyRecord->GetPropertyId(), value, info, requestContext, &result))
            {
                return JavascriptConversion::BooleanToPropertyQueryFlags(result);
            }

            if (propertyRecord->GetPropertyId() == PropertyIds::caller || propertyRecord->GetPropertyId() == PropertyIds::arguments)
            {
                // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
                return DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
            }
        }

        return JavascriptFunction::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
    }

    bool JavascriptGeneratorFunction::GetPropertyBuiltIns(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext, BOOL* result)
    {
        if (propertyId == PropertyIds::length)
        {
            // Cannot just call the base GetProperty for `length` because we need
            // to get the length from our private ScriptFunction instead of ourself.
            int len = 0;
            Var varLength;
            if (scriptFunction->GetProperty(scriptFunction, PropertyIds::length, &varLength, NULL, requestContext))
            {
                len = JavascriptConversion::ToInt32(varLength, requestContext);
            }

            *value = JavascriptNumber::ToVar(len, requestContext);
            *result = true;
            return true;
        }

        return false;
    }

    PropertyQueryFlags JavascriptGeneratorFunction::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptGeneratorFunction::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    BOOL JavascriptGeneratorFunction::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        BOOL result;
        if (SetPropertyBuiltIns(propertyId, value, flags, info, &result))
        {
            return result;
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::SetProperty(propertyId, value, flags, info);
        }

        return JavascriptFunction::SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptGeneratorFunction::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr)
        {
            BOOL result;
            if (SetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, flags, info, &result))
            {
                return result;
            }

            if (propertyRecord->GetPropertyId() == PropertyIds::caller || propertyRecord->GetPropertyId() == PropertyIds::arguments)
            {
                // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
                return DynamicObject::SetProperty(propertyNameString, value, flags, info);
            }
        }

        return JavascriptFunction::SetProperty(propertyNameString, value, flags, info);
    }

    bool JavascriptGeneratorFunction::SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info, BOOL* result)
    {
        if (propertyId == PropertyIds::length)
        {
            JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());

            *result = false;
            return true;
        }

        return false;
    }

    BOOL JavascriptGeneratorFunction::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        if (propertyId == PropertyIds::length)
        {
            return this->scriptFunction->SetAccessors(propertyId, getter, setter, flags);
        }

        return JavascriptFunction::SetAccessors(propertyId, getter, setter, flags);
    }

    BOOL JavascriptGeneratorFunction::GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext)
    {
        if (propertyId == PropertyIds::length)
        {
            return this->scriptFunction->GetAccessors(propertyId, getter, setter, requestContext);
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::GetAccessors(propertyId, getter, setter, requestContext);
        }

        return JavascriptFunction::GetAccessors(propertyId, getter, setter, requestContext);
    }

    DescriptorFlags JavascriptGeneratorFunction::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::GetSetter(propertyId, setterValue, info, requestContext);
        }

        if (propertyId == PropertyIds::length)
        {
            return this->scriptFunction->GetSetter(propertyId, setterValue, info, requestContext);
        }

        return JavascriptFunction::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags JavascriptGeneratorFunction::GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr)
        {
            if (propertyRecord->GetPropertyId() == PropertyIds::length)
            {
                return this->scriptFunction->GetSetter(propertyNameString, setterValue, info, requestContext);
            }

            if ((propertyRecord->GetPropertyId() == PropertyIds::caller || propertyRecord->GetPropertyId() == PropertyIds::arguments))
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::GetSetter(propertyNameString, setterValue, info, requestContext);
        }
        }

        return JavascriptFunction::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    BOOL JavascriptGeneratorFunction::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return SetProperty(propertyId, value, PropertyOperation_None, info);
    }

    BOOL JavascriptGeneratorFunction::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::DeleteProperty(propertyId, flags);
        }

        return JavascriptFunction::DeleteProperty(propertyId, flags);
    }

    BOOL JavascriptGeneratorFunction::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        if (BuiltInPropertyRecords::length.Equals(propertyName))
        {
            return false;
        }

        if (BuiltInPropertyRecords::caller.Equals(propertyName) || BuiltInPropertyRecords::arguments.Equals(propertyName))
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::DeleteProperty(propertyNameString, flags);
        }

        return JavascriptFunction::DeleteProperty(propertyNameString, flags);
    }

    BOOL JavascriptGeneratorFunction::IsWritable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::IsWritable(propertyId);
        }

        return JavascriptFunction::IsWritable(propertyId);
    }

    BOOL JavascriptGeneratorFunction::IsEnumerable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }

        if (propertyId == PropertyIds::caller || propertyId == PropertyIds::arguments)
        {
            // JavascriptFunction has special case for caller and arguments; call DynamicObject:: virtual directly to skip that.
            return DynamicObject::IsEnumerable(propertyId);
        }

        return JavascriptFunction::IsEnumerable(propertyId);
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType JavascriptGeneratorFunction::GetSnapTag_TTD() const
    {
        //we override this with invalid to make sure it isn't unexpectedly handled by the parent class
        return TTD::NSSnapObjects::SnapObjectType::Invalid;
    }

    void JavascriptGeneratorFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(false, "Invalid -- JavascriptGeneratorFunction");
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptAsyncFunction::GetSnapTag_TTD() const
    {
        //we override this with invalid to make sure it isn't unexpectedly handled by the parent class
        return TTD::NSSnapObjects::SnapObjectType::Invalid;
    }

    void JavascriptAsyncFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(false, "Invalid -- JavascriptAsyncFunction");
    }

    TTD::NSSnapObjects::SnapObjectType GeneratorVirtualScriptFunction::GetSnapTag_TTD() const
    {
        //we override this with invalid to make sure it isn't unexpectedly handled by the parent class
        return TTD::NSSnapObjects::SnapObjectType::Invalid;
    }

    void GeneratorVirtualScriptFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(false, "Invalid -- GeneratorVirtualScriptFunction");
    }
#endif
}
