//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    RuntimeFunction::RuntimeFunction(DynamicType * type)
        : JavascriptFunction(type), functionNameId(nullptr)
    {}

    RuntimeFunction::RuntimeFunction(DynamicType * type, FunctionInfo * functionInfo)
        : JavascriptFunction(type, functionInfo), functionNameId(nullptr)
    {}

    RuntimeFunction::RuntimeFunction(DynamicType * type, FunctionInfo * functionInfo, ConstructorCache* cache)
        : JavascriptFunction(type, functionInfo, cache), functionNameId(nullptr)
    {}

    Var
    RuntimeFunction::EnsureSourceString()
    {
        JavascriptLibrary* library = this->GetLibrary();
        ScriptContext * scriptContext = library->GetScriptContext();
        if (this->functionNameId == nullptr)
        {
            this->functionNameId = library->GetFunctionDisplayString();
        }
        else
        {
            if (TaggedInt::Is(this->functionNameId))
            {
                if (scriptContext->GetConfig()->IsES6FunctionNameEnabled() && this->GetTypeHandler()->IsDeferredTypeHandler())
                {
                    JavascriptString* functionName = nullptr;
                    DebugOnly(bool status = ) this->GetFunctionName(&functionName);
                    Assert(status);
                    this->SetPropertyWithAttributes(PropertyIds::name, functionName, PropertyConfigurable, nullptr);
                }

                // This has a side-effect where any other code (such as debugger) that uses functionNameId value will now get the value like "function foo() { native code }"
                // instead of just "foo". Alternative ways will need to be devised; if it's not desirable to use this full display name value in those cases.
                this->functionNameId = GetNativeFunctionDisplayString(scriptContext, scriptContext->GetPropertyString(TaggedInt::ToInt32(this->functionNameId)));
            }
        }
        Assert(JavascriptString::Is(this->functionNameId));
        return this->functionNameId;
    }

    void
    RuntimeFunction::SetFunctionNameId(Var nameId)
    {
        Assert(functionNameId == NULL);
        Assert(TaggedInt::Is(nameId) || Js::JavascriptString::Is(nameId));

        // We are only reference the propertyId, it needs to be tracked to stay alive
        Assert(!TaggedInt::Is(nameId) || this->GetScriptContext()->IsTrackedPropertyId(TaggedInt::ToInt32(nameId)));
        this->functionNameId = nameId;
    }

#if ENABLE_TTD
    void RuntimeFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        if(this->functionNameId != nullptr)
        {
            extractor->MarkVisitVar(this->functionNameId);
        }

        Var revokableProxy = nullptr;
        RuntimeFunction* function = const_cast<RuntimeFunction*>(this);
        if(function->GetInternalProperty(function, Js::InternalPropertyIds::RevocableProxy, &revokableProxy, nullptr, this->GetScriptContext()))
        {
            extractor->MarkVisitVar(revokableProxy);
        }
    }

    TTD::NSSnapObjects::SnapObjectType RuntimeFunction::GetSnapTag_TTD() const
    {
        Var revokableProxy = nullptr;
        RuntimeFunction* function = const_cast<RuntimeFunction*>(this);
        if(function->GetInternalProperty(function, Js::InternalPropertyIds::RevocableProxy, &revokableProxy, nullptr, this->GetScriptContext()))
        {
            return TTD::NSSnapObjects::SnapObjectType::SnapRuntimeRevokerFunctionObject;
        }
        else
        {
            return TTD::NSSnapObjects::SnapObjectType::SnapRuntimeFunctionObject;
        }
    }

    void RuntimeFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        //
        //TODO: need to add more promise support
        //

        Var revokableProxy = nullptr;
        RuntimeFunction* function = const_cast<RuntimeFunction*>(this);
        if(function->GetInternalProperty(function, Js::InternalPropertyIds::RevocableProxy, &revokableProxy, nullptr, this->GetScriptContext()))
        {
            TTD_PTR_ID* proxyId = alloc.SlabAllocateStruct<TTD_PTR_ID>();
            *proxyId = (JavascriptOperators::GetTypeId(revokableProxy) != TypeIds_Null) ? TTD_CONVERT_VAR_TO_PTR_ID(revokableProxy) : TTD_INVALID_PTR_ID;

            if(*proxyId == TTD_INVALID_PTR_ID)
            {
                TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD_PTR_ID*, TTD::NSSnapObjects::SnapObjectType::SnapRuntimeRevokerFunctionObject>(objData, proxyId);
            }
            else
            {
                TTDAssert(TTD::JsSupport::IsVarComplexKind(revokableProxy), "Huh, it looks like we need to check before adding this as a dep on.");

                uint32 depOnCount = 1;
                TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(1);
                depOnArray[0] = TTD_CONVERT_VAR_TO_PTR_ID(revokableProxy);

                TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD_PTR_ID*, TTD::NSSnapObjects::SnapObjectType::SnapRuntimeRevokerFunctionObject>(objData, proxyId, alloc, depOnCount, depOnArray);
            }
        }
        else
        {
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<void*, TTD::NSSnapObjects::SnapObjectType::SnapRuntimeFunctionObject>(objData, nullptr);
        }
    }
#endif
};
