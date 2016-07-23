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
            Limit,
            Unreachable
        };
        bool IsLocalType(WasmTypes::WasmType type);
    }

#define WASM_SIGNATURE(id, nTypes, ...) struct WasmSignature##id {static const WasmTypes::WasmType types[nTypes];};
#include "WasmBinaryOpcodes.h"

    enum WasmOp : byte
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) wb##opname = opcode,
#include "WasmBinaryOpcodes.h"
        wbFuncEnd,
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
        uint32 arity;
        bool hasSubExpr;
    };

    struct WasmBrTableNode
    {
        uint32 arity;
        uint32 numTargets;
        uint32* targetTable;
        uint32 defaultTarget;
    };

    struct WasmReturnNode
    {
        uint32 arity;
    };

    struct WasmCallNode
    {
        uint32 num; // function id
        uint32 arity;
    };

    struct WasmNode
    {
        WasmOp op;
        union
        {
            WasmVarNode var;
            WasmConstLitNode cnst;
            WasmBrNode br;
            WasmBrTableNode brTable;
            WasmMemOpNode mem;
            WasmReturnNode ret;
            WasmCallNode call;
        };
    };

    struct WasmExport
    {
        uint32 funcIndex;
        uint32 nameLength;
        char16* name;
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
