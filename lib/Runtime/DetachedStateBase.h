//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class DetachedStateBase
    {
    protected:
        TypeId typeId;
        bool hasBeenClaimed;

    public:
        DetachedStateBase(TypeId typeId)
            : typeId(typeId),
            hasBeenClaimed(false)
        {
        }

        virtual ~DetachedStateBase()
        {
        }

        TypeId GetTypeId() { return typeId; }

        bool HasBeenClaimed() { return hasBeenClaimed; }

        void MarkAsClaimed() { hasBeenClaimed = true; }

        void CleanUp()
        {
            if (!hasBeenClaimed)
            {
                DiscardState();
            }
            ClearSelfOnly();
        }

        virtual void ClearSelfOnly() = 0;
        virtual void DiscardState() = 0;
        virtual void Discard() = 0;
        virtual void AddRefBufferContent() = 0;
        virtual long ReleaseRefBufferContent() = 0;
    };

    typedef enum ArrayBufferAllocationType
    {
        Heap = 0x0,
        CoTask = 0x1,
        MemAlloc = 0x02,
        External = 0x03,
    } ArrayBufferAllocationType;

    class RefCountedBuffer;

    class ArrayBufferDetachedStateBase : public DetachedStateBase
    {
    public:
        RefCountedBuffer* buffer;
        uint32 bufferLength;
        ArrayBufferAllocationType allocationType;

        ArrayBufferDetachedStateBase(TypeId typeId, RefCountedBuffer* buffer, uint32 bufferLength, ArrayBufferAllocationType allocationType)
            : DetachedStateBase(typeId),
            buffer(buffer),
            bufferLength(bufferLength),
            allocationType(allocationType)
        {}

        virtual void AddRefBufferContent() override;

        virtual long ReleaseRefBufferContent() override;

    protected:
        // Clean up all local state. Different subclasses use different cleanup mechanisms for the buffer allocation.
        template <typename TFreeFn> void DiscardStateBase(TFreeFn freeFunction)
        {
            // this function will be called in the case where transferable object is going away and current arraybuffer is not claimed.

            if (this->buffer != nullptr)
            {
                RefCountedBuffer *local = this->buffer;
                this->buffer = nullptr;

                Assert(local->GetBuffer());

                if (local->GetBuffer() != nullptr)
                {
                    long ref = local->Release();
                    // The ref may not be 0, as we may have put the object in the DelayFree buffer list.
                    if (ref == 0)
                    {
                        freeFunction(local->GetBuffer());
                        HeapDelete(local);
                    }
                }
            }

            this->bufferLength = 0;
        }
    };
}
