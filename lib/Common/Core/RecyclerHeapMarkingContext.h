//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

interface IRecyclerHeapMarkingContext
{
    virtual void MarkObjects(void** objects, size_t count, void* parent) = 0;
};

