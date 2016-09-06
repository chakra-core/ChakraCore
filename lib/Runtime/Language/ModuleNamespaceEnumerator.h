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
        ModuleNamespaceEnumerator(ModuleNamespace* nsObject, EnumeratorFlags flags, ScriptContext* scriptContext);

    public:
        static ModuleNamespaceEnumerator* New(ModuleNamespace* nsObject, EnumeratorFlags flags, ScriptContext* scriptContext);
        BOOL Init();
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
        virtual Var GetCurrentValue() { Assert(false); return nullptr; }

    private:
        ModuleNamespace* nsObject;
        JavascriptStaticEnumerator symbolEnumerator;
        ModuleNamespace::UnambiguousExportMap* nonLocalMap;
        BigPropertyIndex currentLocalMapIndex;
        BigPropertyIndex currentNonLocalMapIndex;
        bool doneWithLocalExports;
        bool doneWithSymbol;
        EnumeratorFlags flags;
    };
}
