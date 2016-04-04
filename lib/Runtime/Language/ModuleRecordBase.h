//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // ModuleRecord need to keep rootFunction etc. alive.
    class ModuleRecordBase : FinalizableObject
    {
    public: 
        const unsigned long ModuleMagicNumber = *(const unsigned long*)"Mode";
        typedef SList<PropertyId> ExportedNames;
        typedef JsUtil::BaseDictionary<ModuleRecordBase*, PropertyId, ArenaAllocator, PrimeSizePolicy> ResolutionDictionary;
        typedef SList<ModuleRecordBase*> ResolveSet;
        typedef struct ModuleNameRecord 
        {
            ModuleRecordBase* module;
            PropertyId bindingName;
        };

        ModuleRecordBase(JavascriptLibrary* library) : 
            namespaceObject(nullptr), wasEvaluated(false), 
            javascriptLibrary(library),  magicNumber(ModuleMagicNumber){};
        bool WasEvaluated() { return wasEvaluated; }
        void SetWasEvaluated() { Assert(!wasEvaluated); wasEvaluated = true; }
        JavascriptLibrary* GetRealm() { return javascriptLibrary; }  // TODO: do we need to provide this method ?
        ModuleNamespace* GetNamespace() { return namespaceObject; }
        void SetNamespace(ModuleNamespace* moduleNamespace) { namespaceObject = moduleNamespace; }

        virtual ExportedNames* GetExportedNames(ResolveSet* exportStarSet) = 0;
        // return false when "ambiguous". otherwise exportRecord.
        virtual bool ResolveExport(PropertyId exportName, ResolutionDictionary* resolveSet, ResolveSet* exportStarSet, ModuleNameRecord** exportRecord) = 0;
        virtual void ModuleDeclarationInstantiation() = 0;
        virtual Var ModuleEvaluation() = 0;

    protected:
        unsigned long magicNumber;
        ModuleNamespace* namespaceObject;
        bool wasEvaluated;
        JavascriptLibrary* javascriptLibrary;
    };
}