//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    const char16 JavascriptSIMDBool32x4::TypeName[] = _u("SIMD.Bool32x4");

    JavascriptSIMDBool32x4::JavascriptSIMDBool32x4(StaticType *type) : JavascriptSIMDType(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool32x4);
    }

    JavascriptSIMDBool32x4::JavascriptSIMDBool32x4(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool32x4);
    }

    JavascriptSIMDBool32x4* JavascriptSIMDBool32x4::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool32x4, requestContext->GetLibrary()->GetSIMDBool32x4TypeStatic());
    }

    JavascriptSIMDBool32x4* JavascriptSIMDBool32x4::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool32x4, val, requestContext->GetLibrary()->GetSIMDBool32x4TypeStatic());
    }

    bool  JavascriptSIMDBool32x4::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDBool32x4;
    }

    JavascriptSIMDBool32x4* JavascriptSIMDBool32x4::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDBool32x4'");

        return reinterpret_cast<JavascriptSIMDBool32x4 *>(aValue);
    }

    const char16* JavascriptSIMDBool32x4::GetTypeName()
    {
        return JavascriptSIMDBool32x4::TypeName;
    }

    Var JavascriptSIMDBool32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool32x4::New(&this->value, requestContext);
    }

    RecyclableObject * JavascriptSIMDBool32x4::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool32x4::New(&value, requestContext);
    }
}
#endif
