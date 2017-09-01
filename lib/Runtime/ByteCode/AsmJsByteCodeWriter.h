//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#if !defined(ASMJS_BYTECODE_WRITER_H) || defined(WASM_BYTECODE_WRITER)

#ifndef WASM_BYTECODE_WRITER
#define ASMJS_BYTECODE_WRITER_H
#define IMP_IWASM
#else
#define IMP_IWASM virtual
#endif

#if defined(ASMJS_PLAT) || defined(ENABLE_WASM)
namespace Js
{
    struct AsmJsByteCodeWriter : public ByteCodeWriter
#ifdef WASM_BYTECODE_WRITER
    , public IWasmByteCodeWriter
#endif
    {
    private:
        using ByteCodeWriter::MarkLabel;

    public:
        IMP_IWASM void InitData(ArenaAllocator* alloc, int32 initCodeBufferSize);
        IMP_IWASM void EmptyAsm(OpCodeAsmJs op);
        IMP_IWASM void Conv(OpCodeAsmJs op, RegSlot R0, RegSlot R1);
        IMP_IWASM void AsmInt1Const1(OpCodeAsmJs op, RegSlot R0, int C1);
        IMP_IWASM void AsmReg1IntConst1(OpCodeAsmJs op, RegSlot R0, int C1);
        IMP_IWASM void AsmLong1Const1(OpCodeAsmJs op, RegSlot R0, int64 C1);
        IMP_IWASM void AsmFloat1Const1(OpCodeAsmJs op, RegSlot R0, float C1);
        IMP_IWASM void AsmDouble1Const1(OpCodeAsmJs op, RegSlot R0, double C1);
        IMP_IWASM void AsmReg1(OpCodeAsmJs op, RegSlot R0);
        IMP_IWASM void AsmReg2(OpCodeAsmJs op, RegSlot R0, RegSlot R1);
        IMP_IWASM void AsmReg3(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2);
        IMP_IWASM void AsmBr(ByteCodeLabel labelID, OpCodeAsmJs op = OpCodeAsmJs::AsmBr);
        IMP_IWASM void AsmBrReg1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1);
        IMP_IWASM void AsmBrReg1Const1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, int C1);
        IMP_IWASM void AsmStartCall(OpCodeAsmJs op, ArgSlot ArgCount, bool isPatching = false);
        IMP_IWASM void AsmCall(OpCodeAsmJs op, RegSlot returnValueRegister, RegSlot functionRegister, ArgSlot givenArgCount, AsmJsRetType retType);
        IMP_IWASM void AsmSlot(OpCodeAsmJs op, RegSlot value, RegSlot instance, uint32 slotId);
        IMP_IWASM void WasmMemAccess(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint32 offset, ArrayBufferView::ViewType viewType);

        IMP_IWASM void MarkAsmJsLabel(ByteCodeLabel labelID);
        IMP_IWASM uint EnterLoop(ByteCodeLabel loopEntrance);
        IMP_IWASM void ExitLoop(uint loopId);

#ifdef WASM_BYTECODE_WRITER
        // We don't want to expose api not in IWasmByteCodeWriter, but it's easier to compile them anyway
    private:
#endif
        void AsmReg4(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3);
        void AsmReg5(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4);
        void AsmReg6(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5);
        void AsmReg7(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6);
        void AsmReg9(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8);
        void AsmReg10(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8, RegSlot R9);
        void AsmReg11(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8, RegSlot R9, RegSlot R10);
        void AsmReg17(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                      RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16);
        void AsmReg18(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                      RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16, RegSlot R17);
        void AsmReg19(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                      RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16, RegSlot R17, RegSlot R18);
        void AsmBrReg2(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, RegSlot R2);
        void AsmTypedArr(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, ArrayBufferView::ViewType viewType);
        void AsmSimdTypedArr(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint8 dataWidth, ArrayBufferView::ViewType viewType);
    private:
        void AsmJsUnsigned1(OpCodeAsmJs op, uint C1);
        template <typename SizePolicy> bool TryWriteAsmReg1(OpCodeAsmJs op, RegSlot R0);
        template <typename SizePolicy> bool TryWriteAsmReg2(OpCodeAsmJs op, RegSlot R0, RegSlot R1);
        template <typename SizePolicy> bool TryWriteAsmReg3(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2);
        template <typename SizePolicy> bool TryWriteAsmReg4(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3);
        template <typename SizePolicy> bool TryWriteAsmReg5(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4);
        template <typename SizePolicy> bool TryWriteAsmReg6(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5);
        template <typename SizePolicy> bool TryWriteAsmReg7(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6);
        template <typename SizePolicy> bool TryWriteAsmReg9(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8);
        template <typename SizePolicy> bool TryWriteAsmReg10(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8, RegSlot R9);
        template <typename SizePolicy> bool TryWriteAsmReg11(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                                                             RegSlot R9, RegSlot R10);
        template <typename SizePolicy> bool TryWriteAsmReg17(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                                                             RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16);
        template <typename SizePolicy> bool TryWriteAsmReg18(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                                                             RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16, RegSlot R17);
        template <typename SizePolicy> bool TryWriteAsmReg19(OpCodeAsmJs op, RegSlot R0, RegSlot R1, RegSlot R2, RegSlot R3, RegSlot R4, RegSlot R5, RegSlot R6, RegSlot R7, RegSlot R8,
                                                             RegSlot R9, RegSlot R10, RegSlot R11, RegSlot R12, RegSlot R13, RegSlot R14, RegSlot R15, RegSlot R16, RegSlot R17, RegSlot R18);

        template <typename SizePolicy> bool TryWriteInt1Const1(OpCodeAsmJs op, RegSlot R0, int C1);
        template <typename SizePolicy> bool TryWriteReg1IntConst1(OpCodeAsmJs op, RegSlot R0, int C1);
        template <typename SizePolicy> bool TryWriteLong1Const1(OpCodeAsmJs op, RegSlot R0, int64 C1);
        template <typename SizePolicy> bool TryWriteFloat1Const1(OpCodeAsmJs op, RegSlot R0, float C1);
        template <typename SizePolicy> bool TryWriteDouble1Const1(OpCodeAsmJs op, RegSlot R0, double C1);
        template <typename SizePolicy> bool TryWriteAsmBrReg1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1);
        template <typename SizePolicy> bool TryWriteAsmBrReg2(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, RegSlot R2);
        template <typename SizePolicy> bool TryWriteAsmBrReg1Const1(OpCodeAsmJs op, ByteCodeLabel labelID, RegSlot R1, int C1);
        template <typename SizePolicy> bool TryWriteAsmCall(OpCodeAsmJs op, RegSlot returnValueRegister, RegSlot functionRegister, ArgSlot givenArgCount, AsmJsRetType retType);
        template <typename SizePolicy> bool TryWriteAsmSlot(OpCodeAsmJs op, RegSlot value, RegSlot instance, uint32 slotId);
        template <typename SizePolicy> bool TryWriteWasmMemAccess(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint32 offset, ArrayBufferView::ViewType viewType);
        template <typename SizePolicy> bool TryWriteAsmTypedArr(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, ArrayBufferView::ViewType viewType);
        template <typename SizePolicy> bool TryWriteAsmSimdTypedArr(OpCodeAsmJs op, RegSlot value, uint32 slotIndex, uint8 dataWidth, ArrayBufferView::ViewType viewType);
        template <typename SizePolicy> bool TryWriteAsmJsUnsigned1(OpCodeAsmJs op, uint C1);

        void AddJumpOffset(Js::OpCodeAsmJs op, ByteCodeLabel labelId, uint fieldByteOffset);

#ifdef WASM_BYTECODE_WRITER
        // WebAssembly only write functions
    public:
        // IWasmByteCodeWriter Implementation
        virtual void Create() override;
        virtual void Begin(FunctionBody* functionWrite, ArenaAllocator* alloc) override;
        virtual void End() override;
        virtual void Reset() override;
        virtual ByteCodeLabel DefineLabel() override;
#endif
    };
}

#undef IMP_IWASM
#endif

#endif