//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Note: this file is to work around linker unresolved externals WRT templatized types.
//       It's better than putting function bodies in .h file because bodies depend on other classes, such as ScriptContext,
//       for which normally only forward declarations would be available in .h file.
//       The reason why these are available in .inl is because .inl files are included in the very end in Runtime.h.

namespace Js
{
    template <typename StringType>
    inline LiteralStringWithPropertyStringPtr * LiteralStringWithPropertyStringPtr::ConvertString(StringType * originalString)
    {
        CompileAssert(sizeof(StringType) >= sizeof(LiteralStringWithPropertyStringPtr));
        VirtualTableInfo<LiteralStringWithPropertyStringPtr>::SetVirtualTable(originalString);
        LiteralStringWithPropertyStringPtr * convertedString = (LiteralStringWithPropertyStringPtr *)originalString;
        convertedString->SetPropertyString(nullptr);
        return convertedString;
    }

    /////////////////////// ConcatStringBase //////////////////////////
    template <typename ConcatStringType>
    inline const char16* ConcatStringBase::GetSzImpl()
    {
        AssertCanHandleOutOfMemory();
        Assert(!this->IsFinalized());
        Assert(!this->IsFinalized());
        ScriptContext* scriptContext = this->GetScriptContext();

        charcount_t allocSize = SafeSzSize();

        Recycler* recycler = scriptContext->GetRecycler();
        char16* target = RecyclerNewArrayLeaf(recycler, char16, allocSize);

        Copy<ConcatStringType>(target, GetLength());
        target[GetLength()] = _u('\0');

        SetBuffer(target);
        return JavascriptString::GetSz();
    }

    /////////////////////// ConcatStringN //////////////////////////

    template<int N>
    ConcatStringN<N>::ConcatStringN(StaticType* stringTypeStatic, bool doZeroSlotsAndLength) :
        ConcatStringBase(stringTypeStatic)
    {
        Assert(stringTypeStatic);

        if (doZeroSlotsAndLength)
        {
            ClearArray(m_slots, N);
            this->SetLength(0); // Note: the length does not include null character.
        }
    }

    // static
    template<int N>
    ConcatStringN<N>* ConcatStringN<N>::New(ScriptContext* scriptContext)
    {
        Assert(scriptContext);
        return RecyclerNew(scriptContext->GetRecycler(), ConcatStringN<N>, scriptContext->GetLibrary()->GetStringTypeStatic());
    }

    // Set the slot at specified index to specified value.
    template<int N>
    void ConcatStringN<N>::SetItem(_In_range_(0, N - 1) int index, JavascriptString* value)
    {
        Assert(index >= 0 && index < N);
        // Note: value can be NULL. // TODO: should we just assert for non-zero?

        charcount_t oldItemLen = m_slots[index] ? m_slots[index]->GetLength() : 0;
        charcount_t newItemLen = value ? value->GetLength() : 0;
        if (value)
        {
            value = CompoundString::GetImmutableOrScriptUnreferencedString(value);
        }
        m_slots[index] = value;
        charcount_t newTotalLen = this->GetLength() - oldItemLen + newItemLen;
        this->SetLength(newTotalLen);
    }

    template<int N>
    inline const char16 * ConcatStringN<N>::GetSz()
    {
        const char16 * sz = GetSzImpl<ConcatStringN>();

        // Allow slots to be garbage collected if no more refs.
        ClearArray(m_slots, N);

        LiteralStringWithPropertyStringPtr::ConvertString(this);

        return sz;
    }

    /////////////////////// ConcatStringWrapping //////////////////////////

    template<char16 L, char16 R>
    ConcatStringWrapping<L, R>::ConcatStringWrapping(JavascriptString* inner) :
        ConcatStringBase(inner->GetLibrary()->GetStringTypeStatic()),
        m_inner(CompoundString::GetImmutableOrScriptUnreferencedString(inner))
    {
        this->SetLength(inner->GetLength() + 2); // does not include null character
    }

    template<char16 L, char16 R>
    ConcatStringWrapping<L, R>* ConcatStringWrapping<L, R>::New(JavascriptString* inner)
    {
        Recycler* recycler = inner->GetRecycler();
        //return RecyclerNew(recycler, ConcatStringWrapping<L, R>, inner);
        // Expand the RecyclerNew macro as it breaks for more than one template arguments.
#ifdef TRACK_ALLOC
        return new (recycler->TrackAllocInfo(TrackAllocData::CreateTrackAllocData(typeid(ConcatStringWrapping<L, R>), 0, (size_t)-1, __FILE__, __LINE__)),
            &Recycler::Alloc) ConcatStringWrapping<L, R>(inner);
#else
        return new (recycler, &Recycler::Alloc) ConcatStringWrapping<L, R>(inner);
#endif
    }

    template<char16 L, char16 R>
    inline const char16 * ConcatStringWrapping<L, R>::GetSz()
    {
        const char16 * sz = GetSzImpl<ConcatStringWrapping>();
        m_inner = nullptr;
        ClearArray(m_slots);
        LiteralStringWithPropertyStringPtr::ConvertString(this);
        return sz;
    }

    template<char16 L, char16 R>
    inline JavascriptString* ConcatStringWrapping<L, R>::GetFirstItem() const
    {
        Assert(m_inner);
        char16 lBuf[2] = { L, '\0' };
        return this->GetLibrary()->CreateStringFromCppLiteral(lBuf);
    }

    template<char16 L, char16 R>
    inline JavascriptString* ConcatStringWrapping<L, R>::GetLastItem() const
    {
        Assert(m_inner);
        char16 rBuf[2] = { R, '\0' };
        return this->GetLibrary()->CreateStringFromCppLiteral(rBuf);
    }

} // namespace Js.
