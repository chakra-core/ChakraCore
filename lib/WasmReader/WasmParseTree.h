//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    const uint16 EXTENDED_OFFSET = 256;
    namespace Simd {
        const size_t VEC_WIDTH = 4;
        typedef uint32 simdvec [VEC_WIDTH]; //TODO: maybe we should pull in SIMDValue?
        const size_t MAX_LANES = 16;
        void EnsureSimdIsEnabled();
        bool IsEnabled();
    }

    namespace Threads
    {
        bool IsEnabled();
    };

    namespace WasmNontrapping
    {
        bool IsEnabled();
    };

    namespace SignExtends
    {
        bool IsEnabled();
    };

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
#ifdef ENABLE_WASM_SIMD
            M128 = 5,
#endif
            Limit,
            Ptr,
            Any,

            FirstLocalType = I32,
            AllLocalTypes = 
                  1 << I32
                | 1 << I64
                | 1 << F32
                | 1 << F64
#ifdef ENABLE_WASM_SIMD
                | 1 << M128
#endif
        };

        namespace SwitchCaseChecks
        {
            template<WasmType... T>
            struct bv;

            template<>
            struct bv<>
            {
                static constexpr uint value = 0;
            };

            template<WasmType... K>
            struct bv<Limit, K...>
            {
                static constexpr uint value = bv<K...>::value;
            };

            template<WasmType K1, WasmType... K>
            struct bv<K1, K...>
            {
                static constexpr uint value = (1 << K1) | bv<K...>::value;
            };
        }

#ifdef ENABLE_WASM_SIMD
#define WASM_M128_CHECK_TYPE Wasm::WasmTypes::M128
#else
#define WASM_M128_CHECK_TYPE Wasm::WasmTypes::Limit
#endif

        template<WasmType... T>
        __declspec(noreturn) void CompileAssertCases()
        {
            CompileAssertMsg(SwitchCaseChecks::bv<T...>::value == AllLocalTypes, "WasmTypes missing in switch-case");
            AssertOrFailFastMsg(UNREACHED, "The WasmType case should have been handled");
        }

        template<WasmType... T>
        void CompileAssertCasesNoFailFast()
        {
            CompileAssertMsg(SwitchCaseChecks::bv<T...>::value == AllLocalTypes, "WasmTypes missing in switch-case");
            AssertMsg(UNREACHED, "The WasmType case should have been handled");
        }

        extern const char16* const strIds[Limit];

        bool IsLocalType(WasmTypes::WasmType type);
        uint32 GetTypeByteSize(WasmType type);
        const char16* GetTypeName(WasmType type);
    }
    typedef WasmTypes::WasmType Local;

    enum class ExternalKinds: uint8
    {
        Function = 0,
        Table = 1,
        Memory = 2,
        Global = 3,
        Limit
    };

    namespace FunctionIndexTypes
    {
        enum Type
        {
            Invalid = -1,
            ImportThunk,
            Function,
            Import
        };
        bool CanBeExported(Type funcType);
    }

    namespace GlobalReferenceTypes
    {
        enum Type
        {
            Invalid, Const, LocalReference, ImportedReference
        };
    }

    struct WasmOpCodeSignatures
    {
#define WASM_SIGNATURE(id, nTypes, ...) static const WasmTypes::WasmType id[nTypes]; DebugOnly(static const int n##id = nTypes;)
#include "WasmBinaryOpCodes.h"
    };

    enum WasmOp : uint16
    {
#define WASM_OPCODE(opname, opcode, ...) wb##opname = opcode,
// Add prefix to the enum to get a compiler error if there is a collision between operators and prefixes
#define WASM_PREFIX(name, value, ...) prefix##name = value,
#include "WasmBinaryOpCodes.h"
    };

    struct WasmConstLitNode
    {
        union
        {
            float f32;
            double f64;
            int32 i32;
            int64 i64;
            Simd::simdvec v128;
        };
    };

    struct WasmShuffleNode
    {
        uint8 indices[Simd::MAX_LANES];
    };

    struct WasmLaneNode
    {
        uint index;
    };

    struct WasmVarNode
    {
        uint32 num;
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
    private:
        bool isSingleResult;
        union
        {
            WasmTypes::WasmType singleResult;
            uint32 signatureId;
        };
    public:
        bool IsSingleResult() const { return isSingleResult; }
        void SetSignatureId(uint32 id)
        {
            isSingleResult = false;
            signatureId = id;
        }
        uint32 GetSignatureId() const 
        {
            Assert(!isSingleResult);
            return signatureId;
        }
        void SetSingleResult(WasmTypes::WasmType type)
        {
            isSingleResult = true;
            singleResult = type;
        }
        WasmTypes::WasmType GetSingleResult() const
        {
            Assert(isSingleResult);
            return singleResult;
        }
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
            WasmLaneNode lane;
            WasmShuffleNode shuffle;
        };
    };

    struct WasmExport
    {
        uint32 index;
        uint32 nameLength;
        const char16* name;
        ExternalKinds kind;
    };

    struct WasmImport
    {
        ExternalKinds kind;
        uint32 modNameLen;
        const char16* modName;
        uint32 importNameLen;
        const char16* importName;
    };

    struct CustomSection
    {
        const char16* name;
        charcount_t nameLength;
        const byte* payload;
        uint32 payloadSize;
    };
}
