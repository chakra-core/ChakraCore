//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// Portions of this file are copyright 2014 Mozilla Foundation, available under the Apache 2.0 license.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef TEMP_DISABLE_ASMJS

namespace WAsmJs 
{
    typedef Js::RegSlot RegSlot;

    struct EmitInfoBase
    {
        EmitInfoBase(RegSlot location_) : location(location_) {}
        EmitInfoBase() : location(Js::Constants::NoRegister) {}

        RegSlot location;
    };

    /// Register space for const, parameters, variables and tmp values
    ///     --------------------------------------------------------
    ///     | 0 (Reserved) | Consts  | Parameters | Variables | Tmp
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
            Assert( reservedSlotsCount >= 0 );
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
            Assert( mFirstTmpReg == mRegisterCount && mNextLocation == mFirstTmpReg );
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
            Assert( this->mNextLocation <= this->mRegisterCount && this->mNextLocation >= this->mFirstTmpReg );

            // Allocate a new temp pseudo-register, increasing the locals count if necessary.
            if( this->mNextLocation == this->mRegisterCount )
            {
                ++this->mRegisterCount;
            }
#if DBG_DUMP
            PrintTmpRegisterAllocation( mNextLocation );
#endif
            return mNextLocation++;
        }

        // Release a location for a temporary register, must be the last location acquired
        void ReleaseTmpRegister( RegSlot tmpReg )
        {
            // make sure the location released is valid
            Assert( tmpReg != Js::Constants::NoRegister );

            // Put this reg back on top of the temp stack (if it's a temp).
            if( this->IsTmpReg( tmpReg ) )
            {
                Assert( tmpReg == this->mNextLocation - 1 );
#if DBG_DUMP
                PrintTmpRegisterDeAllocation( mNextLocation - 1 );
#endif
                this->mNextLocation--;
            }
        }

        // Checks if the register is a temporary register
        bool IsTmpReg( RegSlot tmpReg )
        {
            Assert( this->mFirstTmpReg != Js::Constants::NoRegister );
            return !IsConstReg( tmpReg ) && tmpReg >= mFirstTmpReg;
        }

        // Checks if the register is a const register
        bool IsConstReg( RegSlot reg )
        {
            // a register is const if it is between the first register and the end of consts
            return reg < mNbConst && reg != 0;
        }

        // Checks if the register is a variable register
        bool IsVarReg( RegSlot reg )
        {
            // a register is a var if it is between the last const and the end
            // equivalent to  reg>=mNbConst && reg<mRegisterCount
            // forcing unsigned, if reg < mNbConst then reg-mNbConst = 0xFFFFF..
            return (uint32_t)( reg - mNbConst ) < (uint32_t)( mRegisterCount - mNbConst );
        }

        // Releases a location if its a temporary, safe to call with any expression
        void ReleaseLocation( const EmitInfoBase *pnode )
        {
            // Release the temp assigned to this expression so it can be re-used.
            if( pnode && pnode->location != Js::Constants::NoRegister )
            {
                this->ReleaseTmpRegister( pnode->location );
            }
        }

        // Checks if the location points to a temporary register
        bool IsTmpLocation( const EmitInfoBase* pnode )
        {
            if( pnode && pnode->location != Js::Constants::NoRegister )
            {
                return IsTmpReg( pnode->location );
            }
            return false;
        }

        // Checks if the location points to a constant register
        bool IsConstLocation( const EmitInfoBase* pnode )
        {
            if( pnode && pnode->location != Js::Constants::NoRegister )
            {
                return IsConstReg( pnode->location );
            }
            return false;
        }

        // Checks if the location points to a variable register
        bool IsVarLocation( const EmitInfoBase* pnode )
        {
            if( pnode && pnode->location != Js::Constants::NoRegister )
            {
                return IsVarReg( pnode->location );
            }
            return false;
        }

        // Checks if the location is valid ( within bounds of already allocated registers )
        bool IsValidLocation( const EmitInfoBase* pnode )
        {
            if( pnode && pnode->location != Js::Constants::NoRegister )
            {
                return pnode->location < mRegisterCount;
            }
            return false;
        }

#if DBG_DUMP
        // Used for debugging
        enum Types
        {
            NONE,
            INT32,
            INT64,
            FLOAT32,
            FLOAT64,
            SIMD
        };
        Types mType;
        void PrintTmpRegisterAllocation(RegSlot loc);
        void PrintTmpRegisterDeAllocation(RegSlot loc);
#endif
    };
};

#endif
