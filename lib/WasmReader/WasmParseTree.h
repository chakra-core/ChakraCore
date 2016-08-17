//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    namespace WasmTypes
    {
        enum WasmType
        {
            // based on binary format encoding values
            Void = 0,
            I32 = 1,
            I64 = 2,
            F32 = 3,
            F64 = 4,
            Limit
        };
        bool IsLocalType(WasmTypes::WasmType type);
    }

    namespace ImportKinds
    {
        enum ImportKind
        {
            Function = 0,
            Table = 1,
            Memory = 2,
            Global = 3
        };
    }

    namespace ElementTypes
    {
        enum Type
        {
            anyfunc = 0x20
        };
    }

    namespace FunctionIndexTypes
    {
        enum Type
        {
            Invalid = -1,
            Function,
            Import
        };
    }

    struct WasmOpCodeSignatures
    {
#define WASM_SIGNATURE(id, nTypes, ...) static const WasmTypes::WasmType id[nTypes]; DebugOnly(static const int n##id = nTypes;)
#include "WasmBinaryOpcodes.h"
    };

    enum WasmOp : byte
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) wb##opname = opcode,
#include "WasmBinaryOpcodes.h"
        wbLimit
    };

    struct WasmConstLitNode
    {
        union
        {
            float f32;
            double f64;
            int32 i32;
            int64 i64;
        };
    };

    struct WasmVarNode
    {
        uint num;
        union
        {
            LPCUTF8 exportName;
        };
    };

    struct WasmMemOpNode
    {
        uint32 offset;
        uint8 alignment;
    };

    struct WasmBrNode
    {
        uint32 depth;
    };

    struct WasmBrTableNode
    {
        uint32 numTargets;
        uint32* targetTable;
        uint32 defaultTarget;
    };

    struct WasmCallNode
    {
        uint32 num; // function id
        FunctionIndexTypes::Type funcType;
    };

    struct WasmBlock
    {
        WasmTypes::WasmType sig;
    };

    struct WasmNode
    {
        WasmOp op;
        union
        {
            WasmBlock block;
            WasmBrNode br;
            WasmBrTableNode brTable;
            WasmCallNode call;
            WasmConstLitNode cnst;
            WasmMemOpNode mem;
            WasmVarNode var;
        };
    };

    struct WasmExport
    {
        uint32 funcIndex;
        uint32 nameLength;
        char16* name;
        ImportKinds::ImportKind kind;
    };

    struct WasmImport
    {
        uint32 sigId;
        uint32 modNameLen;
        char16* modName;
        uint32 fnNameLen;
        char16* fnName;
    };
}
