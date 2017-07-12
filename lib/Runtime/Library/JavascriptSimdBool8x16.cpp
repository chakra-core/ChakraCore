//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "RuntimeLibraryPch.h"

#ifdef ENABLE_SIMDJS
namespace Js
{
    const char16 JavascriptSIMDBool8x16::TypeName[] = _u("SIMD.Bool8x16");

    JavascriptSIMDBool8x16::JavascriptSIMDBool8x16(StaticType *type) : JavascriptSIMDType(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool8x16);
    }

    JavascriptSIMDBool8x16::JavascriptSIMDBool8x16(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool8x16);
    }

    JavascriptSIMDBool8x16* JavascriptSIMDBool8x16::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool8x16 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool8x16, requestContext->GetLibrary()->GetSIMDBool8x16TypeStatic());
    }

    JavascriptSIMDBool8x16* JavascriptSIMDBool8x16::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool8x16 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool8x16, val, requestContext->GetLibrary()->GetSIMDBool8x16TypeStatic());
    }

    bool  JavascriptSIMDBool8x16::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDBool8x16;
    }

    JavascriptSIMDBool8x16* JavascriptSIMDBool8x16::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDBool8x16'");

        return reinterpret_cast<JavascriptSIMDBool8x16 *>(aValue);
    }

    RecyclableObject * JavascriptSIMDBool8x16::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool8x16::New(&value, requestContext);
    }

    const char16* JavascriptSIMDBool8x16::GetTypeName()
    {
        return JavascriptSIMDBool8x16::TypeName;
    }

    Var JavascriptSIMDBool8x16::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool8x16::New(&this->value, requestContext);
    }
}
#endif
