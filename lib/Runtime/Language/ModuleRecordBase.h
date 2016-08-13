//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ModuleRecordBase;
    class ModuleNamespace;
    typedef SList<PropertyId> ExportedNames;
    typedef SList<ModuleRecordBase*> ExportModuleRecordList;
    struct ModuleNameRecord
    {
        ModuleRecordBase* module;
        PropertyId bindingName;
    };
    typedef SList<ModuleNameRecord> ResolveSet;

    // ModuleRecord need to keep rootFunction etc. alive.
    class ModuleRecordBase : public FinalizableObject
    {
    public:
        static const uint32 ModuleMagicNumber;
        ModuleRecordBase(JavascriptLibrary* library) :
            namespaceObject(nullptr), wasEvaluated(false),
            javascriptLibrary(library),  magicNumber(ModuleMagicNumber){};
        bool WasEvaluated() { return wasEvaluated; }
        void SetWasEvaluated() { Assert(!wasEvaluated); wasEvaluated = true; }
        JavascriptLibrary* GetRealm() { return javascriptLibrary; }  // TODO: do we need to provide this method ?
        virtual ModuleNamespace* GetNamespace() { return namespaceObject; }
        virtual void SetNamespace(ModuleNamespace* moduleNamespace) { namespaceObject = moduleNamespace; }

        virtual ExportedNames* GetExportedNames(ExportModuleRecordList* exportStarSet) = 0;
        // return false when "ambiguous".
        // otherwise nullptr means "null" where we have circular reference/cannot resolve.
        virtual bool ResolveExport(PropertyId exportName, ResolveSet* resolveSet, ExportModuleRecordList* exportStarSet, ModuleNameRecord** exportRecord) = 0;
        virtual void ModuleDeclarationInstantiation() = 0;
        virtual Var ModuleEvaluation() = 0;
        virtual bool IsSourceTextModuleRecord() { return false; }

    protected:
        uint32 magicNumber;
        ModuleNamespace* namespaceObject;
        bool wasEvaluated;
        JavascriptLibrary* javascriptLibrary;
    };
}