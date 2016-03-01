//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#pragma warning(push)
#pragma warning(disable:6200) // C6200: Index is out of valid index range, compiler complains here we use variable length array

namespace Js
{
    template<class T, typename CountT = T::CounterFields>
    struct CompactCounters
    {
        uint8 fieldSize;
#if DBG
        mutable bool bgThreadCallStarted;
        bool isCleaningUp;
#endif
        union {
            uint8 u8Fields[CountT::Max];
            int8 i8Fields[CountT::Max];
            WriteBarrierPtr<uint16> u16Fields;
            WriteBarrierPtr<uint32> u32Fields;
            WriteBarrierPtr<int16> i16Fields;
            WriteBarrierPtr<int32> i32Fields;
        };

        CompactCounters()
            :fieldSize(1)
#if DBG
            , bgThreadCallStarted(false), isCleaningUp(false)
#endif
        {
            memset(u8Fields, 0, (uint8)CountT::Max);
        }

        void AllocCounters(T* host, uint8 newSize);

        uint32 Get(CountT typeEnum) const
        {
#if DBG
            if (ThreadContext::GetContextForCurrentThread() == nullptr)
            {
                bgThreadCallStarted = true;
            }
#endif
            uint8 type = static_cast<uint8>(typeEnum);
            uint8 localFieldSize = fieldSize;
            uint32 value = 0;
            if (localFieldSize == 1)
            {
                value = this->u8Fields[type];
            }
            else if (localFieldSize == 2)
            {
                value = this->u16Fields[type];
            }
            else
            {
                Assert(localFieldSize == 4);
                value = this->u32Fields[type];
            }

            return value;
        }

        int32 GetSigned(CountT typeEnum) const
        {
#if DBG
            if (ThreadContext::GetContextForCurrentThread() == nullptr)
            {
                bgThreadCallStarted = true;
            }
#endif

            uint8 type = static_cast<uint8>(typeEnum);
            uint8 localFieldSize = fieldSize;
            int32 value = 0;
            if (localFieldSize == 1)
            {
                value = this->i8Fields[type];
            }
            else if (localFieldSize == 2)
            {
                value = this->i16Fields[type];
            }
            else
            {
                Assert(localFieldSize == 4);
                value = this->i32Fields[type];
            }
            return value;
        }

        uint32 Set(CountT typeEnum, uint32 val, T* host)
        {            
            Assert(bgThreadCallStarted == false || isCleaningUp == true);

            uint8 type = static_cast<uint8>(typeEnum);
            if (fieldSize == 1)
            {
                if (val <= UINT8_MAX)
                {
                    return this->u8Fields[type] = static_cast<uint8>(val);
                }
                else
                {
                    AllocCounters(host, val <= UINT16_MAX ? 2 : 4);
                }
                return this->Set(typeEnum, val, host);
            }

            if (fieldSize == 2)
            {
                if (val <= UINT16_MAX)
                {
                    return this->u16Fields[type] = static_cast<uint16>(val);
                }
                else
                {
                    AllocCounters(host, 4);
                }
                return this->Set(typeEnum, val, host);
            }

            Assert(fieldSize == 4);
            return this->u32Fields[type] = val;
        }

        int32 SetSigned(CountT typeEnum, int32 val, T* host)
        {
            Assert(bgThreadCallStarted == false || isCleaningUp == true);

            uint8 type = static_cast<uint8>(typeEnum);
            if (fieldSize == 1)
            {
                if (val <= INT8_MAX && val >= INT8_MIN)
                {
                    return this->i8Fields[type] = static_cast<uint8>(val);
                }
                else
                {
                    AllocCounters(host, (val <= INT16_MAX && val >= INT16_MIN) ? 2 : 4);
                }
                return this->SetSigned(typeEnum, val, host);
            }

            if (fieldSize == 2)
            {
                if (val <= INT16_MAX && val >= INT16_MIN)
                {
                    return this->i16Fields[type] = static_cast<uint16>(val);
                }
                else
                {
                    AllocCounters(host, 4);
                }
                return this->SetSigned(typeEnum, val, host);
            }

            Assert(fieldSize == 4);
            return this->i32Fields[type] = val;
        }

        uint32 Increase(CountT typeEnum, T* host)
        {
            Assert(bgThreadCallStarted == false);

            uint8 type = static_cast<uint8>(typeEnum);
            if (fieldSize == 1)
            {
                if (this->u8Fields[type] < UINT8_MAX)
                {
                    return this->u8Fields[type]++;
                }
                else
                {
                    AllocCounters(host, 2);
                }
                return this->Increase(typeEnum, host);
            }

            if (fieldSize == 2)
            {
                if (this->u16Fields[type] < UINT16_MAX)
                {
                    return this->u16Fields[type]++;
                }
                else
                {
                    AllocCounters(host, 4);
                }
                return this->Increase(typeEnum, host);
            }

            Assert(fieldSize == 4);
            return this->u32Fields[type]++;
        }
    };


    template<class T, typename CountT>
    void CompactCounters<T, CountT>::AllocCounters(T* host, uint8 newSize)
    {
        Assert(ThreadContext::GetContextForCurrentThread() || ThreadContext::GetCriticalSection()->IsLocked());

        typedef CompactCounters<T, CountT> CounterT;
        Assert(host->GetRecycler() != nullptr);

        const uint8 signedStart = static_cast<uint8>(CountT::SignedFieldsStart);
        const uint8 max = static_cast<uint8>(CountT::Max);

        void* newFieldsArray = nullptr;
        if (newSize == 2) 
        {
            newFieldsArray = RecyclerNewArrayLeafZ(host->GetRecycler(), uint16, max);
        }
        else 
        {
            Assert(newSize == 4);
            newFieldsArray = RecyclerNewArrayLeafZ(host->GetRecycler(), uint32, max);
        }

        uint8 i = 0;
        if (this->fieldSize == 1)
        {
            if (newSize == 2)
            {
                for (; i < signedStart; i++)
                {
                    ((uint16*)newFieldsArray)[i] = this->u8Fields[i];
                }
                for (; i < max; i++)
                {
                    ((int16*)newFieldsArray)[i] = this->i8Fields[i];
                }
            }
            else
            {
                for (; i < signedStart; i++)
                {
                    ((uint32*)newFieldsArray)[i] = this->u8Fields[i];
                }
                for (; i < max; i++)
                {
                    ((int32*)newFieldsArray)[i] = this->i8Fields[i];
                }
            }
        }
        else if (this->fieldSize == 2)
        {
            for (; i < signedStart; i++)
            {
                ((uint32*)newFieldsArray)[i] = this->u16Fields[i];
            }
            for (; i < max; i++)
            {
                ((int32*)newFieldsArray)[i] = this->i16Fields[i];
            }
        }
        else
        {
            Assert(false);
        }
        
        this->fieldSize = newSize;
        this->u16Fields = (uint16*)newFieldsArray;        

    }
}

#pragma warning(pop)
