//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
namespace Js
{
    template <bool enumNonEnumerable, bool enumSymbols>
    class DynamicObjectSnapshotEnumerator : public DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, /*snapShotSemantics*/true>
    {
        typedef DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, /*snapShotSemantics*/true> Base;

    protected:
        int initialPropertyCount;

        DynamicObjectSnapshotEnumerator(ScriptContext* scriptContext)
            : DynamicObjectEnumerator<enumNonEnumerable, enumSymbols, /*snapShotSemantics*/true>(scriptContext)
        {
        }

        DEFINE_VTABLE_CTOR(DynamicObjectSnapshotEnumerator, Base);
        DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(DynamicObjectSnapshotEnumerator);

        Var MoveAndGetNextFromArray(PropertyId& propertyId, PropertyAttributes* attributes);
        JavascriptString * MoveAndGetNextFromObject(BigPropertyIndex& index, PropertyId& propertyId, PropertyAttributes* attributes);

        DynamicObjectSnapshotEnumerator() { /* Do nothing, needed by the vtable ctor for ForInObjectEnumeratorWrapper */ }
        void Initialize(DynamicObject* object);

    public:
        static JavascriptEnumerator* New(ScriptContext* scriptContext, DynamicObject* object);

        virtual void Reset() override;
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
    };

} // namespace Js
