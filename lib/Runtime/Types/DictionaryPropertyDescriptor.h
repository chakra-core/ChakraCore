//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

    namespace DictionaryPropertyDescriptorFlags
    {
        enum class Flags : uint8
        {
            None                         = 0,
            PreventFalseReference        = 1 << 0,
            IsShadowed                   = 1 << 1,
            IsAccessor                   = 1 << 2,
#if ENABLE_FIXED_FIELDS
            IsInitialized                = 1 << 3,
            IsOnlyOneAccessorInitialized = 1 << 4,
            IsFixed                      = 1 << 5,
            UsedAsFixed                  = 1 << 6,
#endif
        };

        ENUM_CLASS_HELPERS(Flags, uint8);

        inline Flags FlagIf(bool condition, Flags flag)
        {
            return condition ? flag : Flags::None;
        }
    }

    template <typename TPropertyIndex>
    class DictionaryPropertyDescriptor
    {
        template <typename T>
        friend class DictionaryPropertyDescriptor;
    public:

        using Flags = DictionaryPropertyDescriptorFlags::Flags;

        DictionaryPropertyDescriptor(TPropertyIndex dataSlot, bool isInitialized = false, bool isFixed = false, bool usedAsFixed = false) :
            Data(dataSlot), Getter(NoSlots), Setter(NoSlots), Attributes(PropertyDynamicTypeDefaults), flags(Flags::PreventFalseReference
#if ENABLE_FIXED_FIELDS
            | FlagIf(isInitialized, Flags::IsInitialized) | FlagIf(isFixed, Flags::IsFixed) | FlagIf(usedAsFixed, Flags::UsedAsFixed)
#endif
            ) { }

        DictionaryPropertyDescriptor(TPropertyIndex getterSlot, TPropertyIndex setterSlot, bool isInitialized = false, bool isFixed = false, bool usedAsFixed = false) :
            Data(NoSlots), Getter(getterSlot), Setter(setterSlot), Attributes(PropertyDynamicTypeDefaults), flags(Flags::PreventFalseReference | Flags::IsAccessor
#if ENABLE_FIXED_FIELDS
            | FlagIf(isInitialized, Flags::IsInitialized) | FlagIf(isFixed, Flags::IsFixed) | FlagIf(usedAsFixed, Flags::UsedAsFixed)
#endif
            ) { }

        DictionaryPropertyDescriptor(TPropertyIndex dataSlot, PropertyAttributes attributes, bool isInitialized = false, bool isFixed = false, bool usedAsFixed = false) :
            Data(dataSlot), Getter(NoSlots), Setter(NoSlots), Attributes(attributes), flags(Flags::PreventFalseReference
#if ENABLE_FIXED_FIELDS
            | FlagIf(isInitialized, Flags::IsInitialized) | FlagIf(isFixed, Flags::IsFixed) | FlagIf(usedAsFixed, Flags::UsedAsFixed)
#endif
            ) { }

        DictionaryPropertyDescriptor(TPropertyIndex getterSlot, TPropertyIndex setterSlot, PropertyAttributes attributes, bool isInitialized = false, bool isFixed = false, bool usedAsFixed = false) :
            Data(NoSlots), Getter(getterSlot), Setter(setterSlot), Attributes(attributes), flags(Flags::PreventFalseReference | Flags::IsAccessor
#if ENABLE_FIXED_FIELDS
            | FlagIf(isInitialized, Flags::IsInitialized) | FlagIf(isFixed, Flags::IsFixed) | FlagIf(usedAsFixed, Flags::UsedAsFixed)
#endif
            ) { }

        // this is for initialization.
        DictionaryPropertyDescriptor() : Data(NoSlots), Getter(NoSlots), Setter(NoSlots), Attributes(PropertyDynamicTypeDefaults), flags(Flags::PreventFalseReference) { }

        template <typename TPropertyIndexFrom>
        void CopyFrom(DictionaryPropertyDescriptor<TPropertyIndexFrom>& descriptor);

    private:
        // SimpleDictionaryPropertyDescriptor is allocated by a dictionary along with the PropertyRecord
        // so it cannot be allocated as leaf, tag the lower bit to prevent false reference: flags must
        // always have PreventFalseReference set
        Flags flags;

    public:
        PropertyAttributes Attributes;
    private:
        TPropertyIndex Data;
        // CONSIDER: sharing the Data slot with one of these and use the attributes to tell it apart.
        TPropertyIndex Getter;
        TPropertyIndex Setter;
    public:
        template <bool allowLetConstGlobal>
        TPropertyIndex GetDataPropertyIndex() const;
        TPropertyIndex GetGetterPropertyIndex() const;
        TPropertyIndex GetSetterPropertyIndex() const;
        void ConvertToData();
        bool ConvertToGetterSetter(TPropertyIndex& nextSlotIndex);

        bool HasNonLetConstGlobal() const
        {
            return (this->Attributes & PropertyLetConstGlobal) == 0 || GetIsShadowed();
        }
        void AddShadowedData(TPropertyIndex& nextPropertyIndex, bool addingLetConstGlobal);

#define EXPOSE_FLAG_ACCESSOR(flag) \
        inline bool Get##flag() const\
        {\
            return HasFlag(Flags::flag);\
        }\
        inline void Set##flag(bool set)\
        {\
            SetFlag(Flags::flag, set);\
        }

        EXPOSE_FLAG_ACCESSOR(IsShadowed);
        EXPOSE_FLAG_ACCESSOR(IsAccessor);
#if ENABLE_FIXED_FIELDS
        EXPOSE_FLAG_ACCESSOR(IsInitialized);
        EXPOSE_FLAG_ACCESSOR(IsOnlyOneAccessorInitialized);
        EXPOSE_FLAG_ACCESSOR(IsFixed);
        EXPOSE_FLAG_ACCESSOR(UsedAsFixed);
#endif
#undef EXPOSE_FLAG_ACCESSOR

    private:
        static const TPropertyIndex NoSlots = PropertyIndexRanges<TPropertyIndex>::NoSlots;

        inline bool HasFlag(Flags flag) const
        {
            return (this->flags & flag) == flag;
        }

        inline void SetFlag(Flags flag, bool set)
        {
            if (set)
            {
                this->flags = this->flags | flag;
            }
            else
            {
                this->flags = this->flags & ~flag;
            }
        }

    public:
        bool IsOrMayBecomeFixed()
        {
#if ENABLE_FIXED_FIELDS
            return !GetIsInitialized() || GetIsFixed();
#else
            return false;
#endif
        }
#if ENABLE_FIXED_FIELDS && DBG
        bool SanityCheckFixedBits()
        {
            return
                (!GetIsFixed() && !GetUsedAsFixed()) ||
                (!(this->Attributes & PropertyDeleted) && (this->Data != NoSlots || this->Getter != NoSlots || this->Setter != NoSlots));
        }
#endif

#if DBG_DUMP
    public:
        void Dump(unsigned indent = 0) const;
#endif
    };


    template <typename TPropertyIndex>
    template <bool allowLetConstGlobal>
    TPropertyIndex DictionaryPropertyDescriptor<TPropertyIndex>::GetDataPropertyIndex() const
    {
        // If it is let const global, the data slot is the let const property, and if we allow let const global,
        // we already return that the Getter/Setter slot may be doubled as the Data Slot
        // so only return it if we allow let const
        bool const isLetConstGlobal = (this->Attributes & PropertyLetConstGlobal) != 0;
        if (isLetConstGlobal)
        {
            Assert(this->Data != NoSlots);  // Should always have slot for LetConstGlobal if specified
            if (allowLetConstGlobal)
            {
                return this->Data;
            }
            else if (GetIsShadowed() && !GetIsAccessor())
            {
                // if it is a let const global, if the setter slot is empty, then the Getter slot must be
                // the shadowed data slot, return that.
                return this->Getter;
            }
        }
        else
        {
            Assert(!GetIsAccessor() || this->Data == NoSlots);
            return this->Data;
        }
        return NoSlots;
    }

    template <typename TPropertyIndex>
    TPropertyIndex DictionaryPropertyDescriptor<TPropertyIndex>::GetGetterPropertyIndex() const
    {
        // Need to check data property index first
        Assert(GetDataPropertyIndex<false>() == NoSlots);
        return this->Getter;
    }

    template <typename TPropertyIndex>
    TPropertyIndex DictionaryPropertyDescriptor<TPropertyIndex>::GetSetterPropertyIndex() const
    {
        // Need to check data property index first
        Assert(GetDataPropertyIndex<false>() == NoSlots);
        return this->Setter;
    }

    template <typename TPropertyIndex>
    void DictionaryPropertyDescriptor<TPropertyIndex>::ConvertToData()
    {
        if (GetIsAccessor())
        {
            Assert(this->Getter != NoSlots && this->Setter != NoSlots);
            SetIsAccessor(false);
            if (GetIsShadowed())
            {
                Assert(this->Data != NoSlots);
            }
            else
            {
                Assert(this->Data == NoSlots);
                this->Data = this->Getter;
                this->Getter = NoSlots;
            }
        }
        Assert(GetDataPropertyIndex<false>() != NoSlots);
    }

    template <typename TPropertyIndex>
    void DictionaryPropertyDescriptor<TPropertyIndex>::AddShadowedData(TPropertyIndex& nextPropertyIndex, bool addingLetConstGlobal)
    {
        Assert(!GetIsShadowed());
        SetIsShadowed(true);
        if (GetIsAccessor())
        {
            Assert(this->Data == NoSlots);
            if (addingLetConstGlobal)
            {
                this->Data = ::Math::PostInc(nextPropertyIndex);
            }
        }
        else if (addingLetConstGlobal)
        {
            this->Getter = this->Data;
            this->Data = ::Math::PostInc(nextPropertyIndex);
        }
        else
        {
            this->Getter = ::Math::PostInc(nextPropertyIndex);
        }
        this->Attributes |= PropertyLetConstGlobal;
        Assert((addingLetConstGlobal ? GetDataPropertyIndex<true>() : GetDataPropertyIndex<false>()) != NoSlots);
    }

    template <typename TPropertyIndex>
    bool DictionaryPropertyDescriptor<TPropertyIndex>::ConvertToGetterSetter(TPropertyIndex& nextPropertyIndex)
    {
        // Initial descriptor state and corresponding conversion can be one of the following:
        //
        // | State                              | Data    | Getter   | Setter   | Conversion                                                                                        |
        // |------------------------------------|---------|----------|----------|---------------------------------------------------------------------------------------------------|
        // | Data property                      | valid   | NoSlots? | NoSlots? | Move Data to Getter, set Data to NoSlots, create new slot for Setter if necessary, set IsAccessor |
        // | LetConstGlobal                     | valid   | NoSlots? | NoSlots? | Create new slots for Getter and Setter if necessary, set IsAccessor, set IsShadowed               |
        // | Data property + LetConstGlobal     | valid   | valid    | NoSlots? | Create new slot for Setter if necessary, set IsAccessor                                           |
        // | Accessor property                  | NoSlots | valid    | valid    | Nothing                                                                                           |
        // | Accessor property + LetConstGlobal | valid   | valid    | valid    | Nothing                                                                                           |
        // |------------------------------------|---------|----------|----------|---------------------------------------------------------------------------------------------------|
        //
        // NOTE: Do not create slot for Getter/Setter if they are already valid; possible after previous conversion from Accessor to Data, or deletion of Accessor, etc.

        if (GetIsAccessor())
        {
            // Accessor property
            // Accessor property + LetConstGlobal
            Assert(this->Getter != NoSlots && this->Setter != NoSlots);
            return false;
        }

        SetIsAccessor(true);
        if (this->Attributes & PropertyLetConstGlobal)
        {
            if (GetIsShadowed())
            {
                // Data property + LetConstGlobal
                Assert(this->Getter != NoSlots);
            }
            else
            {
                // LetConstGlobal
                SetIsShadowed(true);
            }
        }
        else
        {
            // Data property
            Assert(this->Data != NoSlots);
            Assert(this->Getter == NoSlots);
            this->Getter = this->Data;
            this->Data = NoSlots;
        }

        bool addedPropertyIndex = false;
        if (this->Getter == NoSlots)
        {
            this->Getter = ::Math::PostInc(nextPropertyIndex);
            addedPropertyIndex = true;
        }
        if (this->Setter == NoSlots)
        {
            this->Setter = ::Math::PostInc(nextPropertyIndex);
            addedPropertyIndex = true;
        }
        Assert(this->GetGetterPropertyIndex() != NoSlots || this->GetSetterPropertyIndex() != NoSlots);
        return addedPropertyIndex;
    }

    template <typename TPropertyIndex>
    template <typename TPropertyIndexFrom>
    void DictionaryPropertyDescriptor<TPropertyIndex>::CopyFrom(DictionaryPropertyDescriptor<TPropertyIndexFrom>& descriptor)
    {
        this->Attributes = descriptor.Attributes;
        this->Data = (descriptor.Data == DictionaryPropertyDescriptor<TPropertyIndexFrom>::NoSlots) ? NoSlots : descriptor.Data;
        this->Getter = (descriptor.Getter == DictionaryPropertyDescriptor<TPropertyIndexFrom>::NoSlots) ? NoSlots : descriptor.Getter;
        this->Setter = (descriptor.Setter == DictionaryPropertyDescriptor<TPropertyIndexFrom>::NoSlots) ? NoSlots : descriptor.Setter;

        this->flags = descriptor.flags;
    }

#if DBG_DUMP
    template<typename TPropertyIndex>
    void DictionaryPropertyDescriptor<TPropertyIndex>::Dump(unsigned indent) const
    {
        const auto padding(_u(""));
        const unsigned fieldIndent(indent + 2);

        Output::Print(_u("%*sDictionaryPropertyDescriptor (0x%p)\n"), indent, padding, this);
        Output::Print(_u("%*sPreventFalseReference: %d\n"), fieldIndent, padding, HasFlag(Flags::PreventFalseReference));
        Output::Print(_u("%*sIsShadowed: %d\n"), fieldIndent, padding, GetIsShadowed());
        Output::Print(_u("%*sIsAccessor: %d\n"), fieldIndent, padding, GetIsAccessor());
#if ENABLE_FIXED_FIELDS
        Output::Print(_u("%*sIsInitialized: %d\n"), fieldIndent, padding, GetIsInitialized());
        Output::Print(_u("%*sIsOnlyOneAccessorInitialized: %d\n"), fieldIndent, padding, GetIsOnlyOneAccessorInitialized());
        Output::Print(_u("%*sIsFixed: %d\n"), fieldIndent, padding, GetIsFixed());
        Output::Print(_u("%*sUsedAsFixed: %d\n"), fieldIndent, padding, GetUsedAsFixed());
#endif
        Output::Print(_u("%*sAttributes: 0x%02x "), fieldIndent, padding, this->Attributes);
        if (this->Attributes != PropertyNone)
        {
            if (this->Attributes & PropertyEnumerable) Output::Print(_u("PropertyEnumerable "));
            if (this->Attributes & PropertyConfigurable) Output::Print(_u("PropertyConfigurable "));
            if (this->Attributes & PropertyWritable) Output::Print(_u("PropertyWritable "));
            if (this->Attributes & PropertyDeleted) Output::Print(_u("PropertyDeleted "));
            if (this->Attributes & PropertyLetConstGlobal) Output::Print(_u("PropertyLetConstGlobal "));
            if (this->Attributes & PropertyDeclaredGlobal) Output::Print(_u("PropertyDeclaredGlobal "));
            if (this->Attributes & PropertyLet) Output::Print(_u("PropertyLet "));
            if (this->Attributes & PropertyConst) Output::Print(_u("PropertyConst "));
        }
        Output::Print(_u("\n"));

        Output::Print(_u("%*sData: %d\n"), fieldIndent, padding, static_cast<int32>(this->Data));
        Output::Print(_u("%*sGetter: %d\n"), fieldIndent, padding, static_cast<int32>(this->Getter));
        Output::Print(_u("%*sSetter: %d\n"), fieldIndent, padding, static_cast<int32>(this->Setter));
    }
#endif
}
