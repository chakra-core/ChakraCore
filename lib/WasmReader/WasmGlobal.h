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
        WasmGlobal(GlobalReferenceTypes::Type refType, uint32 offset, WasmTypes::WasmType type, bool isMutable, WasmNode init) :
            m_rType(refType),
            m_offset(offset),
            m_type(type),
            m_isMutable(isMutable),
            m_init(init)
        {};
        WasmTypes::WasmType GetType() const { return m_type; }
        bool IsMutable() const { return m_isMutable; }
        uint32 GetOffset() const { return m_offset; }
        GlobalReferenceTypes::Type GetReferenceType() const { return m_rType; }

        WasmConstLitNode GetConstInit() const;
        uint32 GetGlobalIndexInit() const;
    private:
        GlobalReferenceTypes::Type m_rType;
        WasmTypes::WasmType m_type;
        bool m_isMutable;
        uint32 m_offset;
        WasmNode m_init;
    };
} // namespace Wasm
