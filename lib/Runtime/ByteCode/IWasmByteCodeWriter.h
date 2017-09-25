//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
namespace Js
{
    struct IWasmByteCodeWriter
    {
    public:
        virtual void Create() = 0;
        virtual void Begin(FunctionBody* functionWrite, ArenaAllocator* alloc) = 0;
        virtual void End() = 0;
        virtual void Reset() = 0;
        virtual ByteCodeLabel DefineLabel() = 0;

        virtual void InitData(ArenaAllocator* alloc, int32 initCodeBufferSize) = 0;
        virtual void MarkAsmJsLabel(ByteCodeLabel labelID) = 0;
        virtual void EmptyAsm(OpCodeAsmJs op) = 0;
        virtual void Conv(OpCodeAsmJs op, RegSlot R0, RegSlot R1) = 0;

        virtual void AsmReg1IntConst1(OpCodeAsmJs op, RegSlot R0, int C1) = 0;
        virtual void AsmInt1Const1(OpCodeAsmJs op, RegSlot R0, int C1) = 0;
        virtual void AsmLong1Const1(OpCodeAsmJs op, RegSlot R0, int64 C1) = 0;
        virtual void AsmFloat1Const1(OpCodeAsmJs op, RegSlot R0, float C1) = 0;
        virtual void AsmDouble1Const1(OpCodeAsmJs op, RegSlot R0, double C1) = 0;
        virtual void AsmReg1(OpCodeAsmJs op, RegSlot R0) = 0;
        virtual void AsmReg2(OpCodeAsmJs op, RegSlot R0, RegSlot R1) = 0;
        virtual void AsmReg3(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2) = 0;
        virtual void AsmReg4(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3) = 0;
        virtual void AsmReg5(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4) = 0;
        virtual void AsmReg9(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8) = 0;
        virtual void AsmReg17(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
            RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16) = 0;
        virtual void AsmShuffle(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, uint8 indices[]) = 0;
        virtual void AsmSimdTypedArr(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint8 dataWidth, ArrayBufferView::ViewType viewType, uint32 offset = 0) = 0;
        virtual void WasmSimdConst(OpCodeAsmJs op, RegSlot R0, int C0, int C1, int C2, int C3) = 0;

        virtual void AsmSlot(OpCodeAsmJs op, RegSlot value, RegSlot instance, uint32 slotId) = 0;
        virtual void AsmBr(ByteCodeLabel labelID, OpCodeAsmJs op = OpCodeAsmJs::AsmBr) = 0;
        virtual void AsmBrReg1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1) = 0;
        virtual void AsmBrReg1Const1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, int C1) = 0;
        virtual void WasmMemAccess(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint32 offset, ArrayBufferView::ViewType viewType) = 0;
        virtual uint EnterLoop(ByteCodeLabel loopEntrance) = 0;
        virtual void ExitLoop(uint loopId) = 0;

        virtual void AsmStartCall(OpCodeAsmJs op, ArgSlot ArgCount, bool isPatching = false) = 0;
        virtual void AsmCall(OpCodeAsmJs op, RegSlot returnValueRegister, RegSlot functionRegister, ArgSlot givenArgCount, AsmJsRetType retType, Js::ProfileId profileId) = 0;

        virtual void SetCallSiteCount(Js::ProfileId callSiteCount) = 0;
    };
}
#endif
