//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    namespace ReferenceTypes
    {
        enum Type
        {
            Invalid, Const, LocalReference, ImportedReference
        };
    }

    class WasmGlobal
    {
    public:
        WasmGlobal(ReferenceTypes::Type refType, uint offset, WasmTypes::WasmType type, bool isMutable, WasmNode init) :
            m_rType(refType),
            m_offset(offset),
            m_type(type),
            m_isMutable(isMutable),
            m_init(init)
        {};
        WasmTypes::WasmType GetType() const { return m_type; }
        bool IsMutable() const { return m_isMutable; }
        uint GetOffset() const { return m_offset; }
        ReferenceTypes::Type GetReferenceType() const { return m_rType; }

        WasmConstLitNode GetConstInit() const;
        uint32 GetGlobalIndexInit() const;
    private:
        ReferenceTypes::Type m_rType;
        WasmTypes::WasmType m_type;
        bool m_isMutable;
        uint m_offset;
        WasmNode m_init;
    };
} // namespace Wasm
