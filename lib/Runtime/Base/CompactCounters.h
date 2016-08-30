//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// In general, write-barriers are used only on non-Windows platforms
// However, classes in this file have been allocated in software write-barrier
// memory for historic reasons, because there is some small perf wins here
// Forcing these classes to be in software write-barrier memory
#define FORCE_USE_WRITE_BARRIER 1
#include <Memory/WriteBarrierMacros.h>

#pragma warning(push)
#pragma warning(disable:6200) // C6200: Index is out of valid index range, compiler complains here we use variable length array

namespace Js
{
    template<class T, typename CountT = typename T::CounterFields>
    struct CompactCounters
    {
        struct Fields {
            union {
                uint8 u8Fields[static_cast<size_t>(CountT::Max)];
                int8 i8Fields[static_cast<size_t>(CountT::Max)];
                uint16 u16Fields[static_cast<size_t>(CountT::Max)];
                int16 i16Fields[static_cast<size_t>(CountT::Max)];
                uint32 u32Fields[static_cast<size_t>(CountT::Max)];
                int32 i32Fields[static_cast<size_t>(CountT::Max)];
            };
            Fields() {}
        };

        Field(uint8) fieldSize;
#if DBG

        mutable Field(bool) bgThreadCallStarted;
        Field(bool) isCleaningUp;
#endif
        Pointer(Fields) fields;

        CompactCounters() { }
        CompactCounters(T* host)
            :fieldSize(0)
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
            else if (localFieldSize == 4)
            {
                value = this->fields->u32Fields[type];
            }
            else
            {
                Assert(localFieldSize == 0 && this->isCleaningUp && this->fields == nullptr); // OOM when initial allocation failed
            }

            return value;
        }

        uint32 Set(CountT typeEnum, uint32 val, T* host)
        {
            Assert(bgThreadCallStarted == false || isCleaningUp == true
                || host->GetScriptContext()->GetThreadContext()->GetEtwRundownCriticalSection()->IsLockedByAnyThread());

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

            if (fieldSize == 4)
            {
                return this->fields->u32Fields[type] = val;
            }

            Assert(fieldSize == 0 && this->isCleaningUp && this->fields == nullptr && val == 0); // OOM when allocating the counters structure
            return val;
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

            const uint8 max = static_cast<uint8>(CountT::Max);
            typedef CompactCounters<T, CountT> CounterT;
            CounterT::Fields* fieldsArray = (CounterT::Fields*)RecyclerNewArrayLeafZ(host->GetRecycler(), FieldT, sizeof(FieldT)*max);
            CounterT::Fields* oldFieldsArray = host->counters.fields;
            uint8 i = 0;
            if (this->fieldSize == 1)
            {
                if (sizeof(FieldT) == 2)
                {
                    for (; i < max; i++)
                    {
                        fieldsArray->u16Fields[i] = oldFieldsArray->u8Fields[i];
                    }
                }
                else if (sizeof(FieldT) == 4)
                {
                    for (; i < max; i++)
                    {
                        fieldsArray->u32Fields[i] = oldFieldsArray->u8Fields[i];
                    }
                }
            }
            else if (this->fieldSize == 2)
            {
                for (; i < max; i++)
                {
                    fieldsArray->u32Fields[i] = oldFieldsArray->u16Fields[i];
                }
            }
            else
            {
                Assert(this->fieldSize==0);
            }

            if (this->fieldSize == 0)
            {
                this->fieldSize = sizeof(FieldT);
                this->fields = fieldsArray;
            }
            else
            {
                AutoCriticalSection autoCS(host->GetScriptContext()->GetThreadContext()->GetEtwRundownCriticalSection());
                this->fieldSize = sizeof(FieldT);
                this->fields = fieldsArray;
            }
        }

    };
}

#pragma warning(pop)

RESTORE_WRITE_BARRIER_MACROS();
