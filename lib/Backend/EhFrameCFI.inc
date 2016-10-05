//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// A subset of DWARF cfi
//

//          instruction     _2_6_ bits      arg1        arg2
//
ENTRY_SM1(  advance_loc,    0x1 << 6,       ubyte               )
ENTRY_SM2(  offset,         0x2 << 6,       ubyte,      ULEB128 )
ENTRY_SM1(  restore,        0x3 << 6,       ubyte               )

ENTRY1   (  advance_loc1,   0x02,           ubyte               )
ENTRY1   (  advance_loc2,   0x03,           uhalf               )
ENTRY1   (  advance_loc4,   0x04,           uword               )

ENTRY2   (  def_cfa,        0x0c,           ULEB128,    ULEB128 )
ENTRY1   (  def_cfa_offset, 0x0e,           ULEB128             )

ENTRY    (  nop,            0                                   )

#undef ENTRY_SM1
#undef ENTRY_SM2
#undef ENTRY
#undef ENTRY1
#undef ENTRY2
