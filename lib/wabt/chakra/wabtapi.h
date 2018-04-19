//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    typedef void* Var;
}
typedef unsigned int uint;
typedef __int32 int32;
typedef __int64 int64;
typedef int32 PropertyId;

namespace ChakraWabt
{
    namespace PropertyIds
    {
        enum PropertyId
        {
            as,
            action,
            args,
            buffer,
            commands,
            expected,
            field,
            line,
            name,
            module,
            text,
            type,
            value,
            COUNT
        };
    }

    struct Error
    {
        Error(const char* message): message(message) {}
        const char* message;
    };

    typedef bool(*SetPropertyFn)(Js::Var obj, PropertyId, Js::Var value, void* user_data);
    typedef Js::Var(*Int32ToVarFn)(int32, void* user_data);
    typedef Js::Var(*Int64ToVarFn)(int64, void* user_data);
    typedef Js::Var(*StringToVarFn)(const char*, uint length, void* user_data);
    typedef Js::Var(*CreateObjectFn)(void* user_data);
    typedef Js::Var(*CreateArrayFn)(void* user_data);
    typedef void(*PushFn)(Js::Var arr, Js::Var obj, void* user_data);
    struct SpecContext
    {
        SetPropertyFn setProperty;
        Int32ToVarFn int32ToVar;
        Int64ToVarFn int64ToVar;
        StringToVarFn stringToVar;
        CreateObjectFn createObject;
        CreateArrayFn createArray;
        PushFn push;
    };

    typedef Js::Var(*CreateBufferFn)(const unsigned char* start, uint size, void* user_data);
    struct ChakraContext
    {
        CreateBufferFn createBuffer;
        SpecContext* spec;
        void* user_data;

        struct
        {
            bool sign_extends : 1;
            bool threads : 1;
            bool simd : 1;
            bool sat_float_to_int : 1;
        } features;
    };

    Js::Var ConvertWast2Wasm(ChakraContext& chakraCtx, char* buffer, uint bufferSize, bool isSpecText);
};
