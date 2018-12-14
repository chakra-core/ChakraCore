//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#if DBG

class DbCheckPostLower
{
private:
    Func *func;

    void        Check(IR::Opnd *opnd);
    void        Check(IR::RegOpnd *regOpnd);

#if defined(_M_IX86) || defined(_M_X64)
    bool        IsCallToHelper(IR::Instr *instr, IR::JnHelperMethod method);
    bool        IsEndBoundary(IR::Instr * instr);
    void        EnsureValidEndBoundary(IR::Instr * instr);
    bool        IsAssign(IR::Instr * instr);
    void        EnsureOnlyMovesToRegisterOpnd(IR::Instr * instr);
#endif

public:
    DbCheckPostLower(Func *func) : func(func) { }
    void        Check();

#if defined(_M_IX86) || defined(_M_X64)
    void        CheckNestedHelperCalls();
#endif
};

#endif  // DBG
