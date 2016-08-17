//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#if !defined(TEMP_DISABLE_ASMJS) || defined(ENABLE_WASM)

namespace WAsmJs
{
    uint32 GetTypeByteSize(Types type)
    {
        // Since this needs to be done manually for each type, this assert will make sure to not forget to update this if a new type is added
        CompileAssert(WAsmJs::LIMIT == 5);
        switch (type)
        {
        case INT32  : return sizeof(int32);
        case INT64  : return sizeof(int64);
        case FLOAT32: return sizeof(float);
        case FLOAT64: return sizeof(double);
        case SIMD   : return sizeof(AsmJsSIMDValue);
        default     : break;
        }
        Js::Throw::InternalError();
    }

#if DBG_DUMP
    void RegisterSpace::GetTypeDebugName(Types type, char16* buf, uint bufsize, bool shortName)
    {
        // Since this needs to be done manually for each type, this assert will make sure to not forget to update this if a new type is added
        CompileAssert(LIMIT == 5);

        switch (type)
        {
        case INT32: wcscpy_s(buf, bufsize  , shortName ? _u("I"): _u("INT32")); break;
        case INT64: wcscpy_s(buf, bufsize  , shortName ? _u("L"): _u("INT64")); break;
        case FLOAT32: wcscpy_s(buf, bufsize, shortName ? _u("F"): _u("FLOAT32")); break;
        case FLOAT64: wcscpy_s(buf, bufsize, shortName ? _u("D"): _u("FLOAT64")); break;
        case SIMD: wcscpy_s(buf, bufsize   , _u("SIMD")); break;
        default: wcscpy_s(buf, bufsize     , _u("UNKNOWN")); break;
        }
    }

    void RegisterSpace::PrintTmpRegisterAllocation(RegSlot loc, bool deallocation)
    {
        if (PHASE_TRACE1(Js::AsmjsTmpRegisterAllocationPhase))
        {
            char16 buf[16];
            GetTypeDebugName(mType, buf, 16, true);
            Output::Print(_u("%s%s %d\n"), deallocation ? _u("-") : _u("+"), buf, loc);
        }
    }
#endif

    TypedRegisterAllocator::TypedRegisterAllocator(ArenaAllocator* allocator, AllocateRegisterSpaceFunc allocateFunc, uint32 excludedMask/* = 0*/)
    {
        Assert(allocateFunc);
        AssertMsg(excludedMask >> WAsmJs::LIMIT == 0, "Invalid type in the excluded mask");
        mExcludedMask = excludedMask;

        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            Types type = (Types)i;
            mTypeSpaces[i] = nullptr;
            if (!IsTypeExcluded(type))
            {
                mTypeSpaces[i] = allocateFunc(allocator, type);
#if DBG_DUMP
                mTypeSpaces[i]->mType = type;
#endif
            }
        }
    }

    uint32 TypedRegisterAllocator::GetJsVarCount(Types type, bool constOnly /*= false*/) const
    {
        if (!IsTypeExcluded(type))
        {
            RegisterSpace* registerSpace = GetRegisterSpace(type);
            uint32 typeSize = GetTypeByteSize(type);
            uint32 count = constOnly ? registerSpace->GetConstCount() : registerSpace->GetTotalVarCount();
            return ConvertOffset<Js::Var>(count, typeSize);
        }
        return 0;
    }

    uint32 TypedRegisterAllocator::GetTotalJsVarCount(bool constOnly /* = false*/) const
    {
        uint32 total = 0;
        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            Types type = (Types)i;
            total = UInt32Math::Add(total, GetJsVarCount(type, constOnly));
        }
        return total;
    }

    void TypedRegisterAllocator::CommitToFunctionInfo(Js::AsmJsFunctionInfo* funcInfo) const
    {
        uint32 offset = Js::AsmJsFunctionMemory::RequiredVarConstants * sizeof(Js::Var);
        WAsmJs::TypedConstSourcesInfo constSourcesInfo = GetConstSourceInfos();

#if DBG_DUMP
        if (PHASE_TRACE1(Js::AsmjsInterpreterStackPhase))
        {
            Output::Print(_u("ASMFunctionInfo Stack Data\n"));
            Output::Print(_u("==========================\n"));
            Output::Print(_u("RequiredVarConstants:%d\n"), Js::AsmJsFunctionMemory::RequiredVarConstants);
        }
#endif

        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            Types type = (Types)i;

            TypedSlotInfo* slotInfo = funcInfo->GetTypedSlotInfo(type);
            // Check if we don't want to commit this type
            if (!IsTypeExcluded(type))
            {
                RegisterSpace* registerSpace = GetRegisterSpace(type);
                slotInfo->isValidType = true;
                slotInfo->constCount = registerSpace->GetConstCount();
                slotInfo->varCount = registerSpace->GetVarCount();
                slotInfo->tmpCount = registerSpace->GetTmpCount();
                slotInfo->constSrcByteOffset = constSourcesInfo.srcByteOffsets[i];

                offset = Math::AlignOverflowCheck(offset, GetTypeByteSize(type));
                slotInfo->byteOffset = offset;

#if DBG_DUMP
                if (PHASE_TRACE1(Js::AsmjsInterpreterStackPhase))
                {
                    char16 buf[16];
                    RegisterSpace::GetTypeDebugName(type, buf, 16);
                    Output::Print(_u("%s Offset:%d  ConstCount:%d  VarCount:%d  TmpCount:%d\n"),
                                  buf,
                                  slotInfo->byteOffset,
                                  slotInfo->constCount,
                                  slotInfo->varCount,
                                  slotInfo->tmpCount);
                }
#endif

                // Update offset for next type
                uint32 totalTypeCount = 0;
                totalTypeCount = UInt32Math::Add(totalTypeCount, slotInfo->constCount);
                totalTypeCount = UInt32Math::Add(totalTypeCount, slotInfo->varCount);
                totalTypeCount = UInt32Math::Add(totalTypeCount, slotInfo->tmpCount);

                offset = UInt32Math::Add(offset, UInt32Math::Mul(totalTypeCount, GetTypeByteSize(type)));
            }
            else
            {
                memset(slotInfo, 0, sizeof(TypedSlotInfo));
                slotInfo->isValidType = false;
            }
        }
#if DBG_DUMP
        if (PHASE_TRACE1(Js::AsmjsInterpreterStackPhase))
        {
            Output::Print(_u("\n"));
        }
#endif
    }

    void TypedRegisterAllocator::CommitToFunctionBody(Js::FunctionBody* body)
    {
        // this value is the number of Var slots needed to allocate all the const
        const uint32 nbConst = UInt32Math::Add(Js::AsmJsFunctionMemory::RequiredVarConstants, GetTotalJsVarCount(true));
        body->CheckAndSetConstantCount(nbConst);
    }

    WAsmJs::TypedConstSourcesInfo TypedRegisterAllocator::GetConstSourceInfos() const
    {
        WAsmJs::TypedConstSourcesInfo infos;
        // The const table doesn't contain the first reg slots (aka return register)
        uint32 offset = ConvertOffset<Js::Var, byte>(Js::AsmJsFunctionMemory::RequiredVarConstants - Js::FunctionBody::FirstRegSlot);
        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            Types type = (Types)i;
            if (!IsTypeExcluded(type))
            {
                infos.srcByteOffsets[i] = offset;
                uint32 typeBytesUsage = ConvertOffset<byte>(GetRegisterSpace(type)->GetConstCount(), GetTypeByteSize(type));
                offset = UInt32Math::Add(offset, typeBytesUsage);
            }
            else
            {
                infos.srcByteOffsets[i] = 0;
            }
        }
        return infos;
    }

    bool TypedRegisterAllocator::IsValidType(Types type) const
    {
        return (uint)type < WAsmJs::LIMIT;
    }

    bool TypedRegisterAllocator::IsTypeExcluded(Types type) const
    {
        return !IsValidType(type) || (mExcludedMask & (1 << type)) != 0;
    }

#if DBG_DUMP
    void TypedRegisterAllocator::DumpLocalsInfo() const
    {
        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            Types type = (Types)i;
            if (!IsTypeExcluded(type))
            {
                char16 typeName[16];
                char16 shortTypeName[16];
                RegisterSpace::GetTypeDebugName(type, typeName, 16);
                RegisterSpace::GetTypeDebugName(type, shortTypeName, 16, true);
                RegisterSpace* registerSpace = GetRegisterSpace(type);
                Output::Print(
                    _u("     %-10s : %u locals (%u temps from %s%u)\n"),
                    typeName,
                    registerSpace->GetVarCount(),
                    registerSpace->GetTmpCount(),
                    shortTypeName,
                    registerSpace->GetFirstTmpRegister());
            }
        }
    }

    void TypedRegisterAllocator::GetArgumentStartIndex(uint32* indexes) const
    {
        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            Types type = (Types)i;
            if (!IsTypeExcluded(type))
            {
                // Arguments starts right after the consts
                indexes[i] = GetRegisterSpace(type)->GetConstCount();
            }
        }
    }
#endif

    RegisterSpace* TypedRegisterAllocator::GetRegisterSpace(Types type) const
    {
        if (!IsValidType(type))
        {
            Assert("Invalid type for RegisterSpace in TypedMemoryStructure");
            Js::Throw::InternalError();
        }
        Assert(!IsTypeExcluded(type));
        return mTypeSpaces[type];
    }
};

#endif
