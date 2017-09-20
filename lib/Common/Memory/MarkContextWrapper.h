//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "core/RecyclerHeapMarkingContext.h"

// Class used to wrap a MarkContext so that calls to MarkObjects during IRecyclerVisitedObject::Trace
// can mark with the correct contextual template parameters
template<bool parallel, bool interior>
class MarkContextWrapper : public IRecyclerHeapMarkingContext
{
public:
    MarkContextWrapper(MarkContext* context) : markContext(context) {}

    void __stdcall MarkObjects(void** objects, size_t count, void* parent) override
    {
        for (size_t i = 0; i < count; i++)
        {
            // We pass true for interior, since we expect a majority of the pointers being marked by
            // external objects to themselves be external (and thus candidates for interior pointers).
            // EdgeGC-TODO: Review this logic and make sure everyone is on board. The implications of this
            // not being the case are that we'd have to remove the alignment check from MarkContext::Mark
            // and change HeapBlockMap.Mark to mod out the candidate pointer to force it to 16-byte alignment.
            markContext->Mark<parallel, /*interior*/true, /*doSpecialMark*/ false>(objects[i], parent);
        }
    }

private:
    // Should only be created on the stack
    void* operator new(size_t);
    MarkContext* markContext;
};
