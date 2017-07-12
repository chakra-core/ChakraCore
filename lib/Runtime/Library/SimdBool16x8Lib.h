//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js {
    class SIMDBool16x8Lib
    {
    public:
        class EntryInfo
        {
        public:
            // Bool16x8
            static FunctionInfo Bool16x8;
            static FunctionInfo Check;
            static FunctionInfo Splat;

            // UnaryOps
            static FunctionInfo Not;
            static FunctionInfo AllTrue;
            static FunctionInfo AnyTrue;

            // BinaryOps
            static FunctionInfo And;
            static FunctionInfo Or;
            static FunctionInfo Xor;

            //Lane access
            static FunctionInfo ExtractLane;
            static FunctionInfo ReplaceLane;
        };

        // Entry points to library
        // constructor
        static Var EntryBool16x8(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryCheck(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySplat(RecyclableObject* function, CallInfo callInfo, ...);

        // Lane Access
        static Var EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...);

        // UnaryOps
        static Var EntryNot(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAllTrue(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAnyTrue(RecyclableObject* function, CallInfo callInfo, ...);

        // BinaryOps
        static Var EntryAnd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryOr(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryXor(RecyclableObject* function, CallInfo callInfo, ...);

        // End entry points
    };
} // namespace Js
#endif
