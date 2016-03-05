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
        struct Fields {
            union {
                uint8 u8Fields[CountT::Max];
                int8 i8Fields[CountT::Max];
                uint16 u16Fields[CountT::Max];
                int16 i16Fields[CountT::Max];
                uint32 u32Fields[CountT::Max];
                int32 i32Fields[CountT::Max];
            };
            Fields() {}
        };

        uint8 fieldSize;
#if DBG
    
        mutable bool bgThreadCallStarted;
        bool isCleaningUp;
#endif    
        WriteBarrierPtr<Fields> fields;
    
        CompactCounters() { }
        CompactCounters(T* host)
            :fieldSize(sizeof(uint8))
#if DBG
            , bgThreadCallStarted(false), isCleaningUp(false)
#endif
        {
            AllocCounters<uint8>(host);
        }

        uint32 Get(CountT typeEnum) const
        {
#if DBG
            if (!bgThreadCallStarted && ThreadContext::GetContextForCurrentThread() == nullptr)
            {
                bgThreadCallStarted = true;
            }
#endif
            uint8 type = static_cast<uint8>(typeEnum);
            uint8 localFieldSize = fieldSize;
            uint32 value = 0;
            if (localFieldSize == 1)
            {
                value = this->fields->u8Fields[type];
            }
            else if (localFieldSize == 2)
            {
                value = this->fields->u16Fields[type];
            }
            else
            {
                Assert(localFieldSize == 4);
                value = this->fields->u32Fields[type];
            }

            return value;
        }

        int32 GetSigned(CountT typeEnum) const
        {
#if DBG
            if (!bgThreadCallStarted && ThreadContext::GetContextForCurrentThread() == nullptr)
            {
                bgThreadCallStarted = true;
            }
#endif

            uint8 type = static_cast<uint8>(typeEnum);
            uint8 localFieldSize = fieldSize;
            int32 value = 0;
            if (localFieldSize == 1)
            {
                value = this->fields->i8Fields[type];
            }
            else if (localFieldSize == 2)
            {
                value = this->fields->i16Fields[type];
            }
            else
            {
                Assert(localFieldSize == 4);
                value = this->fields->i32Fields[type];
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
                    return this->fields->u8Fields[type] = static_cast<uint8>(val);
                }
                else
                {
                    (val <= UINT16_MAX) ? AllocCounters<uint16>(host) : AllocCounters<uint32>(host);
                    return host->counters.Set(typeEnum, val, host);
                }
            }

            if (fieldSize == 2)
            {
                if (val <= UINT16_MAX)
                {
                    return this->fields->u16Fields[type] = static_cast<uint16>(val);
                }
                else
                {
                    AllocCounters<uint32>(host);
                    return host->counters.Set(typeEnum, val, host);
                }
            }

            Assert(fieldSize == 4);
            return this->fields->u32Fields[type] = val;
        }

        int32 SetSigned(CountT typeEnum, int32 val, T* host)
        {
            Assert(bgThreadCallStarted == false || isCleaningUp == true);

            uint8 type = static_cast<uint8>(typeEnum);
            if (fieldSize == 1)
            {
                if (val <= INT8_MAX && val >= INT8_MIN)
                {
                    return this->fields->i8Fields[type] = static_cast<uint8>(val);
                }
                else
                {
                    (val <= INT16_MAX && val >= INT16_MIN)? AllocCounters<uint16>(host): AllocCounters<uint32>(host);
                    return host->counters.SetSigned(typeEnum, val, host);
                }
            }

            if (fieldSize == 2)
            {
                if (val <= INT16_MAX && val >= INT16_MIN)
                {
                    return this->fields->i16Fields[type] = static_cast<uint16>(val);
                }
                else
                {
                    AllocCounters<uint32>(host);
                    return host->counters.SetSigned(typeEnum, val, host);
                }
            }

            Assert(fieldSize == 4);
            return this->fields->i32Fields[type] = val;
        }

        uint32 Increase(CountT typeEnum, T* host)
        {
            Assert(bgThreadCallStarted == false);

            uint8 type = static_cast<uint8>(typeEnum);
            if (fieldSize == 1)
            {
                if (this->fields->u8Fields[type] < UINT8_MAX)
                {
                    return this->fields->u8Fields[type]++;
                }
                else
                {
                    AllocCounters<uint16>(host);
                    return host->counters.Increase(typeEnum, host);
                }
            }

            if (fieldSize == 2)
            {
                if (this->fields->u16Fields[type] < UINT16_MAX)
                {
                    return this->fields->u16Fields[type]++;
                }
                else
                {
                    AllocCounters<uint32>(host);
                    return host->counters.Increase(typeEnum, host);
                }
            }

            Assert(fieldSize == 4);
            return this->fields->u32Fields[type]++;
        }

        template<typename FieldT>
        void AllocCounters(T* host)
        {
            Assert(ThreadContext::GetContextForCurrentThread() || ThreadContext::GetCriticalSection()->IsLocked());
            Assert(host->GetRecycler() != nullptr);

            const uint8 signedStart = static_cast<uint8>(CountT::SignedFieldsStart);
            const uint8 max = static_cast<uint8>(CountT::Max);
            typedef CompactCounters<T, CountT> CounterT;
            CounterT::Fields* fieldsArray = (CounterT::Fields*)RecyclerNewArrayLeafZ(host->GetRecycler(), FieldT, sizeof(FieldT)*max);
            CounterT::Fields* oldFieldsArray = host->counters.fields;
            uint8 i = 0;
            if (this->fieldSize == 1)
            {
                if (sizeof(FieldT) == 2)
                {
                    for (; i < signedStart; i++)
                    {
                        fieldsArray->u16Fields[i] = oldFieldsArray->u8Fields[i];
                    }
                    for (; i < max; i++)
                    {
                        fieldsArray->i16Fields[i] = oldFieldsArray->i8Fields[i];
                    }
                }
                else if (sizeof(FieldT) == 4)
                {
                    for (; i < signedStart; i++)
                    {
                        fieldsArray->u32Fields[i] = oldFieldsArray->u8Fields[i];
                    }
                    for (; i < max; i++)
                    {
                        fieldsArray->i32Fields[i] = oldFieldsArray->i8Fields[i];
                    }
                }
            }
            else if (this->fieldSize == 2)
            {
                for (; i < signedStart; i++)
                {
                    fieldsArray->u32Fields[i] = oldFieldsArray->u16Fields[i];
                }
                for (; i < max; i++)
                {
                    fieldsArray->i32Fields[i] = oldFieldsArray->i16Fields[i];
                }
            }
            else
            {
                Assert(false);
            }

            this->fieldSize = sizeof(FieldT);
            this->fields = fieldsArray;
        }

    };
}

#pragma warning(pop)
