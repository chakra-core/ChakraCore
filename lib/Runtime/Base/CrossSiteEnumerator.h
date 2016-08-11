//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
        // for enumerators, the scriptContext of the enumerator is different
        // from the scriptContext of the object.
#if !defined(USED_IN_STATIC_LIB)
#define DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(T) \
    friend class Js::CrossSiteEnumerator<T>; \
    virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) override \
    { \
        Assert(this->GetScriptContext() == scriptContext); \
        AssertMsg(VirtualTableInfo<T>::HasVirtualTable(this) || VirtualTableInfo<Js::CrossSiteEnumerator<T>>::HasVirtualTable(this), "Derived class need to define marshal to script context"); \
        VirtualTableInfo<Js::CrossSiteEnumerator<T>>::SetVirtualTable(this); \
    }
#else
#define DEFINE_MARSHAL_ENUMERATOR_TO_SCRIPT_CONTEXT(T) \
    virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) override {Assert(FALSE);}
#endif
    template <typename T>
    class CrossSiteEnumerator : public T
    {
    private:
        DEFINE_VTABLE_CTOR(CrossSiteEnumerator<T>, T);

    public:
        virtual void Reset() override;
        virtual Var MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes = nullptr) override;
        virtual BOOL IsCrossSiteEnumerator() override
        {
            return true;
        }

    };

    template <typename T>
    void CrossSiteEnumerator<T>::Reset()
    {
        __super::Reset();
    }

    template <typename T>
    Var CrossSiteEnumerator<T>::MoveAndGetNext(_Out_ PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var result = __super::MoveAndGetNext(propertyId, attributes);
        if (result)
        {
            result = CrossSite::MarshalVar(this->GetScriptContext(), result);
        }
        return result;
    }

};
