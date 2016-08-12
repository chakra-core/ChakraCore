//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ModuleNamespaceEnumerator : public JavascriptEnumerator
    {
    protected:
        DEFINE_VTABLE_CTOR(ModuleNamespaceEnumerator, JavascriptEnumerator);
        DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(ModuleNamespaceEnumerator);
        ModuleNamespaceEnumerator(ModuleNamespace* nsObject, ScriptContext* scriptContext, bool enumNonEnumerable, bool enumSymbols = false);

    public:
        static ModuleNamespaceEnumerator* New(ModuleNamespace* nsObject, ScriptContext* scriptContext, bool enumNonEnumerable, bool enumSymbols = false);
        BOOL Init();
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
        virtual Var GetCurrentValue() { Assert(false); return nullptr; }

    private:
        ModuleNamespace* nsObject;
        JavascriptEnumerator* symbolEnumerator;
        ModuleNamespace::UnambiguousExportMap* nonLocalMap;
        BigPropertyIndex currentLocalMapIndex;
        BigPropertyIndex currentNonLocalMapIndex;
        bool doneWithLocalExports;
        bool enumNonEnumerable;
        bool enumSymbols;
        bool doneWithSymbol;
    };
}
