//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements Atomics according to http://tc39.github.io/ecmascript_sharedmem/shmem.html
//----------------------------------------------------------------------------

#pragma once
namespace Js
{
    class AtomicsOperations
    {
    public:
        template<typename T> static T Load(T* buffer);
        template<typename T> static T Store(T* buffer, T value);
        template<typename T> static T Add(T* buffer, T value);
        template<typename T> static T And(T* buffer, T value);
        template<typename T> static T CompareExchange(T* buffer, T comparand, T replacementValue);
        template<typename T> static T Exchange(T* buffer, T value);
        template<typename T> static T Or(T* buffer, T value);
        template<typename T> static T Sub(T* buffer, T value);
        template<typename T> static T Xor(T* buffer, T value);
    };
}
