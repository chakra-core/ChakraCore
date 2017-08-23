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
        virtual void AsmSlot(OpCodeAsmJs op, RegSlot value, RegSlot instance, uint32 slotId) = 0;
        virtual void AsmBr(ByteCodeLabel labelID, OpCodeAsmJs op = OpCodeAsmJs::AsmBr) = 0;
        virtual void AsmBrReg1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1) = 0;
        virtual void AsmBrReg1Const1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, int C1) = 0;
        virtual void WasmMemAccess(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint32 offset, ArrayBufferView::ViewType viewType) = 0;
        virtual uint EnterLoop(ByteCodeLabel loopEntrance) = 0;
        virtual void ExitLoop(uint loopId) = 0;

        virtual void AsmStartCall(OpCodeAsmJs op, ArgSlot ArgCount, bool isPatching = false) = 0;
        virtual void AsmCall(OpCodeAsmJs op, RegSlot returnValueRegister, RegSlot functionRegister, ArgSlot givenArgCount, AsmJsRetType retType) = 0;
    };
}
#endif
