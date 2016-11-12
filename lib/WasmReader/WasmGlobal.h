//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{

    class WasmGlobal
    {


    public:

        enum ReferenceType { Invalid, Const, LocalReference, ImportedReference };

        WasmGlobal(uint offset, WasmTypes::WasmType type, bool mutability) : m_type(type), m_mutability(mutability), m_offset(offset) {};
        WasmTypes::WasmType GetType() const;
        bool GetMutability() const;
        ReferenceType GetReferenceType() { return m_rType; }
        void SetReferenceType(ReferenceType rt) { m_rType = rt; }
        uint GetOffset() { return m_offset; }
        void SetOffset(uint offset) { m_offset = offset; }

        union
        {
            WasmConstLitNode cnst;
            uint32 importIndex;
        };

    private:

        ReferenceType m_rType;
        WasmTypes::WasmType m_type;
        bool m_mutability;
        uint m_offset;

    };
} // namespace Wasm
