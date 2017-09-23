//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    ExternalIteratorCreatorFunction::ExternalIteratorCreatorFunction(DynamicType* type,
        FunctionInfo* functionInfo,
        JavascriptTypeId typeId,
        uint byteCount,
        Var prototypeForIterator, InitIteratorFunction initFunction, NextFunction nextFunction)
        : RuntimeFunction(type, functionInfo), m_externalTypeId(typeId), m_extraByteCount(byteCount),
        m_prototypeForIterator(prototypeForIterator), m_initFunction(initFunction), m_nextFunction(nextFunction)
    {
    }

    void ExternalIteratorCreatorFunction::ThrowIfNotValidObject(Var instance)
    {
        JavascriptTypeId typeId = (JavascriptTypeId)Js::JavascriptOperators::GetTypeId(instance);
        if (typeId != m_externalTypeId || !RecyclableObject::Is(m_prototypeForIterator))
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_InvalidIterableObject);
        }
    }

    Var ExternalIteratorCreatorFunction::CreateFunction(JavascriptLibrary *library,
        JavascriptTypeId typeId,
        JavascriptMethod entryPoint,
        uint byteCount,
        Var prototypeForIterator, InitIteratorFunction initFunction, NextFunction nextFunction)
    {
        FunctionInfo* functionInfo = RecyclerNew(library->GetRecycler(), FunctionInfo, entryPoint);
        DynamicType* type = library->CreateDeferredPrototypeFunctionType(entryPoint);
        ExternalIteratorCreatorFunction* function = RecyclerNewEnumClass(library->GetRecycler(),
            EnumClass_1_Bit,
            ExternalIteratorCreatorFunction,
            type,
            functionInfo,
            typeId,
            byteCount,
            prototypeForIterator, initFunction, nextFunction);

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);
        return function;
    }

    Var ExternalIteratorCreatorFunction::CreateCustomExternalIterator(Var instance, ExternalIteratorCreatorFunction* function, ExternalIteratorKind kind)
    {
        Assert(function != nullptr);
        function->ThrowIfNotValidObject(instance);
        ScriptContext *scriptContext = function->GetScriptContext();

        AssertOrFailFast(RecyclableObject::Is(function->m_prototypeForIterator));
        DynamicObject *prototype = static_cast<DynamicObject*>(PointerValue(function->m_prototypeForIterator));
        Js::DynamicType *type = scriptContext->GetLibrary()->CreateObjectTypeNoCache(prototype, TypeIds_ExternalIterator);

        AssertOrFailFast(function->m_extraByteCount >= sizeof(void*));

        CustomExternalIterator *iterator = RecyclerNewPlus(scriptContext->GetRecycler(),
            function->m_extraByteCount,
            CustomExternalIterator,
            type,
            kind,
            function->m_externalTypeId,
            function->m_nextFunction);

        AssertOrFailFast(function->m_initFunction != nullptr);
        BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext)
        {
            function->m_initFunction(instance, (Var)iterator);
        }
        END_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext);

        return iterator;
    }

    Var ExternalIteratorCreatorFunction::EntryExternalEntries(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        if (args.Info.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(function->GetScriptContext(), JSERR_ErrorOnNew);
        }

        ExternalIteratorCreatorFunction* iteratorFunction = static_cast<ExternalIteratorCreatorFunction*>(function);
        return CreateCustomExternalIterator(args[0], iteratorFunction, ExternalIteratorKind::External_KeyAndValue);
    }

    Var ExternalIteratorCreatorFunction::EntryExternalKeys(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        if (args.Info.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(function->GetScriptContext(), JSERR_ErrorOnNew);
        }

        ExternalIteratorCreatorFunction* iteratorFunction = static_cast<ExternalIteratorCreatorFunction*>(function);
        return CreateCustomExternalIterator(args[0], iteratorFunction, ExternalIteratorKind::External_Keys);
    }

    Var ExternalIteratorCreatorFunction::EntryExternalValues(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        if (args.Info.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(function->GetScriptContext(), JSERR_ErrorOnNew);
        }

        ExternalIteratorCreatorFunction* iteratorFunction = static_cast<ExternalIteratorCreatorFunction*>(function);
        return CreateCustomExternalIterator(args[0], iteratorFunction, ExternalIteratorKind::External_Values);
    }

    JavascriptExternalIteratorNextFunction::JavascriptExternalIteratorNextFunction(DynamicType* type,
        FunctionInfo* functionInfo,
        JavascriptTypeId typeId)
        : RuntimeFunction(type, functionInfo), m_externalTypeId(typeId)
    { }

    JavascriptExternalIteratorNextFunction* JavascriptExternalIteratorNextFunction::CreateFunction(JavascriptLibrary *library, JavascriptTypeId typeId, JavascriptMethod entryPoint)
    {
        FunctionInfo* functionInfo = RecyclerNew(library->GetRecycler(), FunctionInfo, entryPoint);
        DynamicType* type = library->CreateDeferredPrototypeFunctionType(entryPoint);
        JavascriptExternalIteratorNextFunction* function = RecyclerNewEnumClass(library->GetRecycler(), EnumClass_1_Bit, JavascriptExternalIteratorNextFunction, type, functionInfo, typeId);

        function->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(0), PropertyConfigurable, nullptr);
        return function;
    }

    CustomExternalIterator::CustomExternalIterator(DynamicType* type, ExternalIteratorKind kind, JavascriptTypeId typeId, NextFunction nextFunction) :
        DynamicObject(type),
        m_kind(kind),
        m_externalTypeId(typeId),
        m_nextFunction(nextFunction)
    {
        Assert(type->GetTypeId() == TypeIds_ExternalIterator);
    }

    bool CustomExternalIterator::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return typeId == TypeIds_ExternalIterator;
    }

    CustomExternalIterator* CustomExternalIterator::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'ExternalIterator'");
        return static_cast<CustomExternalIterator *>(RecyclableObject::FromVar(aValue));
    }

    Var CustomExternalIterator::CreateNextFunction(JavascriptLibrary *library, JavascriptTypeId typeId)
    {
        return JavascriptExternalIteratorNextFunction::CreateFunction(library, typeId, CustomExternalIterator::EntryNext);
    }

    Var CustomExternalIterator::EntryNext(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        if (args.Info.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ErrorOnNew);
        }

        JavascriptExternalIteratorNextFunction* iteratorNextFunction = static_cast<JavascriptExternalIteratorNextFunction*>(function);

        Var thisObj = args[0];

        if (!CustomExternalIterator::Is(thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidIteratorObject, _u("Iterator.prototype.next"));
        }

        CustomExternalIterator * currentIterator = CustomExternalIterator::FromVar(thisObj);
        if (iteratorNextFunction->GetExternalTypeId() != currentIterator->m_externalTypeId)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidIteratorObject, _u("Iterator.prototype.next"));
        }

        Var key = nullptr;
        Var value = nullptr;
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (currentIterator->m_nextFunction == nullptr)
        {
            return library->CreateIteratorResultObjectUndefinedTrue();
        }

        bool ret = false;
        BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext)
        {
            ret = currentIterator->m_nextFunction(currentIterator, &key, &value);
        }
        END_LEAVE_SCRIPT_WITH_EXCEPTION(scriptContext);

        if (!ret)
        {
            return library->CreateIteratorResultObjectUndefinedTrue();
        }

        Var result;
        if (currentIterator->m_kind == ExternalIteratorKind::External_KeyAndValue)
        {
            JavascriptArray* keyValueTuple = library->CreateArray(2);
            keyValueTuple->SetItem(0, key, PropertyOperation_None);
            keyValueTuple->SetItem(1, value, PropertyOperation_None);
            result = keyValueTuple;
        }
        else if (currentIterator->m_kind == ExternalIteratorKind::External_Keys)
        {
            result = key;
        }
        else
        {
            Assert(currentIterator->m_kind == ExternalIteratorKind::External_Values);
            result = value;
        }

        return library->CreateIteratorResultObjectValueFalse(result);
    }
}
