//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    ScriptFunctionType::ScriptFunctionType(ScriptFunctionType * type)
        : DynamicType(type), entryPointInfo(type->GetEntryPointInfo())
    {}

    ScriptFunctionType::ScriptFunctionType(ScriptContext* scriptContext, RecyclableObject* prototype,
        JavascriptMethod entryPoint, ProxyEntryPointInfo * entryPointInfo, DynamicTypeHandler * typeHandler,
        bool isLocked, bool isShared)
        : DynamicType(scriptContext, TypeIds_Function, prototype, entryPoint, typeHandler, isLocked, isShared),
        entryPointInfo(entryPointInfo)
    {

    }

    ScriptFunctionType * ScriptFunctionType::New(FunctionProxy * proxy, bool isShared)
    {
        Assert(proxy->GetFunctionInfo()->GetFunctionProxy() == proxy);
        ScriptContext * scriptContext = proxy->GetScriptContext();
        JavascriptLibrary * library = scriptContext->GetLibrary();
        DynamicObject * functionPrototype = proxy->IsAsync() ? library->GetAsyncFunctionPrototype() : library->GetFunctionPrototype();
        JavascriptMethod address = proxy->GetDefaultEntryPointInfo()->jsMethod;

        return RecyclerNew(scriptContext->GetRecycler(), ScriptFunctionType,
            scriptContext, functionPrototype,
            address,
            proxy->GetDefaultEntryPointInfo(),
            library->ScriptFunctionTypeHandler(!proxy->IsConstructor(), proxy->GetIsAnonymousFunction()),
            isShared, isShared);
    }

    void ScriptFunctionType::ChangeEntryPoint(ProxyEntryPointInfo * entryPointInfo, JavascriptMethod entryPoint, bool isAsmJS)
    {
        Assert(entryPoint != nullptr);
        Assert(entryPointInfo != nullptr);
        if (this->GetEntryPoint() == entryPoint && this->GetEntryPointInfo() == entryPointInfo)
        {
            return;
        }

        // ASMJS:- for asmjs we don't need to update the entry point here as it updates the types entry point
        if (!isAsmJS)
        {
            // We can't go from cross-site to non-cross-site. Update only in the non-cross site case
            if (!CrossSite::IsThunk(this->GetEntryPoint()))
            {
                this->SetEntryPoint(entryPoint);
            }
        }
        // instead update the address in the function entrypoint info
        else
        {
            entryPointInfo->jsMethod = entryPoint;
        }

        ProxyEntryPointInfo* oldEntryPointInfo = this->GetEntryPointInfo();
        ScriptFunction::CopyEntryPointInfoToThreadContextIfNecessary(oldEntryPointInfo, entryPointInfo);

        this->SetEntryPointInfo(entryPointInfo);
    }
};
