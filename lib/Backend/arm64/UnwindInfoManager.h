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

    // simple setters; none of this is used to generate unwind data, but the prolog/epilog generation relies on it
    void SetHasCalls(bool has) { this->m_hasCalls = has; }
    void SetHomedParamCount(BYTE count) { Assert(this->m_homedParamCount == 0); this->m_homedParamCount = count; }
    void SetSavedReg(BYTE reg) { this->m_savedRegMask |= 1 << reg; }
    void SetDoubleSavedRegList(DWORD doubleRegMask) { this->m_savedDoubleRegMask = doubleRegMask; }

    // simple getters
    bool GetHasCalls() const { return this->m_hasCalls; }
    DWORD GetHomedParamCount() const { return this->m_homedParamCount; }
    bool TestSavedReg(BYTE reg) const { return ((this->m_savedRegMask >> reg) & 1) != 0; }
    DWORD GetDoubleSavedRegList() const { return this->m_savedDoubleRegMask; }

    // label management
    void SetFunctionOffsetLabel(UnwindFunctionOffsets which, IR::LabelInstr *label);
    void SetLabelOffset(DWORD id, DWORD offset);

private:
    DWORD GetFunctionOffset(UnwindFunctionOffsets which);

    bool m_hasCalls;
    BYTE m_homedParamCount;
    DWORD m_savedRegMask;
    DWORD m_savedDoubleRegMask;
    DWORD m_functionOffsetLabelId[UnwindFunctionOffsetCount];
    DWORD m_functionOffset[UnwindFunctionOffsetCount];

    Arm64XdataGenerator m_xdata;
};
