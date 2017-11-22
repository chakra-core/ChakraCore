//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include "RecyclerVisitedObject.h"

class FinalizableObject : public IRecyclerVisitedObject
{
public:
    virtual void OnMark() {}

    void Mark(RecyclerHeapHandle recycler) final
    {
        Mark(static_cast<Recycler*>(recycler));
    }

    void Trace(IRecyclerHeapMarkingContext* markingContext)
    {
        AssertMsg(false, "Trace called on object that isn't implemented by the host");
    }

    virtual void Mark(Recycler* recycler) = 0;
};
