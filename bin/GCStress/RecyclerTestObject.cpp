//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

size_t RecyclerTestObject::currentGeneration = 1;

size_t RecyclerTestObject::walkObjectCount = 0;
size_t RecyclerTestObject::walkScannedByteCount = 0;
size_t RecyclerTestObject::walkBarrierByteCount = 0;
size_t RecyclerTestObject::walkTrackedByteCount = 0;
size_t RecyclerTestObject::walkLeafByteCount = 0;
size_t RecyclerTestObject::currentWalkDepth = 0;
size_t RecyclerTestObject::maxWalkDepth = 0;

    
