//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    namespace UtilSupport
    {
        TTAutoString::TTAutoString()
            : m_allocSize(-1), m_contents(nullptr), m_optFormatBuff(nullptr)
        {
            ;
        }

        TTAutoString::TTAutoString(LPCWSTR str)
            : m_allocSize(-1), m_contents(nullptr), m_optFormatBuff(nullptr)
        {
            size_t wclen = wcslen(str) + 1;

            this->m_contents = HeapNewArrayZ(wchar, wclen);
            this->m_allocSize = (int32)wclen;

            wcscat_s(this->m_contents, wclen, str);
        }

        TTAutoString::TTAutoString(const TTAutoString& str)
            : m_allocSize(-1), m_contents(nullptr), m_optFormatBuff(nullptr)
        {
            this->Append(str.m_contents);
        }

        TTAutoString::TTAutoString(TTAutoString&& str)
            : m_allocSize(str.m_allocSize), m_contents(str.m_contents), m_optFormatBuff(str.m_optFormatBuff)
        {
            str.m_allocSize = -1;
            str.m_contents = nullptr;
            str.m_optFormatBuff = nullptr;
        }

        TTAutoString::~TTAutoString()
        {
            this->Clear();
        }

        void TTAutoString::Clear()
        {
            if(this->m_contents != nullptr)
            {
                HeapDeleteArray(this->m_allocSize, this->m_contents);
                this->m_allocSize = -1;
                this->m_contents = nullptr;
            }

            if(this->m_optFormatBuff != nullptr)
            {
                HeapDeleteArray(64, this->m_optFormatBuff);
                this->m_optFormatBuff = nullptr;
            }
        }

        TTAutoString& TTAutoString::operator=(const TTAutoString& str)
        {
            if(this != &str)
            {
                this->Clear();

                this->Append(str.GetStrValue());
            }

            return *this;
        }

        TTAutoString& TTAutoString::operator=(TTAutoString&& str)
        {
            if(this != &str)
            {
                this->Clear();

                this->m_allocSize = str.m_allocSize;
                str.m_allocSize = -1;

                this->m_contents = str.m_contents;
                str.m_contents = nullptr;

                this->m_optFormatBuff = str.m_optFormatBuff;
                str.m_optFormatBuff = nullptr;
            }

            return *this;
        }

        bool TTAutoString::IsNullString() const
        {
            return this->m_contents == nullptr;
        }

        void TTAutoString::Append(LPCWSTR str, int32 start, int32 end)
        {
            if(this->m_contents == nullptr && str == nullptr)
            {
                return;
            }

            size_t origsize = (this->m_contents != nullptr ? wcslen(this->m_contents) : 0);
            int32 strsize = -1;
            if(start == 0 && end == INT32_MAX)
            {
                strsize = (str != nullptr ? (int32)wcslen(str) : 0);
            }
            else
            {
                strsize = (end - start) + 1;
            }

            size_t nsize = origsize + strsize + 1;
            wchar* nbuff = HeapNewArrayZ(wchar, nsize);

            if(this->m_contents != nullptr)
            {
                wcscat_s(nbuff, nsize, this->m_contents);

                HeapDeleteArray(origsize + 1, this->m_contents);
                this->m_allocSize = -1;
                this->m_contents = nullptr;
            }

            if(str != nullptr)
            {
                int32 curr = (int32)origsize;
                for(int32 i = start; i <= end && str[i] != '\0'; ++i)
                {
                    nbuff[curr] = str[i];
                    curr++;
                }
                nbuff[curr] = L'\0';
            }

            this->m_contents = nbuff;
            this->m_allocSize = (int32)nsize;
        }

        void TTAutoString::Append(const TTAutoString& str, int32 start, int32 end)
        {
            this->Append(str.GetStrValue(), start, end);
        }

        void TTAutoString::Append(uint64 val)
        {
            if(this->m_optFormatBuff == nullptr)
            {
                this->m_optFormatBuff = HeapNewArrayZ(wchar, 64);
            }

            swprintf_s(this->m_optFormatBuff, 32, L"%I64u", val); //64 wchars is 32 words

            this->Append(this->m_optFormatBuff);
        }

        void TTAutoString::Append(LPCUTF8 strBegin, LPCUTF8 strEnd)
        {
            int32 strCount = (int32)((strEnd - strBegin) + 1);
            wchar* buff = HeapNewArrayZ(wchar, (size_t)strCount);

            LPCUTF8 curr = strBegin;
            int32 i = 0;
            while(curr != strEnd)
            {
                buff[i] = (wchar)*curr;
                i++;
                curr++;
            }
            AssertMsg(i + 1 == strCount, "Our indexing is off.");

            buff[i] = L'\0';
            this->Append(buff);

            HeapDeleteArray((size_t)strCount, buff);
        }

        int32 TTAutoString::GetLength() const
        {
            AssertMsg(!this->IsNullString(), "That doesn't make sense.");

            return (int32)wcslen(this->m_contents);
        }

        wchar TTAutoString::GetCharAt(int32 pos) const
        {
            AssertMsg(!this->IsNullString(), "That doesn't make sense.");
            AssertMsg(0 <= pos && pos < this->GetLength(), "Not in valid range.");

            return this->m_contents[pos];
        }

        LPCWSTR TTAutoString::GetStrValue() const
        {
            return this->m_contents;
        }
    }
    //////////////////

    MarkTable::MarkTable()
    {
        this->m_markPages = HeapNewArrayZ(MarkTableTag*, TTD_PAGES_PER_HEAP);

        this->m_iter.m_currPageIdx = 0;
        this->m_iter.m_currOffset = nullptr;
        this->m_iter.m_currOffsetEnd = nullptr;

        memset(this->m_handlerCounts, 0, ((uint32)MarkTableTag::KindTagCount) * sizeof(uint32));
    }

    MarkTable::~MarkTable()
    {
        this->Clear();

        HeapDeleteArray(TTD_PAGES_PER_HEAP, this->m_markPages);
    }

    void MarkTable::Clear()
    {
        for(uint32 i = 0; i < TTD_PAGES_PER_HEAP; ++i)
        {
            if(this->m_markPages[i] != nullptr)
            {
                HeapDeleteArray(TTD_OFFSETS_PER_PAGE, this->m_markPages[i]);
                this->m_markPages[i] = nullptr;
            }
        }

        this->m_iter.m_currPageIdx = 0;
        this->m_iter.m_currOffset = nullptr;
        this->m_iter.m_currOffsetEnd = nullptr;

        memset(this->m_handlerCounts, 0, ((uint32)MarkTableTag::KindTagCount) * sizeof(uint32));
    }

    bool MarkTable::IsMarked(const void* vaddr) const
    {
        AssertMsg((reinterpret_cast<uint64>(vaddr)& TTD_OBJECT_ALIGNMENT_MASK) == 0, "Not word aligned!!");
        AssertMsg(TTD_REBUILD_ADDR(TTD_MARK_GET_PAGE(reinterpret_cast<uint64>(vaddr)), TTD_MARK_GET_OFFSET(reinterpret_cast<uint64>(vaddr))) == reinterpret_cast<uint64>(vaddr), "Lost some bits in the address.");

        const MarkTableTag* page = this->GetMarkPageForAddrNoCreate(reinterpret_cast<uint64>(vaddr));
        AssertMsg(page != nullptr, "Missing a mark for this address");

        uint32 offset = TTD_MARK_GET_OFFSET(reinterpret_cast<uint64>(vaddr));
        AssertMsg(offset < TTD_OFFSETS_PER_PAGE, "We are writing off the end of the page");

        return page[offset] != MarkTableTag::Clear;
    }

    bool MarkTable::IsTaggedAsWellKnown(const void* vaddr) const
    {
        AssertMsg((reinterpret_cast<uint64>(vaddr)& TTD_OBJECT_ALIGNMENT_MASK) == 0, "Not word aligned!!");
        AssertMsg(TTD_REBUILD_ADDR(TTD_MARK_GET_PAGE(reinterpret_cast<uint64>(vaddr)), TTD_MARK_GET_OFFSET(reinterpret_cast<uint64>(vaddr))) == reinterpret_cast<uint64>(vaddr), "Lost some bits in the address.");

        const MarkTableTag* page = this->GetMarkPageForAddrNoCreate(reinterpret_cast<uint64>(vaddr));
        AssertMsg(page != nullptr, "Missing a mark for this address");

        uint32 offset = TTD_MARK_GET_OFFSET(reinterpret_cast<uint64>(vaddr));
        AssertMsg(offset < TTD_OFFSETS_PER_PAGE, "We are writing off the end of the page");

        return (page[offset] & MarkTableTag::JsWellKnownObj) != MarkTableTag::Clear;
    }

    MarkTableTag MarkTable::GetTagValue() const
    {
        if(this->m_iter.m_currOffset == nullptr)
        {
            return MarkTableTag::Clear;
        }
        else
        {
            return *(this->m_iter.m_currOffset);
        }
    }

    bool MarkTable::GetTagValueIsWellKnown() const
    {
        return (*(this->m_iter.m_currOffset) & MarkTableTag::JsWellKnownObj) != MarkTableTag::Clear;
    }

    bool MarkTable::GetTagValueIsLogged() const
    {
        return (*(this->m_iter.m_currOffset) & MarkTableTag::LogTaggedObj) != MarkTableTag::Clear;
    }

    void MarkTable::ClearMark(const void* vaddr)
    {
        AssertMsg((reinterpret_cast<uint64>(vaddr)& TTD_OBJECT_ALIGNMENT_MASK) == 0, "Not word aligned!!");
        AssertMsg(TTD_REBUILD_ADDR(TTD_MARK_GET_PAGE(reinterpret_cast<uint64>(vaddr)), TTD_MARK_GET_OFFSET(reinterpret_cast<uint64>(vaddr))) == reinterpret_cast<uint64>(vaddr), "Lost some bits in the address.");

        MarkTableTag* page = this->GetMarkPageForAddrNoCreate(reinterpret_cast<uint64>(vaddr));
        AssertMsg(page != nullptr, "Missing a mark for this address");

        uint32 offset = TTD_MARK_GET_OFFSET(reinterpret_cast<uint64>(vaddr));
        AssertMsg(offset < TTD_OFFSETS_PER_PAGE, "We are writing off the end of the page");

        page[offset] = MarkTableTag::Clear;
    }

    void MarkTable::MoveToNextAddress()
    {
        //advance at least one position
        this->m_iter.m_currOffset++;

        while(true)
        {
            while(this->m_iter.m_currOffset < this->m_iter.m_currOffsetEnd)
            {
                MarkTableTag tag = *(this->m_iter.m_currOffset);
                if((tag & MarkTableTag::AllKindMask) != MarkTableTag::Clear)
                {
                    return;
                }
                this->m_iter.m_currOffset++;
            }

            this->m_iter.m_currPageIdx++;
            while(this->m_iter.m_currPageIdx < TTD_PAGES_PER_HEAP)
            {
                if(this->m_markPages[this->m_iter.m_currPageIdx] != nullptr)
                {
                    break;
                }

                this->m_iter.m_currPageIdx++;
            }


            if(this->m_iter.m_currPageIdx == TTD_PAGES_PER_HEAP)
            {
                this->m_iter.m_currPageIdx = 0;
                this->m_iter.m_currOffset = nullptr;
                this->m_iter.m_currOffsetEnd = nullptr;

                return;
            }

            this->m_iter.m_currOffset = this->m_markPages[this->m_iter.m_currPageIdx];
            this->m_iter.m_currOffsetEnd = this->m_markPages[this->m_iter.m_currPageIdx] + TTD_OFFSETS_PER_PAGE;
        }
    }

    void MarkTable::InitializeIter()
    {
        this->m_iter.m_currPageIdx = 0;
        this->m_iter.m_currOffset = nullptr;
        this->m_iter.m_currOffsetEnd = nullptr;

        this->m_iter.m_currPageIdx++;
        while(this->m_iter.m_currPageIdx < TTD_PAGES_PER_HEAP)
        {
            if(this->m_markPages[this->m_iter.m_currPageIdx] != nullptr)
            {
                break;
            }

            this->m_iter.m_currPageIdx++;
        }

        if(this->m_iter.m_currPageIdx == TTD_PAGES_PER_HEAP)
        {
            return;
        }

        this->m_iter.m_currOffset = this->m_markPages[this->m_iter.m_currPageIdx];
        this->m_iter.m_currOffsetEnd = this->m_markPages[this->m_iter.m_currPageIdx] + TTD_OFFSETS_PER_PAGE;

        //the first address in the first page might be marked, if it is we don't want to advance otherwise move to first marked address
        MarkTableTag tag = *(this->m_iter.m_currOffset);
        if((tag & MarkTableTag::AllKindMask) == MarkTableTag::Clear)
        {
            this->MoveToNextAddress();
        }
    }
}

#endif
