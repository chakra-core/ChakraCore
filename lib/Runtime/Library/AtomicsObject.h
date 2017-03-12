//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements Atomics according to http://tc39.github.io/ecmascript_sharedmem/shmem.html
//----------------------------------------------------------------------------

#pragma once
namespace Js
{
    class AtomicsObject
    {
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo Add;
            static FunctionInfo And;
            static FunctionInfo CompareExchange;
            static FunctionInfo Exchange;
            static FunctionInfo IsLockFree;
            static FunctionInfo Load;
            static FunctionInfo Or;
            static FunctionInfo Store;
            static FunctionInfo Sub;
            static FunctionInfo Wait;
            static FunctionInfo Wake;
            static FunctionInfo Xor;
        };

        static Var EntryAdd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAnd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryCompareExchange(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryExchange(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsLockFree(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLoad(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryOr(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryStore(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySub(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryWait(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryWake(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryXor(RecyclableObject* function, CallInfo callInfo, ...);

    private:
        static Var ValidateSharedIntegerTypedArray(Var typedArray, ScriptContext *scriptContext, bool onlyInt32);
        static uint32 ValidateAtomicAccess(Var typedArray, Var index, ScriptContext *scriptContext);

        static TypedArrayBase * ValidateAndGetTypedArray(Var typedArray, Var index, __out uint32 *accessIndex, ScriptContext *scriptContext, bool onlyInt32 = false);
    };
}
