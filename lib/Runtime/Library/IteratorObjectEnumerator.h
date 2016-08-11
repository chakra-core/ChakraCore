//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class IteratorObjectEnumerator sealed : public JavascriptEnumerator
    {
    public:
        static Var Create(ScriptContext* scriptContext, Var iteratorObject);
        virtual Var MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes = nullptr);
        virtual void Reset() override;
    protected:
        IteratorObjectEnumerator(ScriptContext* scriptContext, Var iteratorObject);
        DEFINE_VTABLE_CTOR(IteratorObjectEnumerator, JavascriptEnumerator);
        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) override
        {
            AssertMsg(false, "IteratorObjectEnumerator should never get marshaled");
        }
    private:
        void EnsureIterator();
        RecyclableObject* iteratorObject;
        Var value;
        BOOL done;
    };

}
