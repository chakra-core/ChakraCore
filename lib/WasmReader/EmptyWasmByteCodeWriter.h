//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
namespace Js
{
    struct EmptyWasmByteCodeWriter : public IWasmByteCodeWriter
    {
        virtual void Create() override {}
        virtual void Begin(FunctionBody* functionWrite, ArenaAllocator* alloc) override {}
        virtual void End() override {}
        virtual void Reset() override {}
        virtual ByteCodeLabel DefineLabel() override {return 0;}
        virtual void InitData(ArenaAllocator* alloc, int32 initCodeBufferSize) override {}
        virtual void MarkAsmJsLabel(ByteCodeLabel labelID) override {}
        virtual void EmptyAsm(OpCodeAsmJs op) override {}
        virtual void Conv(OpCodeAsmJs op, RegSlot R0, RegSlot R1) override {}
        virtual void AsmReg1IntConst1(OpCodeAsmJs op, RegSlot R0, int C1) override {}
        virtual void AsmInt1Const1(OpCodeAsmJs op, RegSlot R0, int C1) override {}
        virtual void AsmLong1Const1(OpCodeAsmJs op, RegSlot R0, int64 C1) override {}
        virtual void AsmFloat1Const1(OpCodeAsmJs op, RegSlot R0, float C1) override {}
        virtual void AsmDouble1Const1(OpCodeAsmJs op, RegSlot R0, double C1) override {}
        virtual void AsmReg1(OpCodeAsmJs op, RegSlot R0) override {}
        virtual void AsmReg2(OpCodeAsmJs op, RegSlot R0, RegSlot R1) override {}
        virtual void AsmReg3(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2) override {}
        virtual void AsmReg4(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3) override {}
        virtual void AsmReg5(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4) override {}
        virtual void AsmReg9(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8) override {}
        virtual void AsmReg17(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
            RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16) override {}
        virtual void AsmShuffle(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, uint8 indices[]) override {}
        virtual void AsmSimdTypedArr(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint8 dataWidth, ArrayBufferView::ViewType viewType, uint32 offset = 0) override {};
        virtual void WasmSimdConst(OpCodeAsmJs op, RegSlot R0, int C0, int C1, int C2, int C3) override {};

        virtual void AsmSlot(OpCodeAsmJs op, RegSlot value, RegSlot instance, uint32 slotId) override {}

        virtual void AsmBr(ByteCodeLabel labelID, OpCodeAsmJs op = OpCodeAsmJs::AsmBr) override {}
        virtual void AsmBrReg1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1) override {}
        virtual void AsmBrReg1Const1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, int C1) override {}
        virtual void WasmMemAccess(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint32 offset, ArrayBufferView::ViewType viewType) override {}
        virtual uint32 WasmLoopStart(ByteCodeLabel loopEntrance, __in_ecount(WAsmJs::LIMIT) RegSlot* curRegs) { return 0; }
        virtual void ExitLoop(uint32 loopId) override {}
        virtual void AsmStartCall(OpCodeAsmJs op, ArgSlot ArgCount, bool isPatching = false) override {}
        virtual void AsmCall(OpCodeAsmJs op, RegSlot returnValueRegister, RegSlot functionRegister, ArgSlot givenArgCount, AsmJsRetType retType, Js::ProfileId profileId) override {}

        virtual void SetCallSiteCount(Js::ProfileId callSiteCount) override {}
    };
}
#endif
