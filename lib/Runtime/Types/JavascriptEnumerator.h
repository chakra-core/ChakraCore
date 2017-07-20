//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

namespace Js {
    class JavascriptEnumerator : public RecyclableObject
    {
        friend class CrossSite;
        friend class ExternalObject;
    protected:
        DEFINE_VTABLE_CTOR_ABSTRACT(JavascriptEnumerator, RecyclableObject);

    public:
        JavascriptEnumerator(ScriptContext* scriptContext);

        //
        // Returns item index for all nonnamed Enumerators
        //optional override
        //
        virtual uint32 GetCurrentItemIndex() { return Constants::InvalidSourceIndex; }

        //
        // Sets the enumerator to its initial position
        //
        virtual void Reset() = 0;

        //
        // Moves to the next element and gets the current value.
        // PropertyId: Sets the propertyId of the current value.
        // In some cases, i.e. arrays, propertyId is not returned successfully.
        // Returns: NULL if there are no more elements.
        //
        // Note: in the future we  might want to enumerate specialPropertyIds
        // If that code is added in this base class use JavaScriptRegExpEnumerator.h/cpp
        // as a reference and then remove it. If you have already made the edits before
        // seeing this comment please just consolidate the changes.
        virtual JavascriptString * MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr) = 0;


        static bool Is(Var aValue);
        static JavascriptEnumerator* FromVar(Var varValue);
    };
}
