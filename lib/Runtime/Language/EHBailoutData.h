//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    enum HandlerType
    {
        HT_None,
        HT_Catch,
        HT_Finally
    };
    class EHBailoutData
    {
    public:
        int32 nestingDepth;
        int32 catchOffset;
        int32 finallyOffset;
        HandlerType ht;
        EHBailoutData * parent;
        EHBailoutData * child;

    public:
        EHBailoutData() : nestingDepth(-1), catchOffset(0), finallyOffset(0), parent(nullptr), child(nullptr),  ht(HT_None) {}
        EHBailoutData(int32 nestingDepth, int32 catchOffset, int32 finallyOffset, HandlerType ht, EHBailoutData * parent)
        {
            this->nestingDepth = nestingDepth;
            this->catchOffset = catchOffset;
            this->finallyOffset = finallyOffset;
            this->ht = ht;
            this->parent = parent;
            this->child = nullptr;
        }

#if ENABLE_NATIVE_CODEGEN
        void Fixup(NativeCodeData::DataChunk* chunkList)
        {
            FixupNativeDataPointer(parent, chunkList);
            FixupNativeDataPointer(child, chunkList);
        }
#endif
    };
}
