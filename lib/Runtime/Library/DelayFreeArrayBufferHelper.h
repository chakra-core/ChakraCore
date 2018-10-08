//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  ArrayBuffer is ref-counted. This helper file encapsulate classes to assist free-ing the buffer
//----------------------------------------------------------------------------

#pragma once
namespace Js
{
    class RefCountedBuffer;
    class ArrayBufferContentForDelayedFreeBase;

    class DelayedFreeArrayBuffer
    {
    public:
        DelayedFreeArrayBuffer()
            : listOfBuffers(&HeapAllocator::Instance)
        { }

        ~DelayedFreeArrayBuffer()
        {
            Assert(listOfBuffers.Count() == 0);
        }

        typedef SList<ArrayBufferContentForDelayedFreeBase*, HeapAllocator> DelayedFreeBufferType;

        bool HasAnyItem();
        // Scan the stack to match with current list of delayed free buffer. For those which are not found on the stack 
        // will be released (ref-count decremented)
        void ScanStack(void ** stackTop, size_t byteCount, void ** registers, size_t registersByteCount);
        void ClearAll();

        void Push(ArrayBufferContentForDelayedFreeBase* item);

    private:

        // This array buffer will be added during Detach. They will be released (decrement ref-count) 
        // a. when not found during stack scan 
        // b. Or during callrootlevel == 0
        DelayedFreeBufferType listOfBuffers;

        void ResetToNoMarkObject();
        void ReleaseOnlyNonMarkedObject();
        void CheckAndMarkObject(void * candidate);
    };

    typedef void __cdecl FreeFunction(void* ptr);

    class ArrayBufferContentForDelayedFreeBase
    {
        RefCountedBuffer *buffer;
        uint32 bufferLength;
        Recycler * recycler;
        bool markBit; // This bit will be used when Recycler tries to determine to keep the object based on the object on the stack

    public:
        ArrayBufferContentForDelayedFreeBase(RefCountedBuffer *content, uint32 len, Recycler *r)
            : buffer(content), bufferLength(len), recycler(r), markBit(false)
        {}

        virtual ~ArrayBufferContentForDelayedFreeBase() { }

        virtual void Release();
        virtual bool IsAddressPartOfBuffer(void *obj);
        virtual bool IsEqual(RefCountedBuffer *buffer)
        {
            return this->buffer == buffer;
        }
        virtual void SetMarkBit(bool set)
        {
            markBit = set;
        }
        virtual bool IsMarked()
        {
            return markBit;
        }

        virtual void ClearSelfOnly()
        { 
            Assert(false);
        }

        virtual void FreeTheBuffer(void *buffer)
        {
            Assert(false);
        }

        virtual RefCountedBuffer * GetRefCountedBuffer() { return buffer; }

    };

    template <typename FreeFN>
    class ArrayBufferContentForDelayedFree : public ArrayBufferContentForDelayedFreeBase
    {
        FreeFN* freeFunction;

    public:
        ArrayBufferContentForDelayedFree(RefCountedBuffer *content, uint32 len, Recycler *r, FreeFN * freeFunction)
            : ArrayBufferContentForDelayedFreeBase( content, len, r), freeFunction(freeFunction)
        {}

        virtual void FreeTheBuffer(void* buffer) override
        {
            freeFunction(buffer);
        }
        virtual void ClearSelfOnly() override
        {
            HeapDelete(this);
        }
    };
}
