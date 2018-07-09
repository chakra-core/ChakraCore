//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    enum CallFlags : unsigned
    {
        CallFlags_None    = 0,
        CallFlags_New     = 1,
        CallFlags_Value   = 2,
        CallFlags_Eval    = 4,
        CallFlags_ExtraArg = 8,
        CallFlags_NotUsed = 0x10,
        CallFlags_Wrapped = 0x20,
        CallFlags_NewTarget = 0x40,
        CallFlags_InternalFrame = 0x80
    };
    ENUM_CLASS_HELPERS(CallFlags, unsigned)

    struct CallInfo
    {
        CallInfo() {}

        /*
         * Removed the copy constructor because it forced the 64 bit compiler
         * to pass this object by reference. Interpreter stack setup code expects
         * CallInfo to be passed by value.
         */
        explicit CallInfo(ArgSlot count)
            : Flags(CallFlags_None)
            , Count(count)
#ifdef TARGET_64
            , unused(0)
#endif
        {
            // Keeping this version to avoid the assert
        }

        // The bool is used to avoid the signature confusion between the ArgSlot and uint version of the constructor
        explicit CallInfo(uint count, bool unusedBool)
            : Flags(CallFlags_None)
            , Count(count)
#ifdef TARGET_64
            , unused(0)
#endif
        {
            AssertOrFailFastMsg(count < CallInfo::kMaxCountArgs, "Argument list too large");
        }

        CallInfo(CallFlags flags, uint count)
            : Flags(flags)
            , Count(count)
#ifdef TARGET_64
            , unused(0)
#endif
        {
            // Keeping this version to avoid the assert
        }


        CallInfo(VirtualTableInfoCtorEnum v)
        {
        }

        ArgSlot GetArgCountWithExtraArgs() const
        {
            return CallInfo::GetArgCountWithExtraArgs(this->Flags, this->Count);
        }

        uint GetLargeArgCountWithExtraArgs() const
        {
            return CallInfo::GetLargeArgCountWithExtraArgs(this->Flags, this->Count);
        }

        bool HasExtraArg() const
        {
            return  CallInfo::HasExtraArg(this->Flags);
        }

        bool HasNewTarget() const
        {
            return CallInfo::HasNewTarget(this->Flags);
        }

        static ArgSlot GetArgCountWithExtraArgs(CallFlags flags, uint count);

        static uint GetLargeArgCountWithExtraArgs(CallFlags flags, uint count);

        static ArgSlot GetArgCountWithoutExtraArgs(CallFlags flags, ArgSlot count);

        static bool HasExtraArg(CallFlags flags)
        {
            // Generally HasNewTarget should not be true if CallFlags_ExtraArg is not set.
            Assert(!CallInfo::HasNewTarget(flags) || flags & CallFlags_ExtraArg);

            // we will still check HasNewTarget to be safe in case if above invariant does not hold.
            return (flags & CallFlags_ExtraArg) || CallInfo::HasNewTarget(flags);
        }

        static bool HasNewTarget(CallFlags flags)
        {
            return (flags & CallFlags_NewTarget) == CallFlags_NewTarget;
        }

        // New target value is passed as an extra argument which is nto included in the Count
        static Var GetNewTarget(CallFlags flag, Var* values, uint count)
        {
            if (HasNewTarget(flag))
            {
                return values[count];
            }
            else
            {
                AssertOrFailFast(count > 0);
                return values[0];
            }
        }

        // Assumes big-endian layout
        // If the size of the count is changed, change should happen at following places also
        //  - scriptdirect.idl
        //  - LowererMDArch::LoadInputParamCount
        //
        Field(unsigned)  Count : 24;
        Field(CallFlags) Flags : 8;
#ifdef TARGET_64
        Field(unsigned) unused : 32;
#endif

#if DBG
        bool operator==(CallInfo other) const
        {
            return this->Count == other.Count && this->Flags == other.Flags;
        }
#endif
    public:
        static const ushort ksizeofCount;
        static const ushort ksizeofCallFlags;
        static const uint kMaxCountArgs;
    };

    struct InlineeCallInfo
    {
        // Assumes big-endian layout.
        size_t Count: 4;
        size_t InlineeStartOffset: sizeof(void*) * CHAR_BIT - 4;
        static size_t const MaxInlineeArgoutCount = 0xF;

        static bool Encode(intptr_t &callInfo, size_t count, size_t offset)
        {
            const size_t offsetMask = (~(size_t)0) >> 4;
            const size_t countMask  = 0x0000000F;
            if (count != (count & countMask))
            {
                return false;
            }

            if (offset != (offset & offsetMask))
            {
                return false;
            }

            callInfo = (offset << 4) | count;

            return true;
        }

        void Clear()
        {
            this->Count = 0;
            this->InlineeStartOffset = 0;
        }
    };
}
