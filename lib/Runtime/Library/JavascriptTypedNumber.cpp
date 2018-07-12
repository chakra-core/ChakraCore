//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    template <>
    Var JavascriptTypedNumber<__int64>::ToVar(__int64 value, ScriptContext* scriptContext)
    {
        if (!TaggedInt::IsOverflow(value))
        {
            return TaggedInt::ToVarUnchecked((int)value);
        }
        JavascriptTypedNumber<__int64>* number = RecyclerNewLeaf(scriptContext->GetRecycler(), JavascriptInt64Number, value,
            scriptContext->GetLibrary()->GetInt64TypeStatic());
        return number;
    }

    template <>
    Var JavascriptTypedNumber<unsigned __int64>::ToVar(unsigned __int64 value, ScriptContext* scriptContext)
    {
        if (!TaggedInt::IsOverflow(value))
        {
            return TaggedInt::ToVarUnchecked((uint)value);
        }
        JavascriptTypedNumber<unsigned __int64>* number = RecyclerNewLeaf(scriptContext->GetRecycler(), JavascriptUInt64Number, value,
            scriptContext->GetLibrary()->GetUInt64TypeStatic());
        return number;
    }

    template <>
    JavascriptString* JavascriptTypedNumber<__int64>::ToString(Var value, ScriptContext* scriptContext)
    {
        char16 szBuffer[22];
        __int64 val = VarTo<JavascriptTypedNumber<__int64>>(value)->GetValue();
        int pos = TaggedInt::SignedToString(val, szBuffer, 22);
        return JavascriptString::NewCopyBuffer(szBuffer + pos, (_countof(szBuffer) - 1) - pos, scriptContext);
    }

    template <>
    JavascriptString* JavascriptTypedNumber<unsigned __int64>::ToString(Var value, ScriptContext* scriptContext)
    {
        char16 szBuffer[22];
        unsigned __int64 val = VarTo<JavascriptUInt64Number>(value)->GetValue();
        int pos = TaggedInt::UnsignedToString(val, szBuffer, 22);
        return JavascriptString::NewCopyBuffer(szBuffer + pos, (_countof(szBuffer) - 1) - pos, scriptContext);
    }

    template <typename T>
    RecyclableObject* JavascriptTypedNumber<T>::ToObject(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->CreateNumberObjectWithCheck((double)m_value);
    }
}
