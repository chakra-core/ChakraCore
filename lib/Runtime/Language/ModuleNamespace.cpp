//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"
#include "ModuleNamespace.h"
#include "Types\PropertyIndexRanges.h"
#include "Types\SimpleDictionaryPropertyDescriptor.h"
#include "Types\SimpleDictionaryTypeHandler.h"

namespace Js
{
    ModuleNamespace::ModuleNamespace(ModuleRecordBase* moduleRecord, DynamicType* type) :
        moduleRecord(moduleRecord), DynamicObject(type), unambiguousNonLocalExports(nullptr)
    {

    }

    ModuleNamespace* ModuleNamespace::GetModuleNamespace(ModuleRecordBase* requestModule)
    {
        Assert(requestModule->IsSourceTextModuleRecord());
        SourceTextModuleRecord* moduleRecord = static_cast<SourceTextModuleRecord*>(requestModule);
        ModuleNamespace* nsObject = moduleRecord->GetNamespace();
        if (nsObject != nullptr)
        {
            return nsObject;
        }
        ScriptContext* scriptContext = moduleRecord->GetRealm()->GetScriptContext();
        RecyclableObject* nullValue = scriptContext->GetLibrary()->GetNull();
        Recycler* recycler = scriptContext->GetRecycler();

        ResolvedExportMap* resolvedExportMap = moduleRecord->GetExportedNamesMap();
        if (resolvedExportMap == nullptr)
        {
            Assert(moduleRecord->GetLocalExportCount() == 0);
            Assert(moduleRecord->GetIndirectExportEntryList() == nullptr);
            Assert(moduleRecord->GetStarExportRecordList() == nullptr);
        }

        // First, the storage for local exports are stored in the ModuleRecord object itself, and we can build up a simpleDictionaryTypeHanlder to
        // look them up locally.
        SimpleDictionaryTypeHandlerNotExtensible* typeHandler = SimpleDictionaryTypeHandlerNotExtensible::New(recycler, moduleRecord->GetLocalExportCount(), 0, 0);
        DynamicType* dynamicType = DynamicType::New(scriptContext, TypeIds_ModuleNamespace, nullValue, nullptr, typeHandler);
        nsObject = RecyclerNew(recycler, ModuleNamespace, moduleRecord, dynamicType);

        LocalExportIndexList* localExportIndexList = moduleRecord->GetLocalExportIndexList();
        if (localExportIndexList != nullptr)
        {
            for (uint i = 0; i < (uint)localExportIndexList->Count(); i++)
            {
#if DBG
                ModuleNameRecord tempNameRecord;
                Assert(moduleRecord->GetLocalExportSlotIndexByLocalName(localExportIndexList->Item(i)) == i);
                Assert(resolvedExportMap->TryGetValue(localExportIndexList->Item(i), &tempNameRecord) && tempNameRecord.module == moduleRecord);
#endif
                nsObject->SetPropertyWithAttributes(localExportIndexList->Item(i), nullValue, PropertyModuleNamespaceDefault, nullptr);
            }
        }
        // update the local slot to use the storage for local exports.
        nsObject->SetAuxSlotsForModuleNS(moduleRecord->GetLocalExportSlots());

        // For items that are not in the local export list, we need to resolve them to get it 
        ExportedNames* exportedName = moduleRecord->GetExportedNames(nullptr);
        ModuleNameRecord* moduleNameRecord = nullptr;
#if DBG
        uint unresolvableExportsCount = 0;
#endif
        exportedName->Map([&](PropertyId propertyId) {
            if (!moduleRecord->ResolveExport(propertyId, nullptr, nullptr, &moduleNameRecord))
            {
                // ignore ambigious resolution.
#if DBG
                unresolvableExportsCount++;
#endif
                return;
            }
            // non-ambiguous resolution.
            if (moduleNameRecord == nullptr)
            {
                JavascriptError::ThrowSyntaxError(scriptContext, JSERR_ResolveExportFailed, scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            if (moduleNameRecord->module == requestModule)
            {
                // skip local exports as they are covered in the localExportSlots.
                return;
            }
            Assert(moduleNameRecord->module != moduleRecord);
            nsObject->AddUnambiguousNonLocalExport(propertyId, moduleNameRecord);
        });
#if DBG
        uint totalExportCount = (moduleRecord->GetExportedNames(nullptr) != nullptr) ? moduleRecord->GetExportedNames(nullptr)->Count() : 0;
        uint localExportCount = moduleRecord->GetLocalExportCount();
        uint unambiguousNonLocalCount = (nsObject->GetUnambiguousNonLocalExports() != nullptr) ? nsObject->GetUnambiguousNonLocalExports()->Count() : 0;
        Assert(totalExportCount == localExportCount + unambiguousNonLocalCount + unresolvableExportsCount);
#endif
        BOOL result = nsObject->PreventExtensions();
        Assert(result);
        return nullptr;
        // TODO: enable after actual namespace object implementation.
        //return nsObject;
    }

    void ModuleNamespace::AddUnambiguousNonLocalExport(PropertyId propertyId, ModuleNameRecord* nonLocalExportNameRecord)
    {
        Recycler* recycler = GetScriptContext()->GetRecycler();
        if (unambiguousNonLocalExports == nullptr)
        {
            unambiguousNonLocalExports = RecyclerNew(recycler, UnambiguousExportMap, recycler, 4);
        }
        // keep a local copy of the module/
        unambiguousNonLocalExports->AddNew(propertyId, *nonLocalExportNameRecord);
    }
}
