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
        BOOL Init(EnumeratorCache * enumeratorCache);

    public:
        static ModuleNamespaceEnumerator* New(ModuleNamespace* nsObject, EnumeratorFlags flags, ScriptContext* scriptContext, EnumeratorCache * enumeratorCache);
        virtual void Reset() override;
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
        virtual Var GetCurrentValue() { Assert(false); return nullptr; }

    private:
        Field(ModuleNamespace*) nsObject;
        Field(JavascriptStaticEnumerator) symbolEnumerator;
        Field(ModuleNamespace::UnambiguousExportMap*) nonLocalMap;
        Field(BigPropertyIndex) currentLocalMapIndex;
        Field(BigPropertyIndex) currentNonLocalMapIndex;
        Field(bool) doneWithExports;
        Field(bool) doneWithSymbol;
        Field(EnumeratorFlags) flags;
    };
}
