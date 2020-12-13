//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{

    bool DelayedFreeArrayBuffer::HasAnyItem()
    {
        return !this->listOfBuffers.Empty();
    }

    void DelayedFreeArrayBuffer::Push(ArrayBufferContentForDelayedFreeBase* item)
    {
        AssertOrFailFast(item);
        this->listOfBuffers.Push(item);
    }

    void Js::DelayedFreeArrayBuffer::ResetToNoMarkObject()
    {
        this->listOfBuffers.Map([](Js::ArrayBufferContentForDelayedFreeBase* item) {
            Assert(item != nullptr);
            item->SetMarkBit(false);
        });
    }

    void DelayedFreeArrayBuffer::ReleaseOnlyNonMarkedObject()
    {
        FOREACH_SLIST_ENTRY_EDITING(ArrayBufferContentForDelayedFreeBase*, item, &this->listOfBuffers, iter)
        {
            if (!item->IsMarked())
            {
                item->Release();
                item->ClearSelfOnly();
                iter.RemoveCurrent();
            }
            else
            {
                // Reset the mark bit
                item->SetMarkBit(false);
            }
        } NEXT_SLIST_ENTRY_EDITING
    }

    void DelayedFreeArrayBuffer::CheckAndMarkObject(void * candidate)
    {
        this->listOfBuffers.Map([&](Js::ArrayBufferContentForDelayedFreeBase* item) {
            if (!item->IsMarked() && item->IsAddressPartOfBuffer(candidate))
            {
                item->SetMarkBit(true);
            }
        });
    }

    void DelayedFreeArrayBuffer::ClearAll()
    {
        if (HasAnyItem())
        {
            this->listOfBuffers.Map([](Js::ArrayBufferContentForDelayedFreeBase* item) {
                item->Release();
                item->ClearSelfOnly();
            });

            this->listOfBuffers.Clear();
        }
    }

    void DelayedFreeArrayBuffer::ScanStack(void ** stackTop, size_t byteCount, void ** registers, size_t registersByteCount)
    {
        AssertOrFailFast(HasAnyItem());
        ResetToNoMarkObject();

        auto BufferFreeFunction = [&](void ** obj, size_t byteCount)
        {
            Assert(byteCount != 0);
            Assert(byteCount % sizeof(void *) == 0);

            void ** objEnd = obj + (byteCount / sizeof(void *));

            do
            {
                // We need to ensure that the compiler does not reintroduce reads to the object after inlining.
                // This could cause the value to change after the marking checks (e.g., the null/low address check).
                // Intrinsics avoid the expensive memory barrier on ARM (due to /volatile:ms).
#if defined(_M_ARM64)
                void * candidate = reinterpret_cast<void *>(__iso_volatile_load64(reinterpret_cast<volatile __int64 *>(obj)));
#elif defined(_M_ARM)
                void * candidate = reinterpret_cast<void *>(__iso_volatile_load32(reinterpret_cast<volatile __int32 *>(obj)));
#else
                void * candidate = *(static_cast<void * volatile *>(obj));
#endif
                CheckAndMarkObject(candidate);
                obj++;
            } while (obj != objEnd);

        };

        BufferFreeFunction(registers, registersByteCount);
        BufferFreeFunction(stackTop, byteCount);

        ReleaseOnlyNonMarkedObject();
    }

    void ArrayBufferContentForDelayedFreeBase::Release()
    {
        // this function will be called when we are releasing instance from the listOfBuffer which we have delayed.

        RefCountedBuffer *content = this->buffer;
        this->buffer = nullptr;
        long refCount = content->Release();
        if (refCount == 0)
        {
            FreeTheBuffer(content->GetBuffer());
            HeapDelete(content);
        }
    }

    bool ArrayBufferContentForDelayedFreeBase::IsAddressPartOfBuffer(void *obj)
    {
        void *start = this->buffer->GetBuffer();
        void *end = this->buffer->GetBuffer() + this->bufferLength;
        return start <= obj && obj < end;
    }

}
