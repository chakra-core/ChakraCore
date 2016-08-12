//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    // Enumerator for undefined/number/double values.
    class NullEnumerator : public JavascriptEnumerator
    {
    private:
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;

    protected:
        DEFINE_VTABLE_CTOR(NullEnumerator, JavascriptEnumerator);

        // nothing to marshal
        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) override {}
    public:
        NullEnumerator(ScriptContext* scriptContext) : JavascriptEnumerator(scriptContext) {}
    };
};
