//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/SimpleDictionaryPropertyDescriptor.h"
#include "Types/SimpleDictionaryTypeHandler.h"
#include "ModuleNamespace.h"
#include "ModuleNamespaceEnumerator.h"

namespace Js
{
    ModuleNamespaceEnumerator::ModuleNamespaceEnumerator(ModuleNamespace* _nsObject, EnumeratorFlags flags, ScriptContext* scriptContext) :
        JavascriptEnumerator(scriptContext), nsObject(_nsObject), currentLocalMapIndex(Constants::NoBigSlot), currentNonLocalMapIndex(Constants::NoBigSlot), nonLocalMap(nullptr),
        flags(flags)
    {
    }

    ModuleNamespaceEnumerator* ModuleNamespaceEnumerator::New(ModuleNamespace* nsObject, EnumeratorFlags flags, ScriptContext* scriptContext, ForInCache * forInCache)
    {
        ModuleNamespaceEnumerator* enumerator = RecyclerNew(scriptContext->GetRecycler(), ModuleNamespaceEnumerator, nsObject, flags, scriptContext);
        if (enumerator->Init(forInCache))
        {
            return enumerator;
        }
        return nullptr;
    }

    BOOL ModuleNamespaceEnumerator::Init(ForInCache * forInCache)
    {
        if (!nsObject->DynamicObject::GetEnumerator(&symbolEnumerator, flags, GetScriptContext(), forInCache))
        {
            return FALSE;
        }
        nonLocalMap = nsObject->GetUnambiguousNonLocalExports();
        Reset();
        return TRUE;
    }

    void ModuleNamespaceEnumerator::Reset()
    {
        currentLocalMapIndex = Constants::NoBigSlot;
        currentNonLocalMapIndex = Constants::NoBigSlot;
        doneWithLocalExports = false;
        if (!!(flags & EnumeratorFlags::EnumSymbols))
        {
            doneWithSymbol = false;
        }
        else
        {
            doneWithSymbol = true;
        }
        symbolEnumerator.Reset();
    }

    // enumeration order: symbol first; local exports next; nonlocal exports last.
    Var ModuleNamespaceEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var undefined = GetLibrary()->GetUndefined();
        Var result = undefined;
        if (attributes != nullptr)
        {
            // all the attribute should have the same setting here in namespace object.
            *attributes = PropertyModuleNamespaceDefault;
        }
        if (!doneWithSymbol)
        {
            result = symbolEnumerator.MoveAndGetNext(propertyId, attributes);
            if (result == nullptr)
            {
                doneWithSymbol = true;
            }
            else
            {
                return result;
            }
        }
        if (!this->doneWithLocalExports)
        {
            currentLocalMapIndex++;
            JavascriptString* propertyString = nullptr;
            if (!nsObject->FindNextProperty(currentLocalMapIndex, &propertyString, &propertyId, attributes, this->GetScriptContext()))
            {
                // we are done with the object part; 
                this->doneWithLocalExports = true;
            }
            else
            {
                return propertyString;
            }
        }
        if (this->nonLocalMap != nullptr && (currentNonLocalMapIndex + 1 < nonLocalMap->Count()))
        {
            currentNonLocalMapIndex++;
            result = this->GetScriptContext()->GetPropertyString(this->nonLocalMap->GetKeyAt(currentNonLocalMapIndex));
            return result;
        }
        return nullptr;
    }
}
