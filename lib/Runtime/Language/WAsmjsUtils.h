//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#if !defined(TEMP_DISABLE_ASMJS) || defined(ENABLE_WASM)

namespace WAsmJs 
{
    static const int DOUBLE_SLOTS_SPACE = (sizeof(double) / sizeof(Js::Var)); // 2 in x86 and 1 in x64
    static const double FLOAT_SLOTS_SPACE = (sizeof(float) / (double)sizeof(Js::Var)); // 1 in x86 and 0.5 in x64
    static const double INT_SLOTS_SPACE = (sizeof(int) / (double)sizeof(Js::Var)); // 1 in x86 and 0.5 in x64
    static const double SIMD_SLOTS_SPACE = (sizeof(SIMDValue) / sizeof(Js::Var)); // 4 in x86 and 2 in x64

    typedef Js::RegSlot RegSlot;

    template<typename ToType> uint32 ConvertOffset(uint32 ptr, uint32 fromSize)
    {
        if (fromSize == sizeof(ToType))
        {
            return ptr;
        }
        uint64 tmp = ptr * fromSize;
        tmp = Math::Align<uint64>(tmp, sizeof(ToType));
        tmp /= sizeof(ToType);
        if (tmp > (uint64)UINT32_MAX)
        {
            Math::DefaultOverflowPolicy();
        }
        return (uint32)tmp;
    }
    template<typename FromType, typename ToType> uint32 ConvertOffset(uint32 ptr)
    {
        return ConvertOffset<ToType>(ptr, sizeof(FromType));
    }
    template<typename T> uint32 ConvertToJsVarOffset(uint32 ptr)
    {
        return ConvertOffset<T, Js::Var>(ptr);
    }
    template<typename T> uint32 ConvertFromJsVarOffset(uint32 ptr)
    {
        return ConvertOffset<Js::Var, T>(ptr);
    }

    struct EmitInfoBase
    {
        EmitInfoBase(RegSlot location_) : location(location_) {}
        EmitInfoBase() : location(Js::Constants::NoRegister) {}

        RegSlot location;
    };

    enum Types
    {
        INT32,
        INT64,
        FLOAT32,
        FLOAT64,
        SIMD,
        LIMIT
    };

    /// Register space for const, parameters, variables and tmp values
    ///     --------------------------------------------------------
    ///     | Reserved  | Consts  | Parameters | Variables | Tmp
    ///     --------------------------------------------------------
    ///     Cannot allocate in any different order
    class RegisterSpace
    {
        // Total number of register allocated
        RegSlot   mRegisterCount;

        // location of the first temporary register and last variable + 1
        RegSlot   mFirstTmpReg;

        // Location of the next register to be allocated
        RegSlot   mNextLocation;

        // number of const, includes the reserved slots
        RegSlot    mNbConst;

    public:
        // Constructor
        RegisterSpace(RegSlot reservedSlotsCount = 0) :
            mRegisterCount( reservedSlotsCount )
            , mFirstTmpReg( reservedSlotsCount )
            , mNextLocation( reservedSlotsCount )
            , mNbConst( reservedSlotsCount )
        {
        }
        // Get the number of const allocated
        RegSlot GetConstCount() const      { return mNbConst; }
        // Get the location of the first temporary register
        RegSlot GetFirstTmpRegister() const{ return mFirstTmpReg; }
        // Get the total number of temporary register allocated
        RegSlot GetTmpCount() const        { return mRegisterCount-mFirstTmpReg; }
        // Get number of local variables
        RegSlot GetVarCount() const        { return mFirstTmpReg - mNbConst; }
        // Get the total number of variable allocated ( including temporaries )
        RegSlot GetTotalVarCount() const   { return mRegisterCount - mNbConst; }
        RegSlot GetRegisterCount() const   { return mRegisterCount; }

        // Acquire a location for a register. Use only for arguments and Variables
        RegSlot AcquireRegister()
        {
            // Makes sure no temporary register have been allocated yet
            Assert(mFirstTmpReg == mRegisterCount && mNextLocation == mFirstTmpReg);
            ++mFirstTmpReg;
            ++mRegisterCount;
            return mNextLocation++;
        }

        // Acquire a location for a constant
        RegSlot AcquireConstRegister()
        {
            ++mNbConst;
            return AcquireRegister();
        }

        // Acquire a location for a temporary register
        RegSlot AcquireTmpRegister()
        {
            // Make sure this function is called correctly
            Assert(mNextLocation <= mRegisterCount && mNextLocation >= mFirstTmpReg);

            // Allocate a new temp pseudo-register, increasing the locals count if necessary.
            if(mNextLocation == mRegisterCount)
            {
                ++mRegisterCount;
            }
#if DBG_DUMP
            PrintTmpRegisterAllocation(mNextLocation);
#endif
            return mNextLocation++;
        }

        // Release a location for a temporary register, must be the last location acquired
        void ReleaseTmpRegister( RegSlot tmpReg )
        {
            // make sure the location released is valid
            Assert(tmpReg != Js::Constants::NoRegister);

            // Put this reg back on top of the temp stack (if it's a temp).
            if( this->IsTmpReg( tmpReg ) )
            {
                Assert( tmpReg == this->mNextLocation - 1 );
#if DBG_DUMP
                PrintTmpRegisterAllocation(mNextLocation - 1, true);
#endif
                mNextLocation--;
            }
        }

        // Checks if the register is a temporary register
        bool IsTmpReg(RegSlot tmpReg)
        {
            Assert(mFirstTmpReg != Js::Constants::NoRegister);
            return !IsConstReg(tmpReg) && tmpReg >= mFirstTmpReg;
        }

        // Checks if the register is a const register
        bool IsConstReg(RegSlot reg)
        {
            // a register is const if it is between the first register and the end of consts
            return reg < mNbConst && reg != 0;
        }

        // Checks if the register is a variable register
        bool IsVarReg(RegSlot reg)
        {
            // a register is a var if it is between the last const and the end
            // equivalent to  reg>=mNbConst && reg<mRegisterCount
            // forcing unsigned, if reg < mNbConst then reg-mNbConst = 0xFFFFF..
            return (uint32)(reg - mNbConst) < (uint32)(mRegisterCount - mNbConst);
        }

        // Releases a location if its a temporary, safe to call with any expression
        void ReleaseLocation(const EmitInfoBase *pnode)
        {
            // Release the temp assigned to this expression so it can be re-used.
            if(pnode && pnode->location != Js::Constants::NoRegister)
            {
                ReleaseTmpRegister(pnode->location);
            }
        }

        // Checks if the location points to a temporary register
        bool IsTmpLocation(const EmitInfoBase* pnode)
        {
            if(pnode && pnode->location != Js::Constants::NoRegister)
            {
                return IsTmpReg(pnode->location);
            }
            return false;
        }

        // Checks if the location points to a constant register
        bool IsConstLocation(const EmitInfoBase* pnode)
        {
            if(pnode && pnode->location != Js::Constants::NoRegister)
            {
                return IsConstReg(pnode->location);
            }
            return false;
        }

        // Checks if the location points to a variable register
        bool IsVarLocation(const EmitInfoBase* pnode)
        {
            if(pnode && pnode->location != Js::Constants::NoRegister)
            {
                return IsVarReg(pnode->location);
            }
            return false;
        }

        // Checks if the location is valid (within bounds of already allocated registers)
        bool IsValidLocation(const EmitInfoBase* pnode)
        {
            if(pnode && pnode->location != Js::Constants::NoRegister)
            {
                return pnode->location < mRegisterCount;
            }
            return false;
        }

        template<typename T> static Types GetRegisterSpaceType(){return WAsmJs::LIMIT;}
        template<> static Types GetRegisterSpaceType<int32>(){return WAsmJs::INT32;}
        template<> static Types GetRegisterSpaceType<int64>(){return WAsmJs::INT64;}
        template<> static Types GetRegisterSpaceType<float>(){return WAsmJs::FLOAT32;}
        template<> static Types GetRegisterSpaceType<double>(){return WAsmJs::FLOAT64;}
        template<> static Types GetRegisterSpaceType<AsmJsSIMDValue>(){return WAsmJs::SIMD;}
        static uint32 GetTypeByteSize(Types type);
#if DBG_DUMP
        // Used for debugging
        Types mType;
        static void GetTypeDebugName(Types type, char16* buf, uint bufsize, bool shortName = false);
        void PrintTmpRegisterAllocation(RegSlot loc, bool deallocation = false);
#endif
    };

    struct TypedSlotInfo
    {
        uint32 constCount;
        uint32 varCount;
        uint32 tmpCount;
        uint32 byteOffset;
    };

    typedef RegisterSpace*(*AllocateRegisterSpaceFunc)(ArenaAllocator*, WAsmJs::Types);
    class TypedRegisterAllocator
    {
        uint32 mExcludedMask;
        RegisterSpace* mTypeSpaces[WAsmJs::LIMIT];
    public:
        TypedRegisterAllocator(ArenaAllocator* allocator, AllocateRegisterSpaceFunc allocateFunc, uint32 excludedMask = 0);

        uint32 GetJsVarCount(Types type, bool constOnly /*= false*/) const;
        uint32 GetTotalJsVarCount(bool constOnly = false) const;
        void CommitToFunctionInfo(Js::AsmJsFunctionInfo* funcInfo) const;
        void CommitToFunctionBody(Js::FunctionBody* body);
        bool IsTypeExcluded(Types type) const;
#if DBG_DUMP
        void DumpLocalsInfo() const;
        // indexes' array size must be WAsmJs::RegisterSpace::LIMIT
        void GetArgumentStartIndex(uint32* indexes) const;
#endif

        RegisterSpace* GetRegisterSpace(Types type) const;
    private:
        bool IsValidType(Types type) const;
    };
};

#endif
