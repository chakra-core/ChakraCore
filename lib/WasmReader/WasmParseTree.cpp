//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

namespace Wasm
{
namespace Simd
{
bool IsEnabled()
{
#ifdef ENABLE_WASM_SIMD
    return CONFIG_FLAG_RELEASE(WasmSimd);
#else
    return false;
#endif
}

}

namespace Threads
{
bool IsEnabled()
{
#ifdef ENABLE_WASM
    return CONFIG_FLAG(WasmThreads) && CONFIG_FLAG(ESSharedArrayBuffer);
#else
    return false;
#endif
}
}

namespace WasmNontrapping
{
    bool IsEnabled()
    {
#ifdef ENABLE_WASM
        return CONFIG_FLAG(WasmNontrapping);
#else
        return false;
#endif
    }
}

namespace SignExtends
{
bool IsEnabled()
{
#ifdef ENABLE_WASM
    return CONFIG_FLAG(WasmSignExtends);
#else
    return false;
#endif
}
}

}


#ifdef ENABLE_WASM

namespace Wasm
{
namespace Simd
{
void EnsureSimdIsEnabled()
{
    if (!Wasm::Simd::IsEnabled())
    {
        throw WasmCompilationException(_u("Wasm.Simd support is not enabled"));
    }
}
}

namespace WasmTypes
{

bool IsLocalType(WasmTypes::WasmType type)
{
    // Check if type in range ]Void,Limit[
#ifdef ENABLE_WASM_SIMD
    if (type == WasmTypes::M128 && !Simd::IsEnabled())
    {
        return false;
    }
#endif
    return type >= WasmTypes::FirstLocalType && type < WasmTypes::Limit;
}

uint32 GetTypeByteSize(WasmType type)
{
    switch (type)
    {
    case Void: return sizeof(Js::Var);
    case I32: return sizeof(int32);
    case I64: return sizeof(int64);
    case F32: return sizeof(float);
    case F64: return sizeof(double);
#ifdef ENABLE_WASM_SIMD
    case M128:
        Simd::EnsureSimdIsEnabled();
        CompileAssert(sizeof(Simd::simdvec) == 16);
        return sizeof(Simd::simdvec);
#endif
    case Ptr: return sizeof(void*);
    default:
        Js::Throw::InternalError();
    }
}

const char16 * GetTypeName(WasmType type)
{
    switch (type) {
    case WasmTypes::WasmType::Void: return _u("void");
    case WasmTypes::WasmType::I32: return _u("i32");
    case WasmTypes::WasmType::I64: return _u("i64");
    case WasmTypes::WasmType::F32: return _u("f32");
    case WasmTypes::WasmType::F64: return _u("f64");
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::WasmType::M128: 
        Simd::EnsureSimdIsEnabled();
        return _u("m128");
#endif
    case WasmTypes::WasmType::Any: return _u("any");
    default: Assert(UNREACHED); break;
    }
    return _u("unknown");
}

} // namespace WasmTypes

WasmTypes::WasmType LanguageTypes::ToWasmType(int8 binType)
{
    switch (binType)
    {
    case LanguageTypes::i32: return WasmTypes::I32;
    case LanguageTypes::i64: return WasmTypes::I64;
    case LanguageTypes::f32: return WasmTypes::F32;
    case LanguageTypes::f64: return WasmTypes::F64;
#ifdef ENABLE_WASM_SIMD
    case LanguageTypes::m128:
        Simd::EnsureSimdIsEnabled();
        return WasmTypes::M128;
#endif
    default:
        throw WasmCompilationException(_u("Invalid binary type %d"), binType);
    }
}

bool FunctionIndexTypes::CanBeExported(FunctionIndexTypes::Type funcType)
{
    return funcType == FunctionIndexTypes::Function || funcType == FunctionIndexTypes::ImportThunk;
}

} // namespace Wasm
#endif // ENABLE_WASM
