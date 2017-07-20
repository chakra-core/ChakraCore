//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_SIMDJS
namespace Js {

    class SIMDUint16x8Lib
    {
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo Uint16x8;
            static FunctionInfo Check;
            static FunctionInfo Splat;
            // Conversions
            static FunctionInfo FromFloat32x4Bits;
            static FunctionInfo FromInt32x4Bits;
            static FunctionInfo FromInt8x16Bits;
            static FunctionInfo FromInt16x8Bits;
            static FunctionInfo FromUint32x4Bits;
            static FunctionInfo FromUint8x16Bits;
            // UnaryOps
            static FunctionInfo Not;
            static FunctionInfo Neg;
            // BinaryOps
            static FunctionInfo Add;
            static FunctionInfo Sub;
            static FunctionInfo Mul;
            static FunctionInfo And;
            static FunctionInfo Or;
            static FunctionInfo Xor;
            static FunctionInfo Max;
            static FunctionInfo Min;
            // CompareOps
            static FunctionInfo LessThan;
            static FunctionInfo LessThanOrEqual;
            static FunctionInfo Equal;
            static FunctionInfo NotEqual;
            static FunctionInfo GreaterThan;
            static FunctionInfo GreaterThanOrEqual;
            // ShiftOps
            static FunctionInfo ShiftLeftByScalar;
            static FunctionInfo ShiftRightByScalar;
            // Others
            static FunctionInfo ExtractLane;
            static FunctionInfo ReplaceLane;
            static FunctionInfo Shuffle;
            static FunctionInfo Swizzle;
            static FunctionInfo Select;
            static FunctionInfo Load;
            static FunctionInfo Store;
            static FunctionInfo AddSaturate;
            static FunctionInfo SubSaturate;
        };

        // Entry points to library
        // constructor
        static Var EntryUint16x8(RecyclableObject* function, CallInfo callInfo, ...);
        // type-check
        static Var EntryCheck(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySplat(RecyclableObject* function, CallInfo callInfo, ...);
        //Type Conversions
        static Var EntryFromFloat32x4Bits(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFromInt32x4Bits(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFromInt8x16Bits(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFromInt16x8Bits(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFromUint32x4Bits(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFromUint8x16Bits(RecyclableObject* function, CallInfo callInfo, ...);
        //// UnaryOps
        static Var EntryNot(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryNeg(RecyclableObject* function, CallInfo callInfo, ...);
        //// BinaryOps
        static Var EntryAdd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySub(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryMul(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAnd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryOr(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryXor(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryMax(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryMin(RecyclableObject* function, CallInfo callInfo, ...);
        //// CompareOps
        static Var EntryLessThan(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLessThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEqual(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryNotEqual(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGreaterThan(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGreaterThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...);
        //// ShiftOps
        static Var EntryShiftLeftByScalar(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryShiftRightByScalar(RecyclableObject* function, CallInfo callInfo, ...);
        // Lane Access
        static Var EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLoad(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryStore(RecyclableObject* function, CallInfo callInfo, ...);
        //Other
        static Var EntryShuffle(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySwizzle(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySelect(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAddSaturate(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySubSaturate(RecyclableObject* function, CallInfo callInfo, ...);
        // End entry points
    };

} // namespace Js
#endif
