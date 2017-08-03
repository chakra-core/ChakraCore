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
        doneWithExports = false;
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

    // enumeration order: 9.4.6.10 (sorted) exports first, followed by symbols
    JavascriptString * ModuleNamespaceEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        if (attributes != nullptr)
        {
            // all the attribute should have the same setting here in namespace object.
            *attributes = PropertyModuleNamespaceDefault;
        }

        if (!(this->doneWithExports))
        {
            ListForListIterator *sortedExportedNames = nsObject->GetSortedExportedNames();
            currentLocalMapIndex++;
            if (currentLocalMapIndex < sortedExportedNames->Count())
            {
                Assert(JavascriptString::Is(sortedExportedNames->Item(currentLocalMapIndex)));
                return JavascriptString::FromVar(sortedExportedNames->Item(currentLocalMapIndex));
            }
            else
            {
                this->doneWithExports = true;
            }
        }

        if (!doneWithSymbol)
        {
            JavascriptString * symbolResult = symbolEnumerator.MoveAndGetNext(propertyId, attributes);
            if (symbolResult == nullptr)
            {
                doneWithSymbol = true;
            }
            else
            {
                return symbolResult;
            }
        }

        return nullptr;
    }
}
