//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

//
// Machine dependent constants.
//

static const int MachChar = 1;
static const int MachShort = 2;
static const int MachInt = 4;
static const int MachRegInt = 8;
static const int MachPtr = 8;
static const int MachDouble = 8;
static const int MachRegDouble = 8;
static const int MachArgsSlotOffset = MachPtr;
static const int MachStackAlignment = 16; // On ARM64 SP needs to be 16 byte aligned for load/store
static const unsigned __int64 MachSignBit = 0x8000000000000000;
