//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ArgumentsObjectEnumerator : public JavascriptEnumerator
    {
    protected:
        ArgumentsObject* argumentsObject;
        uint32 formalArgIndex;
        bool doneFormalArgs;
        JavascriptEnumerator* objectEnumerator;
        BOOL enumNonEnumerable;
        bool enumSymbols;

    protected:
        DEFINE_VTABLE_CTOR(ArgumentsObjectEnumerator, JavascriptEnumerator);
        DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(ArgumentsObjectEnumerator);

    public:
        ArgumentsObjectEnumerator(ArgumentsObject* argumentsObject, ScriptContext* requestcontext, BOOL enumNonEnumerable, bool enumSymbols = false);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };

    class ES5ArgumentsObjectEnumerator : public ArgumentsObjectEnumerator
    {
    protected:
        DEFINE_VTABLE_CTOR(ES5ArgumentsObjectEnumerator, ArgumentsObjectEnumerator);
        DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(ES5ArgumentsObjectEnumerator);

    public:
        ES5ArgumentsObjectEnumerator(ArgumentsObject* argumentsObject, ScriptContext* requestcontext, BOOL enumNonEnumerable, bool enumSymbols = false);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    private:
        uint enumeratedFormalsInObjectArrayCount;  // The number of enumerated formals for far.
    };
}
