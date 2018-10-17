//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

//
// Machine dependent constants.
//

static const int MachInt = 4;
static const int MachRegInt = 8;
static const int MachPtr = 8;
static const int MachDouble = 8;
static const int MachRegDouble = 8;
static const int MachStackAlignment = MachPtr;
static const int MachArgsSlotOffset = MachPtr;
static const int MachMaxInstrSize = 12;
static const unsigned __int64 MachSignBit = 0x8000000000000000;
static const int MachSimd128 = 16;
