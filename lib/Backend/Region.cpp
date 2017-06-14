//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

Region *
Region::New(RegionType type, Region * parent, Func * func)
{
    Region * region = JitAnew(func->m_alloc, Region);

    region->type = type;
    region->parent = parent;
    region->bailoutReturnThunkLabel = IR::LabelInstr::New(Js::OpCode::Label, func, true);
    if (type == RegionTypeTry)
    {
        region->selfOrFirstTryAncestor = region;
    }
    return region;
}

void
Region::AllocateEHBailoutData(Func * func, IR::Instr * tryInstr)
{
    if (this->GetType() == RegionTypeRoot)
    {
        this->ehBailoutData = NativeCodeDataNew(func->GetNativeCodeDataAllocator(), Js::EHBailoutData, -1 /*nestingDepth*/, 0 /*catchOffset*/, 0 /*finallyOffset*/, Js::HandlerType::HT_None, nullptr /*parent*/);
    }
    else
    {
        this->ehBailoutData = NativeCodeDataNew(func->GetNativeCodeDataAllocator(), Js::EHBailoutData, this->GetParent()->ehBailoutData->nestingDepth + 1, 0, 0, Js::HandlerType::HT_None, this->GetParent()->ehBailoutData);
        if (this->GetType() == RegionTypeTry)
        {
            Assert(tryInstr);
            if (tryInstr->m_opcode == Js::OpCode::TryCatch)
            {
                this->ehBailoutData->catchOffset = tryInstr->AsBranchInstr()->GetTarget()->GetByteCodeOffset(); // ByteCode offset of the Catch
            }
            else if (tryInstr->m_opcode == Js::OpCode::TryFinally)
            {
                this->ehBailoutData->finallyOffset = tryInstr->AsBranchInstr()->GetTarget()->GetByteCodeOffset(); // ByteCode offset of the Finally
            }
        }
        else if (this->GetType() == RegionTypeCatch)
        {
            this->ehBailoutData->ht = Js::HandlerType::HT_Catch;
        }
        else
        {
            this->ehBailoutData->ht = Js::HandlerType::HT_Finally;
        }
    }
}

Region *
Region::GetSelfOrFirstTryAncestor()
{
    if (!this->selfOrFirstTryAncestor)
    {
        Region* region = this;
        while (region && region->GetType() != RegionTypeTry)
        {
            region = region->GetParent();
        }
        this->selfOrFirstTryAncestor = region;
    }
    return this->selfOrFirstTryAncestor;
}

// Return the first ancestor of the region's parent which is not a non exception finally
// Skip all non exception finally regions in the region tree
// Return the parent of the first non-non-exception finally region
Region *
Region::GetFirstAncestorOfNonExceptingFinallyParent()
{
    Region * ancestor = this;
    while (ancestor && ancestor->IsNonExceptingFinally())
    {
        ancestor = ancestor->GetParent();
    }
    // ancestor is the first ancestor which is not a non exception finally
    Assert(ancestor && !ancestor->IsNonExceptingFinally());
    // If the ancestor's parent is a non exception finally, recurse
    if (ancestor && ancestor->GetType() != RegionTypeRoot && ancestor->GetParent()->IsNonExceptingFinally())
    {
        return ancestor->GetParent()->GetFirstAncestorOfNonExceptingFinally();
    }

    Assert(ancestor);
    // Null check added to avoid prefast warning only
    return ancestor ? (ancestor->GetType() == RegionTypeRoot ? ancestor : ancestor->GetParent()) : nullptr;
}

// Return first ancestor which is not a non exception finally
Region *
Region::GetFirstAncestorOfNonExceptingFinally()
{
    Region *ancestor = this->GetParent();
    while (ancestor->IsNonExceptingFinally())
    {
        ancestor = ancestor->GetParent();
    }
    return ancestor;
}

