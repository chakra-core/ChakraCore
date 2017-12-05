//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "ARM64UnwindEncoder.h"

enum UnwindFunctionOffsets
{
    UnwindInvalid,
    UnwindPrologStart,
    UnwindPrologEnd,
    UnwindEpilogStart,
    UnwindEpilogEnd,
    UnwindFunctionOffsetCount
};

class UnwindInfoManager
{
public:
    UnwindInfoManager();

    void Init(Func * func);
    DWORD GetPDataCount(DWORD length);
    DWORD SizeOfUnwindInfo() const { return m_xdata.GetXdataBytes(); }
    BYTE *GetUnwindInfo() { return reinterpret_cast<BYTE *>(const_cast<void *>(m_xdata.GetXdata())); }
    void FinalizeUnwindInfo(BYTE *functionStart, DWORD codeSize);

    // label management
    void SetFunctionOffsetLabel(UnwindFunctionOffsets which, IR::LabelInstr *label);
    void SetLabelOffset(DWORD id, DWORD offset);

private:
    DWORD GetFunctionOffset(UnwindFunctionOffsets which);

    DWORD m_functionOffsetLabelId[UnwindFunctionOffsetCount];
    DWORD m_functionOffset[UnwindFunctionOffsetCount];

    Arm64XdataGenerator m_xdata;
};
