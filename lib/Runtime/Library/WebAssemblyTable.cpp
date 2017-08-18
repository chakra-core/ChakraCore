//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "WasmLimits.h"

namespace Js
{

WebAssemblyTable::WebAssemblyTable(Var * values, uint32 currentLength, uint32 initialLength, uint32 maxLength, DynamicType * type) :
    DynamicObject(type),
    m_values((Field(Var)*)values),
    m_currentLength(currentLength),
    m_initialLength(initialLength),
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
        JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("WebAssembly.Table"));
    }

    if (args.Info.Count < 2 || !JavascriptOperators::IsObject(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("tableDescriptor"));
    }
    DynamicObject * tableDescriptor = JavascriptObject::FromVar(args[1]);

    Var elementVar = JavascriptOperators::OP_GetProperty(tableDescriptor, PropertyIds::element, scriptContext);
    if (!JavascriptOperators::StrictEqualString(elementVar, scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("anyfunc"))))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_ExpectedAnyFunc, _u("tableDescriptor.element"));
    }

    Var initVar = JavascriptOperators::OP_GetProperty(tableDescriptor, PropertyIds::initial, scriptContext);
    uint32 initial = WebAssembly::ToNonWrappingUint32(initVar, scriptContext);

    uint32 maximum = Wasm::Limits::GetMaxTableSize();
    if (JavascriptOperators::OP_HasProperty(tableDescriptor, PropertyIds::maximum, scriptContext))
    {
        Var maxVar = JavascriptOperators::OP_GetProperty(tableDescriptor, PropertyIds::maximum, scriptContext);
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
    return JavascriptNumber::ToVar(table->m_currentLength, scriptContext);
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
    uint32 oldLength = table->m_currentLength;

    Var deltaVar = args.Info.Count >= 2 ? args[1] : scriptContext->GetLibrary()->GetUndefined();
    uint32 delta = WebAssembly::ToNonWrappingUint32(deltaVar, scriptContext);
    if (delta > 0)
    {
        uint32 newLength = 0;
        if (UInt32Math::Add(table->m_currentLength, delta, &newLength) || newLength > table->m_maxLength)
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
        }

        Field(Var) * newValues = RecyclerNewArrayZ(scriptContext->GetRecycler(), Field(Var), newLength);
        CopyArray(newValues, newLength, table->m_values, table->m_currentLength);

        table->m_values = newValues;
        table->m_currentLength = newLength;
    }

    return JavascriptNumber::ToVar(oldLength, scriptContext);
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
    if (index >= table->m_currentLength)
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
    else if (!AsmJsScriptFunction::IsWasmScriptFunction(args[2]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedWebAssemblyFunc);
    }

    uint32 index = WebAssembly::ToNonWrappingUint32(indexVar, scriptContext);
    if (index >= table->m_currentLength)
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }

    table->m_values[index] = value;

    return scriptContext->GetLibrary()->GetUndefined();
}

WebAssemblyTable *
WebAssemblyTable::Create(uint32 initial, uint32 maximum, ScriptContext * scriptContext)
{
    if (initial > maximum || initial > Wasm::Limits::GetMaxTableSize() || maximum > Wasm::Limits::GetMaxTableSize())
    {
        JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange);
    }
    Field(Var) * values = nullptr;
    if (initial > 0)
    {
        values = RecyclerNewArrayZ(scriptContext->GetRecycler(), Field(Var), initial);
    }
    return RecyclerNew(scriptContext->GetRecycler(), WebAssemblyTable, (Var*)values, initial, initial, maximum, scriptContext->GetLibrary()->GetWebAssemblyTableType());
}

void
WebAssemblyTable::DirectSetValue(uint index, Var val)
{
    Assert(index < m_currentLength);
    Assert(!val || AsmJsScriptFunction::Is(val));
    m_values[index] = val;
}

Var
WebAssemblyTable::DirectGetValue(uint index) const
{
    Assert(index < m_currentLength);
    Var val = m_values[index];
    Assert(!val || AsmJsScriptFunction::Is(val));
    return val;
}

uint32
WebAssemblyTable::GetCurrentLength() const
{
    return m_currentLength;
}

uint32
WebAssemblyTable::GetInitialLength() const
{
    return m_initialLength;
}

uint32
WebAssemblyTable::GetMaximumLength() const
{
    return m_maxLength;
}

} // namespace Js

#endif // ENABLE_WASM
