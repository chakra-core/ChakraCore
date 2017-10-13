//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Core/RecyclerHeapMarkingContext.h"

// Class used to wrap a MarkContext so that calls to MarkObjects during IRecyclerVisitedObject::Trace
// can mark with the correct contextual template parameters
template<bool parallel>
class MarkContextWrapper : public IRecyclerHeapMarkingContext
{
public:
    MarkContextWrapper(MarkContext* context) : markContext(context) {}

    void MarkObjects(void** objects, size_t count, void* parent) override
    {
        for (size_t i = 0; i < count; i++)
        {
            // We pass true for interior, since we expect a majority of the pointers being marked by
            // external objects to themselves be external (and thus candidates for interior pointers).
            markContext->Mark<parallel, /*interior*/true, /*doSpecialMark*/ false>(objects[i], parent);
        }
    }

private:
    // Should only be created on the stack
    void* operator new(size_t);
    MarkContext* markContext;
};
