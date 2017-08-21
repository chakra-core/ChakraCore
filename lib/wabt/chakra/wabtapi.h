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
    typedef void*(*AllocatorFn)(uint size, void* user_data);
    struct Context
    {
        AllocatorFn allocator;
        CreateBufferFn createBuffer;
        SpecContext* spec;
        void* user_data;

        void* Allocate(uint size)
        {
            return allocator(size, user_data);
        }
        void Validate(bool isSpec) const;
    };

    Js::Var ConvertWast2Wasm(Context& ctx, char* buffer, uint bufferSize, bool isSpecText);
};
