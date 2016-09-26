//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

namespace Js
{
    template <bool isGuestArena>
    TempArenaAllocatorWrapper<isGuestArena>* TempArenaAllocatorWrapper<isGuestArena>::Create(ThreadContext * threadContext)
    {
        Recycler * recycler = threadContext->GetRecycler();
        TempArenaAllocatorWrapper<isGuestArena> * wrapper = RecyclerNewFinalizedLeaf(recycler, Js::TempArenaAllocatorWrapper<isGuestArena>,
            _u("temp"), threadContext->GetPageAllocator(), Js::Throw::OutOfMemory);
        if (isGuestArena)
        {
            wrapper->recycler = recycler;
            wrapper->AdviseInUse();
        }
        return wrapper;
    }

    template <bool isGuestArena>
    TempArenaAllocatorWrapper<isGuestArena>::TempArenaAllocatorWrapper(__in LPCWSTR name, PageAllocator * pageAllocator, void (*outOfMemoryFunc)()) :
        allocator(name, pageAllocator, outOfMemoryFunc), recycler(nullptr), externalGuestArenaRef(nullptr)
    {
    }

    template <bool isGuestArena>
    void TempArenaAllocatorWrapper<isGuestArena>::Dispose(bool isShutdown)
    {
        allocator.Clear();
        if (isGuestArena && externalGuestArenaRef != nullptr)
        {
            this->recycler->UnregisterExternalGuestArena(externalGuestArenaRef);
            externalGuestArenaRef = nullptr;
        }

        Assert(allocator.AllocatedSize() == 0);
    }

    template <bool isGuestArena>
    void TempArenaAllocatorWrapper<isGuestArena>::AdviseInUse()
    {
        if (isGuestArena)
        {
            if (externalGuestArenaRef == nullptr)
            {
                externalGuestArenaRef = this->recycler->RegisterExternalGuestArena(this->GetAllocator());
                if (externalGuestArenaRef == nullptr)
                {
                    Js::Throw::OutOfMemory();
                }
            }
        }
    }

    template <bool isGuestArena>
    void TempArenaAllocatorWrapper<isGuestArena>::AdviseNotInUse()
    {
        this->allocator.Reset();

        if (isGuestArena)
        {
            Assert(externalGuestArenaRef != nullptr);
            this->recycler->UnregisterExternalGuestArena(externalGuestArenaRef);
            externalGuestArenaRef = nullptr;
        }
    }

    // Explicit instantiation
    template class TempArenaAllocatorWrapper<true>;
    template class TempArenaAllocatorWrapper<false>;

} // namespace Js
