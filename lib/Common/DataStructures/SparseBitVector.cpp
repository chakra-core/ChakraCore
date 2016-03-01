//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonDataStructuresPch.h"

BVSparseNode::BVSparseNode(BVIndex beginIndex, BVSparseNode * nextNode) :
    startIndex(beginIndex),
    data(0),
    next(nextNode)
{
}
void BVSparseNode::init(BVIndex beginIndex, BVSparseNode * nextNode)
{
    this->startIndex = beginIndex;
    this->data = 0;
    this->next = nextNode;
}
