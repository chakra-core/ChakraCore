//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ArgumentsObjectPrefixEnumerator : public JavascriptEnumerator
    {
    protected:
        ArgumentsObject* argumentsObject;
        uint32 formalArgIndex;
        bool doneFormalArgs;
        EnumeratorFlags flags;

    protected:
        DEFINE_VTABLE_CTOR(ArgumentsObjectPrefixEnumerator, JavascriptEnumerator);

    public:
        ArgumentsObjectPrefixEnumerator(ArgumentsObject* argumentsObject, EnumeratorFlags flags, ScriptContext* requestContext);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };

    class ES5ArgumentsObjectEnumerator : public ArgumentsObjectPrefixEnumerator
    {
    protected:
        DEFINE_VTABLE_CTOR(ES5ArgumentsObjectEnumerator, ArgumentsObjectPrefixEnumerator);
        ES5ArgumentsObjectEnumerator(ArgumentsObject* argumentsObject, EnumeratorFlags flags, ScriptContext* requestContext);
        BOOL Init(ForInCache * forInCache);
    public:
        static ES5ArgumentsObjectEnumerator * New(ArgumentsObject* argumentsObject, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache);
        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    private:
        JavascriptStaticEnumerator objectEnumerator;
        uint enumeratedFormalsInObjectArrayCount;  // The number of enumerated formals for far.
    };
}
