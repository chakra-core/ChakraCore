//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM

namespace Js
{

WebAssemblyTable::WebAssemblyTable(Var * values, uint32 length, uint32 maxLength, DynamicType * type) :
    DynamicObject(type),
    m_values(values),
    m_length(length),
    m_maxLength(maxLength)
{
}

/* static */
bool
WebAssemblyTable::Is(Var value)
{
    return JavascriptOperators::GetTypeId(value) == TypeIds_WebAssemblyTable;
}

/* static */
WebAssemblyTable *
WebAssemblyTable::FromVar(Var value)
{
    Assert(WebAssemblyTable::Is(value));
    return static_cast<WebAssemblyTable*>(value);
}

Var
WebAssemblyTable::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
    bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
    Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);

    if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew);
    }

    if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
    }
    DynamicObject * memoryDescriptor = JavascriptObject::FromVar(args[1]);

    PropertyRecord const * elementPropRecord = nullptr;
    scriptContext->GetOrAddPropertyRecord(_u("element"), lstrlen(_u("element")), &elementPropRecord);
    Var elementVar = JavascriptOperators::OP_GetProperty(memoryDescriptor, elementPropRecord->GetPropertyId(), scriptContext);
    if (!JavascriptOperators::StrictEqualString(elementVar, scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("anyfunc"))))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_ExpectedAnyFunc);
    }

    PropertyRecord const * initPropRecord = nullptr;
    scriptContext->GetOrAddPropertyRecord(_u("initial"), lstrlen(_u("initial")), &initPropRecord);
    Var initVar = JavascriptOperators::OP_GetProperty(memoryDescriptor, initPropRecord->GetPropertyId(), scriptContext);
    uint32 initial = WebAssembly::ToNonWrappingUint32(initVar, scriptContext);

    PropertyRecord const * maxPropRecord = nullptr;
    scriptContext->GetOrAddPropertyRecord(_u("maximum"), lstrlen(_u("maximum")), &maxPropRecord);
    uint32 maximum = UINT_MAX;
    if (JavascriptOperators::OP_HasProperty(memoryDescriptor, maxPropRecord->GetPropertyId(), scriptContext))
    {
        Var maxVar = JavascriptOperators::OP_GetProperty(memoryDescriptor, initPropRecord->GetPropertyId(), scriptContext);
        maximum = WebAssembly::ToNonWrappingUint32(maxVar, scriptContext);
    }
    return Create(initial, maximum, scriptContext);
}

Var
WebAssemblyTable::EntryGetterLength(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count == 0 || !WebAssemblyTable::Is(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedTableObject);
    }
    WebAssemblyTable * table = WebAssemblyTable::FromVar(args[0]);
    return JavascriptNumber::ToVar(table->m_length, scriptContext);
}

Var
WebAssemblyTable::EntryGrow(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count == 0 || !WebAssemblyTable::Is(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedTableObject);
    }
    WebAssemblyTable * table = WebAssemblyTable::FromVar(args[0]);

    Var deltaVar = scriptContext->GetLibrary()->GetUndefined();
    if (args.Info.Count >= 2)
    {
        deltaVar = args[1];
    }
    uint32 delta = WebAssembly::ToNonWrappingUint32(deltaVar, scriptContext);
    if ((uint64)table->m_length + delta > table->m_maxLength)
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }
    CompileAssert(sizeof(m_maxLength) == sizeof(uint32));

    uint32 newLength = table->m_length + delta;
    Var * newValues = RecyclerNewArrayZ(scriptContext->GetRecycler(), Var, newLength);
    memcpy_s(newValues, newLength, table->m_values, table->m_length);

    table->m_values = newValues;
    table->m_length = newLength;

    return scriptContext->GetLibrary()->GetUndefined();
}

Var
WebAssemblyTable::EntryGet(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count == 0 || !WebAssemblyTable::Is(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedTableObject);
    }
    WebAssemblyTable * table = WebAssemblyTable::FromVar(args[0]);

    Var indexVar = scriptContext->GetLibrary()->GetUndefined();
    if (args.Info.Count >= 2)
    {
        indexVar = args[1];
    }
    uint32 index = WebAssembly::ToNonWrappingUint32(indexVar, scriptContext);
    if (index > table->m_length)
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }
    if (!table->m_values[index])
    {
        return scriptContext->GetLibrary()->GetNull();
    }

    return table->m_values[index];
}

Var
WebAssemblyTable::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count == 0 || !WebAssemblyTable::Is(args[0]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedTableObject);
    }
    WebAssemblyTable * table = WebAssemblyTable::FromVar(args[0]);

    if (args.Info.Count < 3)
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedWebAssemblyFunc);
    }
    Var indexVar = args[1];
    Var value = args[2];
    
    if (JavascriptOperators::IsNull(value))
    {
        value = nullptr;
    }
    else if (!AsmJsScriptFunction::Is(args[2]) || !AsmJsScriptFunction::FromVar(args[2])->GetFunctionBody()->IsWasmFunction())
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedWebAssemblyFunc);
    }

    uint32 index = WebAssembly::ToNonWrappingUint32(indexVar, scriptContext);
    if (index > table->m_length)
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }

    table->m_values[index] = value;

    return scriptContext->GetLibrary()->GetUndefined();
}

WebAssemblyTable *
WebAssemblyTable::Create(uint32 initial, uint32 maximum, ScriptContext * scriptContext)
{
    // TODO: implement SHOULD clause and try to reserve maximum to avoid having to copy on grow
    Var * values = nullptr;
    if (initial > 0)
    {
        values = RecyclerNewArrayZ(scriptContext->GetRecycler(), Var, initial);
    }
    return RecyclerNew(scriptContext->GetRecycler(), WebAssemblyTable, values, initial, maximum, scriptContext->GetLibrary()->GetWebAssemblyTableType());
}

void
WebAssemblyTable::DirectSetValue(uint index, Var val)
{
    Assert(index < m_length);
    Assert(!val || AsmJsScriptFunction::Is(val));
    m_values[index] = val;
}

Var
WebAssemblyTable::DirectGetValue(uint index) const
{
    Assert(index < m_length);
    Var val = m_values[index];
    Assert(!val || AsmJsScriptFunction::Is(val));
    return val;
}

Var *
WebAssemblyTable::GetValues() const
{
    return m_values;
}
uint32
WebAssemblyTable::GetLength() const
{
    return m_length;
}

} // namespace Js

#endif // ENABLE_WASM
