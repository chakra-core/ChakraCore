//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    // TODO (michhol) cleanup forward declarations
    struct EmitInfo;

    class WasmRegisterSpace
    {
    public:
        WasmRegisterSpace();
        WasmRegisterSpace(Js::RegSlot reservedCount);

        Js::RegSlot GetFirstTmpRegister() const;
        Js::RegSlot GetTmpCount() const;
        Js::RegSlot GetVarCount() const;
        Js::RegSlot GetRegisterCount() const;

        Js::RegSlot AcquireRegister();
        Js::RegSlot AcquireTmpRegister();

        void ReleaseTmpRegister(Js::RegSlot tmpReg);

        bool IsTmpReg(Js::RegSlot tmpReg) const;
        bool IsVarReg(Js::RegSlot reg) const;

        void ReleaseLocation(const EmitInfo * info);

        bool IsTmpLocation(const EmitInfo * info);
        bool IsVarLocation(const EmitInfo * info);
        bool IsValidLocation(const EmitInfo * info);
        Js::RegSlot AcquireRegisterAndReleaseLocations(const EmitInfo * lhs, const EmitInfo * rhs);

    private:
        Js::RegSlot m_registerCount;
        Js::RegSlot m_varCount;
        Js::RegSlot m_firstTmpReg;
        Js::RegSlot m_nextLocation;
        Js::RegSlot m_nextConstLocation;
    };
} // namespace Wasm
