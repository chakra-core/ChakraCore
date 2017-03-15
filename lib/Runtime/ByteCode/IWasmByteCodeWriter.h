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
        virtual void Create() PURE;
        virtual void Begin(FunctionBody* functionWrite, ArenaAllocator* alloc) PURE;
        virtual void End() PURE;
        virtual void Reset() PURE;
        virtual ByteCodeLabel DefineLabel() PURE;

        virtual void InitData(ArenaAllocator* alloc, int32 initCodeBufferSize) PURE;
        virtual void MarkAsmJsLabel(ByteCodeLabel labelID) PURE;
        virtual void EmptyAsm(OpCodeAsmJs op) PURE;
        virtual void Conv(OpCodeAsmJs op, RegSlot R0, RegSlot R1) PURE;

        virtual void AsmReg1IntConst1(OpCodeAsmJs op, RegSlot R0, int C1) PURE;
        virtual void AsmInt1Const1(OpCodeAsmJs op, RegSlot R0, int C1) PURE;
        virtual void AsmLong1Const1(OpCodeAsmJs op, RegSlot R0, int64 C1) PURE;
        virtual void AsmFloat1Const1(OpCodeAsmJs op, RegSlot R0, float C1) PURE;
        virtual void AsmDouble1Const1(OpCodeAsmJs op, RegSlot R0, double C1) PURE;
        virtual void AsmReg1(OpCodeAsmJs op, RegSlot R0) PURE;
        virtual void AsmReg2(OpCodeAsmJs op, RegSlot R0, RegSlot R1) PURE;
        virtual void AsmReg3(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2) PURE;
        virtual void AsmSlot(OpCodeAsmJs op, RegSlot value, RegSlot instance, int32 slotId) PURE;
        virtual void AsmBr(ByteCodeLabel labelID, OpCodeAsmJs op = OpCodeAsmJs::AsmBr) PURE;
        virtual void AsmBrReg1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1) PURE;
        virtual void AsmBrReg1Const1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, int C1) PURE;
        virtual void WasmMemAccess(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint32 offset, ArrayBufferView::ViewType viewType) PURE;
        virtual uint EnterLoop(ByteCodeLabel loopEntrance) PURE;
        virtual void ExitLoop(uint loopId) PURE;

        virtual void AsmStartCall(OpCodeAsmJs op, ArgSlot ArgCount, bool isPatching = false) PURE;
        virtual void AsmCall(OpCodeAsmJs op, RegSlot returnValueRegister, RegSlot functionRegister, ArgSlot givenArgCount, AsmJsRetType retType) PURE;
    };
}
#endif
