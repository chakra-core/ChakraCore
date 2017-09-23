//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js {
    class SIMDBool32x4Lib
    {
    public:
        class EntryInfo
        {
        public:
            // Bool32x4
            static FunctionInfo Bool32x4;
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
        static Var EntryBool32x4(RecyclableObject* function, CallInfo callInfo, ...);
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
