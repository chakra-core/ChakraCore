//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/SimpleDictionaryPropertyDescriptor.h"
#include "Types/SimpleDictionaryTypeHandler.h"
#include "Types/NullTypeHandler.h"
#include "ModuleNamespace.h"
#include "ModuleNamespaceEnumerator.h"

namespace Js
{
    Js::FunctionInfo ModuleNamespace::EntryInfo::SymbolIterator(ModuleNamespace::EntrySymbolIterator);

    ModuleNamespace::ModuleNamespace(ModuleRecordBase* moduleRecord, DynamicType* type) :
        moduleRecord(moduleRecord), DynamicObject(type), unambiguousNonLocalExports(nullptr), 
        sortedExportedNames(nullptr), nsSlots(nullptr)
    {

    }

    ModuleNamespace* ModuleNamespace::GetModuleNamespace(ModuleRecordBase* requestModule)
    {
        Assert(requestModule->IsSourceTextModuleRecord());
        SourceTextModuleRecord* moduleRecord = SourceTextModuleRecord::FromHost(requestModule);
        ModuleNamespace* nsObject = moduleRecord->GetNamespace();
        if (nsObject != nullptr)
        {
            return nsObject;
        }
        ScriptContext* scriptContext = moduleRecord->GetRealm()->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();

        nsObject = RecyclerNew(recycler, ModuleNamespace, moduleRecord, scriptContext->GetLibrary()->GetModuleNamespaceType());
        nsObject->Initialize();

        moduleRecord->SetNamespace(nsObject);
        return nsObject;
    }

    void ModuleNamespace::Initialize()
    {
        ScriptContext* scriptContext = moduleRecord->GetRealm()->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();
        SourceTextModuleRecord* sourceTextModuleRecord = static_cast<SourceTextModuleRecord*>(moduleRecord);
        JavascriptLibrary* library = GetLibrary();

        if (scriptContext->GetConfig()->IsES6ToStringTagEnabled())
        {
            DynamicObject::SetPropertyWithAttributes(PropertyIds::_symbolToStringTag, library->GetModuleTypeDisplayString(),
                PropertyConfigurable, nullptr);
        }

        DynamicType* type = library->CreateFunctionWithLengthType(&EntryInfo::SymbolIterator);
        RuntimeFunction* iteratorFunction = RecyclerNewEnumClass(scriptContext->GetRecycler(),
            library->EnumFunctionClass, RuntimeFunction,
            type, &EntryInfo::SymbolIterator);
        DynamicObject::SetPropertyWithAttributes(PropertyIds::_symbolIterator, iteratorFunction, PropertyBuiltInMethodDefaults, nullptr);

        ModuleImportOrExportEntryList* localExportList = sourceTextModuleRecord->GetLocalExportEntryList();
        // We don't have a type handler that can handle ModuleNamespace object. We have properties that could be aliased
        // like {export foo as foo1, foo2, foo3}, and external properties as reexport from current module. The problem with aliasing
        // is that multiple propertyId can be associated with one slotIndex. We need to build from PropertyMap directly here.
        // there is one instance of ModuleNamespace per module file; we can always use the BigPropertyIndex for security.
        propertyMap = RecyclerNew(recycler, SimplePropertyDescriptorMap, recycler, sourceTextModuleRecord->GetLocalExportCount());
        if (localExportList != nullptr)
        {
            localExportList->Map([=](ModuleImportOrExportEntry exportEntry) {
                PropertyId exportNameId = exportEntry.exportName->GetPropertyId();
                PropertyId localNameId = exportEntry.localName->GetPropertyId();
                const Js::PropertyRecord* exportNameRecord = scriptContext->GetThreadContext()->GetPropertyName(exportNameId);
                ModuleNameRecord* importRecord = nullptr;
                AssertMsg(exportNameId != Js::Constants::NoProperty, "should have been initialized already");
                // ignore local exports that are actually indirect exports.
                if (sourceTextModuleRecord->GetImportEntryList() == nullptr ||
                    (sourceTextModuleRecord->ResolveImport(localNameId, &importRecord)
                    && importRecord == nullptr))
                {
                    BigPropertyIndex index = sourceTextModuleRecord->GetLocalExportSlotIndexByExportName(exportNameId);
                    Assert((uint)index < sourceTextModuleRecord->GetLocalExportCount());
                    SimpleDictionaryPropertyDescriptor<BigPropertyIndex> propertyDescriptor = { index, PropertyModuleNamespaceDefault };
                    propertyMap->Add(exportNameRecord, propertyDescriptor);
                }
            });
        }
        // update the local slot to use the storage for local exports.
        SetNSSlotsForModuleNS(sourceTextModuleRecord->GetLocalExportSlots());

        // For items that are not in the local export list, we need to resolve them to get it 
        ExportedNames* exportedNames = sourceTextModuleRecord->GetExportedNames(nullptr);
        ModuleNameRecord* moduleNameRecord = nullptr;
#if DBG
        uint unresolvableExportsCount = 0;
        uint localExportCount = 0;
#endif
        if (exportedNames != nullptr)
        {
            exportedNames->Map([&](PropertyId propertyId) {
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
                if (moduleNameRecord->module == moduleRecord)
                {
                    // skip local exports as they are covered in the localExportSlots.
#if DBG
                    localExportCount++;
#endif
                    return;
                }
                Assert(moduleNameRecord->module != moduleRecord);
                this->AddUnambiguousNonLocalExport(propertyId, moduleNameRecord);
            });
        }
#if DBG
        uint totalExportCount = exportedNames != nullptr ? exportedNames->Count() : 0;
        uint unambiguousNonLocalCount = (this->GetUnambiguousNonLocalExports() != nullptr) ? this->GetUnambiguousNonLocalExports()->Count() : 0;
        Assert(totalExportCount == localExportCount + unambiguousNonLocalCount + unresolvableExportsCount);
#endif
        BOOL result = this->PreventExtensions();
        Assert(result);
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

    BOOL ModuleNamespace::HasProperty(PropertyId propertyId)
    {
        SimpleDictionaryPropertyDescriptor<BigPropertyIndex> propertyDescriptor;
        const Js::PropertyRecord* propertyRecord = GetScriptContext()->GetThreadContext()->GetPropertyName(propertyId);
        if (propertyRecord->IsSymbol())
        {
            return this->DynamicObject::HasProperty(propertyId);
        }
        if (propertyMap != nullptr && propertyMap->TryGetValue(propertyRecord, &propertyDescriptor))
        {
            return TRUE;
        }
        if (unambiguousNonLocalExports != nullptr)
        {
            return unambiguousNonLocalExports->ContainsKey(propertyId);
        }
        return FALSE;
    }

    BOOL ModuleNamespace::HasOwnProperty(PropertyId propertyId)
    {
        return HasProperty(propertyId);
    }

    BOOL ModuleNamespace::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        SimpleDictionaryPropertyDescriptor<BigPropertyIndex> propertyDescriptor;
        const Js::PropertyRecord* propertyRecord = requestContext->GetThreadContext()->GetPropertyName(propertyId);
        if (propertyRecord->IsSymbol())
        {
            return this->DynamicObject::GetProperty(originalInstance, propertyId, value, info, requestContext);
        }
        if (propertyMap != nullptr && propertyMap->TryGetValue(propertyRecord, &propertyDescriptor))
        {
            Assert((uint)propertyDescriptor.propertyIndex < ((SourceTextModuleRecord*)moduleRecord)->GetLocalExportCount());
            PropertyValueInfo::SetNoCache(info, this); // Disable inlinecache for localexport slot for now.
            //if ((PropertyIndex)propertyDescriptor.propertyIndex == propertyDescriptor.propertyIndex)
            //{
            //    PropertyValueInfo::Set(info, this, (PropertyIndex)propertyDescriptor.propertyIndex, propertyDescriptor.Attributes);
            //}
            *value = this->GetNSSlot(propertyDescriptor.propertyIndex);
            return TRUE;
        }
        if (unambiguousNonLocalExports != nullptr)
        {
            ModuleNameRecord moduleNameRecord;
            // TODO: maybe we can cache the slot address & offset, instead of looking up everytime? We do need to look up the reference everytime.
            if (unambiguousNonLocalExports->TryGetValue(propertyId, &moduleNameRecord))
            {
                return moduleNameRecord.module->GetNamespace()->GetProperty(originalInstance, moduleNameRecord.bindingName, value, info, requestContext);
            }
        }
        return FALSE;
    }

    BOOL ModuleNamespace::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        const PropertyRecord* propertyRecord = nullptr;
        GetScriptContext()->GetOrAddPropertyRecord(propertyNameString->GetString(), propertyNameString->GetLength(), &propertyRecord);
        return GetProperty(originalInstance, propertyRecord->GetPropertyId(), value, info, requestContext);
    }

    BOOL ModuleNamespace::GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return FALSE;
    }

    BOOL ModuleNamespace::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    BOOL ModuleNamespace::GetEnumerator(BOOL enumNonEnumerable, Var* enumerator, ScriptContext* scriptContext, bool preferSnapshotSemantics, bool enumSymbols)
    {
        ModuleNamespaceEnumerator* moduleEnumerator = ModuleNamespaceEnumerator::New(this, scriptContext, !!enumNonEnumerable, enumSymbols);
        if (moduleEnumerator == nullptr)
        {
            return FALSE;
        }
        *enumerator = moduleEnumerator;
        return TRUE;
    }

    BOOL ModuleNamespace::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        //Assert: IsPropertyKey(P) is true.
        //Let exports be O.[[Exports]].
        //If P is an element of exports, return false.
        //Return true.        
        return !HasProperty(propertyId);
    }

    BOOL ModuleNamespace::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("{ModuleNamespaceObject}"));
        return TRUE;
    }

    BOOL ModuleNamespace::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (ModuleNamespaceObject)"));
        return TRUE;
    }

    Var ModuleNamespace::GetNSSlot(BigPropertyIndex propertyIndex)
    {
        Assert((uint)propertyIndex < (static_cast<SourceTextModuleRecord*>(moduleRecord))->GetLocalExportCount());
        return this->nsSlots[propertyIndex];
    }

    PropertyId ModuleNamespace::GetPropertyId(BigPropertyIndex index)
    {
        SimpleDictionaryPropertyDescriptor<BigPropertyIndex> propertyDescriptor;
        if (propertyMap->TryGetValueAt(index, &propertyDescriptor))
        {
            const PropertyRecord* propertyRecord = propertyMap->GetKeyAt(index);
            return propertyRecord->GetPropertyId();
        }
        return Constants::NoProperty;
    }

    BOOL ModuleNamespace::FindNextProperty(BigPropertyIndex& index, JavascriptString** propertyString, PropertyId* propertyId, PropertyAttributes* attributes) const
    {
        if (index < propertyMap->Count())
        {
            SimpleDictionaryPropertyDescriptor<BigPropertyIndex> propertyDescriptor(propertyMap->GetValueAt(index));
            Assert(propertyDescriptor.Attributes == PropertyModuleNamespaceDefault);
            const PropertyRecord* propertyRecord = propertyMap->GetKeyAt(index);
            *propertyId = propertyRecord->GetPropertyId();
            if (propertyString != nullptr)
            {
                *propertyString = GetScriptContext()->GetPropertyString(*propertyId);
            }
            if (attributes != nullptr)
            {
                *attributes = propertyDescriptor.Attributes;
            }
            return TRUE;
        }
        else
        {
            *propertyId = Constants::NoProperty;
            if (propertyString != nullptr)
            {
                *propertyString = nullptr;
            }
        }
        return FALSE;
    }

    // We will make sure the iterator will iterate through the exported properties in sorted order.
    // There is no such requirement for enumerator (forin).
    ListForListIterator* ModuleNamespace::EnsureSortedExportedNames()
    {
        if (sortedExportedNames == nullptr)
        {
            ExportedNames* exportedNames = moduleRecord->GetExportedNames(nullptr);
            ScriptContext* scriptContext = GetScriptContext();
            sortedExportedNames = ListForListIterator::New(scriptContext->GetRecycler());
            exportedNames->Map([&](PropertyId propertyId) {
                JavascriptString* propertyString = scriptContext->GetPropertyString(propertyId);
                sortedExportedNames->Add(propertyString);
            });
            sortedExportedNames->Sort([](void* context, const void* left, const void* right) ->int {
                JavascriptString** leftString = (JavascriptString**) (left);
                JavascriptString** rightString = (JavascriptString**) (right);
                if (JavascriptString::LessThan(*leftString, *rightString))
                {
                    return -1;
                }
                if (JavascriptString::LessThan(*rightString, *leftString))
                {
                    return 1;
                }
                return 0;
            }, nullptr);
        }
        return sortedExportedNames;
    }

    Var ModuleNamespace::EntrySymbolIterator(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedNamespace, _u("Namespace[Symbol.iterator]"));
        }

        if (JavascriptOperators::GetTypeId(args[0]) != TypeIds_ModuleNamespace)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedNamespace, _u("Namespace[Symbol.iterator]"));
        }

        ModuleNamespace* moduleNamespace = ModuleNamespace::FromVar(args[0]);
        ListForListIterator* sortedExportedNames = moduleNamespace->EnsureSortedExportedNames();
        return scriptContext->GetLibrary()->CreateListIterator(sortedExportedNames);
    }
}
