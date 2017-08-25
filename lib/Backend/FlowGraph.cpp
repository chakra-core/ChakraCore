//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

FlowGraph *
FlowGraph::New(Func * func, JitArenaAllocator * alloc)
{
    FlowGraph * graph;

    graph = JitAnew(alloc, FlowGraph, func, alloc);

    return graph;
}


// When there is an early exit within an EH region,
// Leave instructions are inserted in the bytecode to jump up the region tree one by one
// We delete this Leave chain of instructions, and add an edge to Finally
IR::LabelInstr * FlowGraph::DeleteLeaveChainBlocks(IR::BranchInstr *leaveInstr, IR::Instr * &instrPrev)
{
    // Cleanup Rest of the Leave chain
    IR::LabelInstr * leaveTarget = leaveInstr->GetTarget();
    Assert(leaveTarget->GetNextRealInstr()->IsBranchInstr());
    IR::BranchInstr *leaveChain = leaveTarget->GetNextRealInstr()->AsBranchInstr();
    IR::LabelInstr * curLabel = leaveTarget->AsLabelInstr();

    while (leaveChain->m_opcode != Js::OpCode::Br)
    {
        Assert(leaveChain->m_opcode == Js::OpCode::Leave || leaveChain->m_opcode == Js::OpCode::BrOnException);
        IR::LabelInstr * nextLabel = leaveChain->m_next->AsLabelInstr();
        leaveChain = nextLabel->m_next->AsBranchInstr();
        BasicBlock *curBlock = curLabel->GetBasicBlock();
        this->RemoveBlock(curBlock);
        curLabel = nextLabel;
    }

    instrPrev = leaveChain->m_next;
    IR::LabelInstr * exitLabel = leaveChain->GetTarget();
    BasicBlock * curBlock = curLabel->GetBasicBlock();
    this->RemoveBlock(curBlock);
    return exitLabel;
}

bool FlowGraph::Dominates(Region *region1, Region *region2)
{
    Assert(region1);
    Assert(region2);
    Region *startR1 = region1;
    Region *startR2 = region2;
    int region1Depth = 0;
    while (startR1 != nullptr)
    {
        region1Depth++;
        startR1 = startR1->GetParent();
    }
    int region2Depth = 0;
    while (startR2 != nullptr)
    {
        region2Depth++;
        startR2 = startR2->GetParent();
    }
    return region1Depth > region2Depth;
}

bool FlowGraph::DoesExitLabelDominate(IR::BranchInstr *leaveInstr)
{
    IR::LabelInstr * leaveTarget = leaveInstr->GetTarget();
    Assert(leaveTarget->GetNextRealInstr()->IsBranchInstr());
    IR::BranchInstr *leaveChain = leaveTarget->GetNextRealInstr()->AsBranchInstr();

    while (leaveChain->m_opcode != Js::OpCode::Br)
    {
        Assert(leaveChain->m_opcode == Js::OpCode::Leave || leaveChain->m_opcode == Js::OpCode::BrOnException);
        IR::LabelInstr * nextLabel = leaveChain->m_next->AsLabelInstr();
        leaveChain = nextLabel->m_next->AsBranchInstr();
    }
    IR::LabelInstr * exitLabel = leaveChain->GetTarget();
    return Dominates(exitLabel->GetRegion(), finallyLabelStack->Top()->GetRegion());
}

bool FlowGraph::CheckIfEarlyExitAndAddEdgeToFinally(IR::BranchInstr *leaveInstr, Region *currentRegion, Region *branchTargetRegion, IR::Instr * &instrPrev, IR::LabelInstr * &exitLabel)
{
    if (finallyLabelStack->Empty())
    {
        return false;
    }

    if (currentRegion->GetType() == RegionTypeTry)
    {
        if (currentRegion->GetMatchingCatchRegion() == nullptr)
        {
            // try of try-finally
            bool isEarly =
                (branchTargetRegion != currentRegion->GetMatchingFinallyRegion(false) &&
                    branchTargetRegion != currentRegion->GetMatchingFinallyRegion(true));
            if (!isEarly) return false;

            if (DoesExitLabelDominate(leaveInstr)) return false;

            // Cleanup Rest of the Leave chain
            exitLabel = DeleteLeaveChainBlocks(leaveInstr, instrPrev);
            return true;
        }
        // try of try-catch

        IR::BranchInstr *leaveChain = leaveInstr;
        IR::LabelInstr * leaveTarget = leaveChain->GetTarget();
        IR::Instr *target = leaveTarget->GetNextRealInstr();
        if (target->m_opcode == Js::OpCode::Br)
        {
            IR::BranchInstr *tryExit = target->AsBranchInstr();
            instrPrev = tryExit;
            return false;
        }

        if (DoesExitLabelDominate(leaveInstr)) return false;

        // Cleanup Rest of the Leave chain
        exitLabel = DeleteLeaveChainBlocks(leaveInstr, instrPrev);
        return true;
    }
    if (currentRegion->GetType() == RegionTypeCatch)
    {
        // We don't care for early exits in catch blocks, because we bailout anyway
        return false;
    }
    Assert(currentRegion->GetType() == RegionTypeFinally);
    // All Leave's inside Finally region are early exits
    // Since we execute non-excepting Finallys in JIT now, we should convert Leave to Br
    if (DoesExitLabelDominate(leaveInstr)) return false;
    exitLabel = DeleteLeaveChainBlocks(leaveInstr, instrPrev);
    return true;
}

///----------------------------------------------------------------------------
///
/// FlowGraph::Build
///
/// Construct flow graph and loop structures for the current state of the function.
///
///----------------------------------------------------------------------------
void
FlowGraph::Build(void)
{
    Func * func = this->func;
    BEGIN_CODEGEN_PHASE(func, Js::FGPeepsPhase);
    this->RunPeeps();
    END_CODEGEN_PHASE(func, Js::FGPeepsPhase);

    // We don't optimize fully with SimpleJit. But, when JIT loop body is enabled, we do support
    // bailing out from a simple jitted function to do a full jit of a loop body in the function
    // (BailOnSimpleJitToFullJitLoopBody). For that purpose, we need the flow from try to handler.
    if (this->func->HasTry() && (this->func->DoOptimizeTry() ||
            (this->func->IsSimpleJit() && this->func->GetJITFunctionBody()->DoJITLoopBody())))
    {
        this->catchLabelStack = JitAnew(this->alloc, SList<IR::LabelInstr*>, this->alloc);
    }
    if (this->func->HasFinally() && (this->func->DoOptimizeTry() ||
        (this->func->IsSimpleJit() && this->func->GetJITFunctionBody()->DoJITLoopBody())))
    {
        this->finallyLabelStack = JitAnew(this->alloc, SList<IR::LabelInstr*>, this->alloc);
        this->regToFinallyEndMap = JitAnew(this->alloc, RegionToFinallyEndMapType, this->alloc, 0);
    }

    IR::Instr * currLastInstr = nullptr;
    BasicBlock * currBlock = nullptr;
    BasicBlock * nextBlock = nullptr;
    bool hasCall = false;

    FOREACH_INSTR_IN_FUNC_BACKWARD_EDITING(instr, instrPrev, func)
    {
        if (currLastInstr == nullptr || instr->EndsBasicBlock())
        {
            // Start working on a new block.
            // If we're currently processing a block, then wrap it up before beginning a new one.
            if (currLastInstr != nullptr)
            {
                nextBlock = currBlock;
                currBlock = this->AddBlock(instr->m_next, currLastInstr, nextBlock);
                currBlock->hasCall = hasCall;
                hasCall = false;
            }

            currLastInstr = instr;
        }

        if (instr->StartsBasicBlock())
        {
            // Insert a BrOnException after the loop top if we are in a try-catch/try-finally. This is required to
            // model flow from the loop to the catch/finally block for loops that don't have a break condition.
            if (instr->IsLabelInstr() && instr->AsLabelInstr()->m_isLoopTop && instr->m_next->m_opcode != Js::OpCode::BrOnException)
            {
                IR::BranchInstr * brOnException = nullptr;
                IR::Instr *instrNext = instr->m_next;
                if (this->catchLabelStack && !this->catchLabelStack->Empty())
                {
                    brOnException = IR::BranchInstr::New(Js::OpCode::BrOnException, this->catchLabelStack->Top(), instr->m_func);
                    instr->InsertAfter(brOnException);
                }
                if (this->finallyLabelStack && !this->finallyLabelStack->Empty())
                {
                    brOnException = IR::BranchInstr::New(Js::OpCode::BrOnException, this->finallyLabelStack->Top(), instr->m_func);
                    instr->InsertAfter(brOnException);
                }
                if (brOnException)
                {
                    instrPrev = instrNext->m_prev;
                    continue;
                }
            }

            // Wrap up the current block and get ready to process a new one.
            nextBlock = currBlock;
            currBlock = this->AddBlock(instr, currLastInstr, nextBlock);
            currBlock->hasCall = hasCall;
            hasCall = false;
            currLastInstr = nullptr;
        }

        switch (instr->m_opcode)
        {
        case Js::OpCode::Catch:
            Assert(instr->m_prev->IsLabelInstr());
            if (this->catchLabelStack)
            {
                this->catchLabelStack->Push(instr->m_prev->AsLabelInstr());
            }
            break;

        case Js::OpCode::Finally:
        {
            if (!this->finallyLabelStack)
            {
                break;
            }

            //To enable globopt on functions with try finallys we transform the flowgraph as below :
            //          TryFinally L1
            //          <try code>
            //          Leave L2
            //  L2 :    Br L3
            //  L1 :    <finally code>
            //          LeaveNull
            //  L3 :    <code after try finally>
            //
            //to:
            //
            //          TryFinally L1
            //          <try code>
            //          BrOnException L1
            //          Leave L1'
            //  L1 :    BailOnException
            //  L2 :    Finally
            //          <finally code>
            //          LeaveNull
            //  L3:     <code after try finally>
            //We generate 2 flow edges from the try - an exception path and a non exception path.
            //This transformation enables us to optimize on the non-excepting finally path

            Assert(instr->m_prev->IsLabelInstr());

            IR::LabelInstr * finallyLabel = instr->m_prev->AsLabelInstr();

            // Find leave label

            Assert(finallyLabel->m_prev->m_opcode == Js::OpCode::Br && finallyLabel->m_prev->m_prev->m_opcode == Js::OpCode::Label);

            IR::Instr * insertPoint = finallyLabel->m_prev;
            IR::LabelInstr * leaveTarget = finallyLabel->m_prev->m_prev->AsLabelInstr();

            leaveTarget->Unlink();
            finallyLabel->InsertBefore(leaveTarget);
            finallyLabel->Unlink();
            insertPoint->InsertBefore(finallyLabel);

            // Bailout from the opcode following Finally
            IR::Instr * bailOnException = IR::BailOutInstr::New(Js::OpCode::BailOnException, IR::BailOutOnException, instr->m_next, instr->m_func);
            insertPoint->InsertBefore(bailOnException);
            insertPoint->Remove();

            this->finallyLabelStack->Push(finallyLabel);

            Assert(leaveTarget->labelRefs.HasOne());
            IR::BranchInstr * brOnException = IR::BranchInstr::New(Js::OpCode::BrOnException, finallyLabel, instr->m_func);
            leaveTarget->labelRefs.Head()->InsertBefore(brOnException);

            instrPrev = instr->m_prev;
        }
        break;

        case Js::OpCode::TryCatch:
            if (this->catchLabelStack)
            {
                this->catchLabelStack->Pop();
            }
            break;

        case Js::OpCode::TryFinally:
            if (this->finallyLabelStack)
            {
                this->finallyLabelStack->Pop();
            }
            break;
        case Js::OpCode::CloneBlockScope:
        case Js::OpCode::CloneInnerScopeSlots:
            // It would be nice to do this in IRBuilder, but doing so gives us
            // trouble when doing the DoSlotArrayCheck since it assume single def
            // of the sym to do its check properly. So instead we assign the dst
            // here in FlowGraph.
            instr->SetDst(instr->GetSrc1());
            break;
        }

        if (OpCodeAttr::UseAllFields(instr->m_opcode))
        {
            // UseAllFields opcode are call instruction or opcode that would call.
            hasCall = true;

            if (OpCodeAttr::CallInstr(instr->m_opcode))
            {
                if (!instr->isCallInstrProtectedByNoProfileBailout)
                {
                    instr->m_func->SetHasCallsOnSelfAndParents();
                }

                // For ARM & X64 because of their register calling convention
                // the ArgOuts need to be moved next to the call.
#if defined(_M_ARM) || defined(_M_X64)

                IR::Instr* argInsertInstr = instr;
                instr->IterateArgInstrs([&](IR::Instr* argInstr)
                {
                    if (argInstr->m_opcode != Js::OpCode::LdSpreadIndices &&
                        argInstr->m_opcode != Js::OpCode::ArgOut_A_Dynamic &&
                        argInstr->m_opcode != Js::OpCode::ArgOut_A_FromStackArgs &&
                        argInstr->m_opcode != Js::OpCode::ArgOut_A_SpreadArg)
                    {
                        // don't have bailout in asm.js so we don't need BytecodeArgOutCapture
                        if (!argInstr->m_func->GetJITFunctionBody()->IsAsmJsMode())
                        {
                            // Need to always generate byte code arg out capture,
                            // because bailout can't restore from the arg out as it is
                            // replaced by new sym for register calling convention in lower
                            argInstr->GenerateBytecodeArgOutCapture();
                        }
                        // Check if the instruction is already next
                        if (argInstr != argInsertInstr->m_prev)
                        {
                            // It is not, move it.
                            argInstr->Move(argInsertInstr);
                        }
                        argInsertInstr = argInstr;
                    }
                    return false;
                });
#endif
            }
        }
    }
    NEXT_INSTR_IN_FUNC_BACKWARD_EDITING;
    this->func->isFlowGraphValid = true;
    Assert(!this->catchLabelStack || this->catchLabelStack->Empty());
    Assert(!this->finallyLabelStack || this->finallyLabelStack->Empty());

    // We've been walking backward so that edge lists would be in the right order. Now walk the blocks
    // forward to number the blocks in lexical order.
    unsigned int blockNum = 0;
    FOREACH_BLOCK(block, this)
    {
        block->SetBlockNum(blockNum++);
    }NEXT_BLOCK;
    AssertMsg(blockNum == this->blockCount, "Block count is out of whack");

    this->RemoveUnreachableBlocks();

    // Regions need to be assigned before Globopt because:
    //     1. FullJit: The Backward Pass will set the write-through symbols on the regions and the forward pass will
    //        use this information to insert ToVars for those symbols. Also, for a symbol determined as write-through
    //        in the try region to be restored correctly by the bailout, it should not be removed from the
    //        byteCodeUpwardExposedUsed upon a def in the try region (the def might be preempted by an exception).
    //
    //     2. SimpleJit: Same case of correct restoration as above applies in SimpleJit too. However, the only bailout
    //        we have in Simple Jitted code right now is BailOnSimpleJitToFullJitLoopBody, installed in IRBuilder. So,
    //        for now, we can just check if the func has a bailout to assign regions pre globopt while running SimpleJit.

    bool assignRegionsBeforeGlobopt = this->func->HasTry() && (this->func->DoOptimizeTry() ||
        (this->func->IsSimpleJit() && this->func->hasBailout));

    blockNum = 0;
    FOREACH_BLOCK_ALL(block, this)
    {
        block->SetBlockNum(blockNum++);
        if (assignRegionsBeforeGlobopt)
        {
            if (block->isDeleted && !block->isDead)
            {
                continue;
            }
            this->UpdateRegionForBlock(block);
        }
    } NEXT_BLOCK_ALL;
    AssertMsg(blockNum == this->blockCount, "Block count is out of whack");

#if DBG_DUMP
    if (PHASE_DUMP(Js::FGBuildPhase, this->GetFunc()))
    {
        if (assignRegionsBeforeGlobopt)
        {
            Output::Print(_u("Before adding early exit edges\n"));
            FOREACH_BLOCK_ALL(block, this)
            {
                block->DumpHeader(true);
                Region *region = block->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (region)
                {
                    const char16 * regMap[] = { _u("RegionTypeInvalid"),
                        _u("RegionTypeRoot"),
                        _u("RegionTypeTry"),
                        _u("RegionTypeCatch"),
                        _u("RegionTypeFinally") };
                    Output::Print(_u("Region %p RegionParent %p RegionType %s\n"), region, region->GetParent(), regMap[region->GetType()]);
                }
            } NEXT_BLOCK_ALL;
            this->func->Dump();
        }
    }
#endif

    if (this->finallyLabelStack)
    {
        Assert(this->finallyLabelStack->Empty());

        // Add s0 definition at the beginning of the function
        // We need this because - s0 symbol can get added to bytcodeUpwardExposed use when there are early returns,
        // And globopt will complain that s0 is uninitialized, because we define it to undefined only at the end of the function
        const auto addrOpnd = IR::AddrOpnd::New(this->func->GetScriptContextInfo()->GetUndefinedAddr(), IR::AddrOpndKindDynamicVar, this->func, true);
        addrOpnd->SetValueType(ValueType::Undefined);
        IR::RegOpnd *regOpnd = IR::RegOpnd::New(this->func->m_symTable->FindStackSym(0), TyVar, this->func);
        IR::Instr *ldRet = IR::Instr::New(Js::OpCode::Ld_A, regOpnd, addrOpnd, this->func);
        this->func->m_headInstr->GetNextRealInstr()->InsertBefore(ldRet);

        IR::LabelInstr * currentLabel = nullptr;
        // look for early exits from a try, and insert bailout
        FOREACH_INSTR_IN_FUNC_EDITING(instr, instrNext, func)
        {
            if (instr->IsLabelInstr())
            {
                currentLabel = instr->AsLabelInstr();
            }
            else if (instr->m_opcode == Js::OpCode::TryFinally)
            {
                Assert(instr->AsBranchInstr()->GetTarget()->GetRegion()->GetType() == RegionTypeFinally);
                this->finallyLabelStack->Push(instr->AsBranchInstr()->GetTarget());
            }
            else if (instr->m_opcode == Js::OpCode::Leave)
            {
                IR::LabelInstr *branchTarget = instr->AsBranchInstr()->GetTarget();
                IR::LabelInstr *exitLabel = nullptr;
                // An early exit (break, continue, return) from an EH region appears as Leave opcode
                // When there is an early exit within a try finally, we have to execute finally code
                // Currently we bailout on early exits
                // For all such edges add edge from eh region -> finally and finally -> earlyexit
                Assert(currentLabel != nullptr);
                if (currentLabel && CheckIfEarlyExitAndAddEdgeToFinally(instr->AsBranchInstr(), currentLabel->GetRegion(), branchTarget->GetRegion(), instrNext, exitLabel))
                {
                    Assert(exitLabel);
                    IR::Instr * bailOnEarlyExit = IR::BailOutInstr::New(Js::OpCode::BailOnEarlyExit, IR::BailOutOnEarlyExit, instr, instr->m_func); 
                    instr->InsertBefore(bailOnEarlyExit);
                    IR::LabelInstr *exceptFinallyLabel = this->finallyLabelStack->Top();
                    IR::LabelInstr *nonExceptFinallyLabel = exceptFinallyLabel->m_next->m_next->AsLabelInstr();

                    // It is possible for the finally region to have a non terminating loop, in which case the end of finally is eliminated
                    // We can skip adding edge from finally to early exit in this case
                    IR::Instr * leaveToFinally = IR::BranchInstr::New(Js::OpCode::Leave, exceptFinallyLabel, this->func);
                    instr->InsertBefore(leaveToFinally);
                    instr->Remove();
                    this->AddEdge(currentLabel->GetBasicBlock(), exceptFinallyLabel->GetBasicBlock());

                    if (this->regToFinallyEndMap->ContainsKey(nonExceptFinallyLabel->GetRegion()))
                    {
                        BasicBlock * finallyEndBlock = this->regToFinallyEndMap->Item(nonExceptFinallyLabel->GetRegion());
                        Assert(finallyEndBlock);
                        Assert(finallyEndBlock->GetFirstInstr()->AsLabelInstr()->GetRegion() == nonExceptFinallyLabel->GetRegion());
                        InsertEdgeFromFinallyToEarlyExit(finallyEndBlock, exitLabel);
                    }
                }
                else if (currentLabel && currentLabel->GetRegion()->GetType() == RegionTypeFinally)
                {
                    Assert(currentLabel->GetRegion()->GetMatchingTryRegion()->GetMatchingFinallyRegion(false) == currentLabel->GetRegion());
                    // Convert Leave to Br because we execute non-excepting Finally in native code
                    instr->m_opcode = Js::OpCode::Br;
#if DBG
                    instr->AsBranchInstr()->m_leaveConvToBr = true;
#endif
                }
            }
            else if (instr->m_opcode == Js::OpCode::Finally)
            {
                this->finallyLabelStack->Pop();
            }
        }
        NEXT_INSTR_IN_FUNC_EDITING;

        this->RemoveUnreachableBlocks();

        blockNum = 0;
        FOREACH_BLOCK_ALL(block, this)
        {
            block->SetBlockNum(blockNum++);
        } NEXT_BLOCK_ALL;
    }

    this->FindLoops();

#if DBG_DUMP
    if (PHASE_DUMP(Js::FGBuildPhase, this->GetFunc()))
    {
        if (assignRegionsBeforeGlobopt)
        {
            Output::Print(_u("After adding early exit edges/Before CanonicalizeLoops\n"));
            FOREACH_BLOCK_ALL(block, this)
            {
                block->DumpHeader(true);
                Region *region = block->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (region)
                {
                    const char16 * regMap[] = { _u("RegionTypeInvalid"),
                        _u("RegionTypeRoot"),
                        _u("RegionTypeTry"),
                        _u("RegionTypeCatch"),
                        _u("RegionTypeFinally") };
                    Output::Print(_u("Region %p RegionParent %p RegionType %s\n"), region, region->GetParent(), regMap[region->GetType()]);
                }
            } NEXT_BLOCK_ALL;
            this->func->Dump();
        }
    }
#endif

    bool breakBlocksRelocated = this->CanonicalizeLoops();

    blockNum = 0;
    FOREACH_BLOCK_ALL(block, this)
    {
        block->SetBlockNum(blockNum++);
    } NEXT_BLOCK_ALL;

#if DBG
    FOREACH_BLOCK_ALL(block, this)
    {
        if (assignRegionsBeforeGlobopt)
        {
            if (block->GetFirstInstr()->AsLabelInstr()->GetRegion())
            {
                Assert(block->GetFirstInstr()->AsLabelInstr()->GetRegion()->ehBailoutData);
            }
        }
    } NEXT_BLOCK_ALL;
#endif

#if DBG_DUMP
    if (PHASE_DUMP(Js::FGBuildPhase, this->GetFunc()))
    {
        if (assignRegionsBeforeGlobopt)
        {
            Output::Print(_u("After CanonicalizeLoops\n"));
            FOREACH_BLOCK_ALL(block, this)
            {
                block->DumpHeader(true);
                Region *region = block->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (region)
                {
                    const char16 * regMap[] = { _u("RegionTypeInvalid"),
                    _u("RegionTypeRoot"),
                    _u("RegionTypeTry"),
                    _u("RegionTypeCatch"),
                    _u("RegionTypeFinally") };
                    Output::Print(_u("Region %p RegionParent %p RegionType %s\n"), region, region->GetParent(), regMap[region->GetType()]);
                }
            } NEXT_BLOCK_ALL;
            this->func->Dump();
        }
    }
#endif

    if (breakBlocksRelocated)
    {
        // Sort loop lists only if there is break block removal.
        SortLoopLists();
    }

#if DBG
    this->VerifyLoopGraph();
#endif

#if DBG_DUMP
    this->Dump(false, nullptr);
#endif
}

void FlowGraph::InsertEdgeFromFinallyToEarlyExit(BasicBlock *finallyEndBlock, IR::LabelInstr *exitLabel)
{
    IR::Instr * lastInstr = finallyEndBlock->GetLastInstr();
    IR::LabelInstr * lastLabel = finallyEndBlock->GetFirstInstr()->AsLabelInstr();

    Assert(lastInstr->m_opcode == Js::OpCode::LeaveNull || lastInstr->m_opcode == Js::OpCode::Leave || lastInstr->m_opcode == Js::OpCode::BrOnException);
    // Add a new block, add BrOnException to earlyexit, assign region
    // Finally
    // ...
    // L1:
    // LeaveNull
    // to
    // Finally
    // ...
    // L1:
    // BrOnException earlyExitLabel
    // L1':
    // LeaveNull

    BasicBlock *nextBB = finallyEndBlock->GetNext();

    IR::LabelInstr *leaveLabel = IR::LabelInstr::New(Js::OpCode::Label, this->func);
    lastInstr->InsertBefore(leaveLabel);

    this->AddBlock(leaveLabel, lastInstr, nextBB, finallyEndBlock /*prevBlock*/);
    leaveLabel->SetRegion(lastLabel->GetRegion());

    this->AddEdge(finallyEndBlock, leaveLabel->GetBasicBlock());

    // If the Leave/LeaveNull at the end of finally was not preceeded by a Label, we have to create a new block with BrOnException to early exit
    if (!lastInstr->GetPrevRealInstrOrLabel()->IsLabelInstr())
    {
        IR::LabelInstr *brLabel = IR::LabelInstr::New(Js::OpCode::Label, this->func);
        leaveLabel->InsertBefore(brLabel);

        IR::BranchInstr *brToExit = IR::BranchInstr::New(Js::OpCode::BrOnException, exitLabel, this->func);
        brToExit->m_brFinallyToEarlyExit = true;
        brToExit->SetByteCodeOffset(lastInstr);
        leaveLabel->InsertBefore(brToExit);

        this->AddBlock(brLabel, brToExit, finallyEndBlock->GetNext(), finallyEndBlock /*prevBlock*/);
        brLabel->SetRegion(lastLabel->GetRegion());

        this->AddEdge(finallyEndBlock, brLabel->GetBasicBlock());
    }
    else
    {
        // If the Leave/LeaveNull at the end of finally was preceeded by a Label, we reuse the block inserting BrOnException to early exit in it
        IR::BranchInstr *brToExit = IR::BranchInstr::New(Js::OpCode::BrOnException, exitLabel, this->func);
        brToExit->SetByteCodeOffset(lastInstr);
        brToExit->m_brFinallyToEarlyExit = true;
        leaveLabel->InsertBefore(brToExit);
        this->AddEdge(finallyEndBlock, exitLabel->GetBasicBlock());
    }

    // In case of throw/non-terminating loop, there maybe no edge to the next block
    if (this->FindEdge(finallyEndBlock, nextBB))
    {
        finallyEndBlock->RemoveSucc(nextBB, this);
    }

    this->regToFinallyEndMap->Item(lastLabel->GetRegion(), leaveLabel->GetBasicBlock());
}

void
FlowGraph::SortLoopLists()
{
    // Sort the blocks in loopList
    for (Loop *loop = this->loopList; loop; loop = loop->next)
    {
        unsigned int lastBlockNumber = loop->GetHeadBlock()->GetBlockNum();
        // Insertion sort as the blockList is almost sorted in the loop.
        FOREACH_BLOCK_IN_LOOP_EDITING(block, loop, iter)
        {
            if (lastBlockNumber <= block->GetBlockNum())
            {
                lastBlockNumber = block->GetBlockNum();
            }
            else
            {
                iter.UnlinkCurrent();
                FOREACH_BLOCK_IN_LOOP_EDITING(insertBlock,loop,newIter)
                {
                    if (insertBlock->GetBlockNum() > block->GetBlockNum())
                    {
                        break;
                    }
                }NEXT_BLOCK_IN_LOOP_EDITING;
                newIter.InsertBefore(block);
            }
        }NEXT_BLOCK_IN_LOOP_EDITING;
    }
}

void
FlowGraph::RunPeeps()
{
    if (this->func->HasTry())
    {
        return;
    }

    if (PHASE_OFF(Js::FGPeepsPhase, this->func))
    {
        return;
    }

    IR::Instr * instrCm = nullptr;
    bool tryUnsignedCmpPeep = false;

    FOREACH_INSTR_IN_FUNC_EDITING(instr, instrNext, this->func)
    {
        switch(instr->m_opcode)
        {
        case Js::OpCode::Br:
        case Js::OpCode::BrEq_I4:
        case Js::OpCode::BrGe_I4:
        case Js::OpCode::BrGt_I4:
        case Js::OpCode::BrLt_I4:
        case Js::OpCode::BrLe_I4:
        case Js::OpCode::BrUnGe_I4:
        case Js::OpCode::BrUnGt_I4:
        case Js::OpCode::BrUnLt_I4:
        case Js::OpCode::BrUnLe_I4:
        case Js::OpCode::BrNeq_I4:
        case Js::OpCode::BrEq_A:
        case Js::OpCode::BrGe_A:
        case Js::OpCode::BrGt_A:
        case Js::OpCode::BrLt_A:
        case Js::OpCode::BrLe_A:
        case Js::OpCode::BrUnGe_A:
        case Js::OpCode::BrUnGt_A:
        case Js::OpCode::BrUnLt_A:
        case Js::OpCode::BrUnLe_A:
        case Js::OpCode::BrNotEq_A:
        case Js::OpCode::BrNotNeq_A:
        case Js::OpCode::BrSrNotEq_A:
        case Js::OpCode::BrSrNotNeq_A:
        case Js::OpCode::BrNotGe_A:
        case Js::OpCode::BrNotGt_A:
        case Js::OpCode::BrNotLt_A:
        case Js::OpCode::BrNotLe_A:
        case Js::OpCode::BrNeq_A:
        case Js::OpCode::BrNotNull_A:
        case Js::OpCode::BrNotAddr_A:
        case Js::OpCode::BrAddr_A:
        case Js::OpCode::BrSrEq_A:
        case Js::OpCode::BrSrNeq_A:
        case Js::OpCode::BrOnHasProperty:
        case Js::OpCode::BrOnNoProperty:
        case Js::OpCode::BrHasSideEffects:
        case Js::OpCode::BrNotHasSideEffects:
        case Js::OpCode::BrFncEqApply:
        case Js::OpCode::BrFncNeqApply:
        case Js::OpCode::BrOnEmpty:
        case Js::OpCode::BrOnNotEmpty:
        case Js::OpCode::BrFncCachedScopeEq:
        case Js::OpCode::BrFncCachedScopeNeq:
        case Js::OpCode::BrOnObject_A:
        case Js::OpCode::BrOnClassConstructor:
        case Js::OpCode::BrOnBaseConstructorKind:
            if (tryUnsignedCmpPeep)
            {
                this->UnsignedCmpPeep(instr);
            }
            instrNext = Peeps::PeepBranch(instr->AsBranchInstr());
            break;

        case Js::OpCode::MultiBr:
            // TODO: Run peeps on these as well...
            break;

        case Js::OpCode::BrTrue_I4:
        case Js::OpCode::BrFalse_I4:
        case Js::OpCode::BrTrue_A:
        case Js::OpCode::BrFalse_A:
            if (instrCm)
            {
                if (instrCm->GetDst()->IsInt32())
                {
                    Assert(instr->m_opcode == Js::OpCode::BrTrue_I4 || instr->m_opcode == Js::OpCode::BrFalse_I4);
                    instrNext = this->PeepTypedCm(instrCm);
                }
                else
                {
                    instrNext = this->PeepCm(instrCm);
                }
                instrCm = nullptr;

                if (instrNext == nullptr)
                {
                    // Set instrNext back to the current instr.
                    instrNext = instr;
                }
            }
            else
            {
                instrNext = Peeps::PeepBranch(instr->AsBranchInstr());
            }
            break;

        case Js::OpCode::CmEq_I4:
        case Js::OpCode::CmGe_I4:
        case Js::OpCode::CmGt_I4:
        case Js::OpCode::CmLt_I4:
        case Js::OpCode::CmLe_I4:
        case Js::OpCode::CmNeq_I4:
        case Js::OpCode::CmEq_A:
        case Js::OpCode::CmGe_A:
        case Js::OpCode::CmGt_A:
        case Js::OpCode::CmLt_A:
        case Js::OpCode::CmLe_A:
        case Js::OpCode::CmNeq_A:
        case Js::OpCode::CmSrEq_A:
        case Js::OpCode::CmSrNeq_A:
            if (tryUnsignedCmpPeep)
            {
                this->UnsignedCmpPeep(instr);
            }
        case Js::OpCode::CmUnGe_I4:
        case Js::OpCode::CmUnGt_I4:
        case Js::OpCode::CmUnLt_I4:
        case Js::OpCode::CmUnLe_I4:
        case Js::OpCode::CmUnGe_A:
        case Js::OpCode::CmUnGt_A:
        case Js::OpCode::CmUnLt_A:
        case Js::OpCode::CmUnLe_A:
            // There may be useless branches between the Cm instr and the branch that uses the result.
            // So save the last Cm instr seen, and trigger the peep on the next BrTrue/BrFalse.
            instrCm = instr;
            break;

        case Js::OpCode::Label:
            if (instr->AsLabelInstr()->IsUnreferenced())
            {
                instrNext = Peeps::PeepUnreachableLabel(instr->AsLabelInstr(), false);
            }
            break;

        case Js::OpCode::StatementBoundary:
            instr->ClearByteCodeOffset();
            instr->SetByteCodeOffset(instr->GetNextRealInstrOrLabel());
            break;

        case Js::OpCode::ShrU_I4:
        case Js::OpCode::ShrU_A:
            if (tryUnsignedCmpPeep)
            {
                break;
            }
            if (instr->GetDst()->AsRegOpnd()->m_sym->IsSingleDef()
                && instr->GetSrc2()->IsRegOpnd() && instr->GetSrc2()->AsRegOpnd()->m_sym->IsTaggableIntConst()
                && instr->GetSrc2()->AsRegOpnd()->m_sym->GetIntConstValue() == 0)
            {
                tryUnsignedCmpPeep = true;
            }
            break;
        default:
            Assert(!instr->IsBranchInstr());
        }
   } NEXT_INSTR_IN_FUNC_EDITING;
}

void
Loop::InsertLandingPad(FlowGraph *fg)
{
    BasicBlock *headBlock = this->GetHeadBlock();

    // Always create a landing pad.  This allows globopt to easily hoist instructions
    // and re-optimize the block if needed.
    BasicBlock *landingPad = BasicBlock::New(fg);
    this->landingPad = landingPad;
    IR::Instr * headInstr = headBlock->GetFirstInstr();
    IR::LabelInstr *landingPadLabel = IR::LabelInstr::New(Js::OpCode::Label, headInstr->m_func);
    landingPadLabel->SetByteCodeOffset(headInstr);
    headInstr->InsertBefore(landingPadLabel);

    landingPadLabel->SetBasicBlock(landingPad);
    landingPadLabel->SetRegion(headBlock->GetFirstInstr()->AsLabelInstr()->GetRegion());
    landingPadLabel->m_hasNonBranchRef = headBlock->GetFirstInstr()->AsLabelInstr()->m_hasNonBranchRef;
    landingPad->SetBlockNum(fg->blockCount++);
    landingPad->SetFirstInstr(landingPadLabel);
    landingPad->SetLastInstr(landingPadLabel);

    landingPad->prev = headBlock->prev;
    landingPad->prev->next = landingPad;
    landingPad->next = headBlock;
    headBlock->prev = landingPad;

    Loop *parentLoop = this->parent;
    landingPad->loop = parentLoop;

    // We need to add this block to the block list of the parent loops
    while (parentLoop)
    {
        // Find the head block in the block list of the parent loop
        FOREACH_BLOCK_IN_LOOP_EDITING(block, parentLoop, iter)
        {
            if (block == headBlock)
            {
                // Add the landing pad to the block list
                iter.InsertBefore(landingPad);
                break;
            }
        } NEXT_BLOCK_IN_LOOP_EDITING;

        parentLoop = parentLoop->parent;
    }

    // Fix predecessor flow edges
    FOREACH_PREDECESSOR_EDGE_EDITING(edge, headBlock, iter)
    {
        // Make sure it isn't a back-edge
        if (edge->GetPred()->loop != this && !this->IsDescendentOrSelf(edge->GetPred()->loop))
        {
            if (edge->GetPred()->GetLastInstr()->IsBranchInstr() && headBlock->GetFirstInstr()->IsLabelInstr())
            {
                IR::BranchInstr *branch = edge->GetPred()->GetLastInstr()->AsBranchInstr();
                branch->ReplaceTarget(headBlock->GetFirstInstr()->AsLabelInstr(), landingPadLabel);
            }
            headBlock->UnlinkPred(edge->GetPred(), false);
            landingPad->AddPred(edge, fg);
            edge->SetSucc(landingPad);
        }
    } NEXT_PREDECESSOR_EDGE_EDITING;

    fg->AddEdge(landingPad, headBlock);

    if (headBlock->GetFirstInstr()->AsLabelInstr()->GetRegion() && headBlock->GetFirstInstr()->AsLabelInstr()->GetRegion()->GetType() != RegionTypeRoot)
    {
        landingPadLabel->m_hasNonBranchRef = true;
    }
}

bool
Loop::RemoveBreakBlocks(FlowGraph *fg)
{
    bool breakBlockRelocated = false;
    if (PHASE_OFF(Js::RemoveBreakBlockPhase, fg->GetFunc()))
    {
        return false;
    }

    BasicBlock *loopTailBlock = nullptr;
    FOREACH_BLOCK_IN_LOOP(block, this)
    {
        loopTailBlock = block;
    }NEXT_BLOCK_IN_LOOP;

    AnalysisAssert(loopTailBlock);

    FOREACH_BLOCK_BACKWARD_IN_RANGE_EDITING(breakBlockEnd, loopTailBlock, this->GetHeadBlock(), blockPrev)
    {
        while (!this->IsDescendentOrSelf(breakBlockEnd->loop))
        {
            // Found at least one break block;
            breakBlockRelocated = true;

#if DBG
            breakBlockEnd->isBreakBlock = true;
#endif
            // Find the first block in this break block sequence.
            BasicBlock *breakBlockStart = breakBlockEnd;
            BasicBlock *breakBlockStartPrev = breakBlockEnd->GetPrev();

            // Walk back the blocks until we find a block which belongs to that block.
            // Note: We don't really care if there are break blocks corresponding to different loops. We move the blocks conservatively to the end of the loop.

            // Algorithm works on one loop at a time.
            while((breakBlockStartPrev->loop == breakBlockEnd->loop) || !this->IsDescendentOrSelf(breakBlockStartPrev->loop))
            {
                breakBlockStart = breakBlockStartPrev;
                breakBlockStartPrev = breakBlockStartPrev->GetPrev();
            }

#if DBG
            breakBlockStart->isBreakBlock = true; // Mark the first block as well.
#endif

            BasicBlock *exitLoopTail = loopTailBlock;
            // Move these break blocks to the tail of the loop.
            fg->MoveBlocksBefore(breakBlockStart, breakBlockEnd, exitLoopTail->next);

#if DBG_DUMP
            fg->Dump(true /*needs verbose flag*/, _u("\n After Each iteration of canonicalization \n"));
#endif
            // Again be conservative, there are edits to the loop graph. Start fresh for this loop.
            breakBlockEnd = loopTailBlock;
            blockPrev = breakBlockEnd->prev;
        }
    } NEXT_BLOCK_BACKWARD_IN_RANGE_EDITING;

    return breakBlockRelocated;
}

void
FlowGraph::MoveBlocksBefore(BasicBlock *blockStart, BasicBlock *blockEnd, BasicBlock *insertBlock)
{
    BasicBlock *srcPredBlock = blockStart->prev;
    BasicBlock *srcNextBlock = blockEnd->next;
    BasicBlock *dstPredBlock = insertBlock->prev;
    IR::Instr* dstPredBlockLastInstr = dstPredBlock->GetLastInstr();
    IR::Instr* blockEndLastInstr = blockEnd->GetLastInstr();

    // Fix block linkage
    srcPredBlock->next = srcNextBlock;
    srcNextBlock->prev = srcPredBlock;

    dstPredBlock->next = blockStart;
    insertBlock->prev = blockEnd;

    blockStart->prev = dstPredBlock;
    blockEnd->next = insertBlock;

    // Fix instruction linkage
    IR::Instr::MoveRangeAfter(blockStart->GetFirstInstr(), blockEndLastInstr, dstPredBlockLastInstr);

    // Fix instruction flow
    IR::Instr *srcLastInstr = srcPredBlock->GetLastInstr();
    if (srcLastInstr->IsBranchInstr() && srcLastInstr->AsBranchInstr()->HasFallThrough())
    {
        // There was a fallthrough in the break blocks original position.
        IR::BranchInstr *srcBranch = srcLastInstr->AsBranchInstr();
        IR::Instr *srcBranchNextInstr = srcBranch->GetNextRealInstrOrLabel();

        // Save the target and invert the branch.
        IR::LabelInstr *srcBranchTarget = srcBranch->GetTarget();
        srcPredBlock->InvertBranch(srcBranch);
        IR::LabelInstr *srcLabel = blockStart->GetFirstInstr()->AsLabelInstr();

        // Point the inverted branch to break block.
        srcBranch->SetTarget(srcLabel);

        if (srcBranchNextInstr != srcBranchTarget)
        {
            FlowEdge *srcEdge  = this->FindEdge(srcPredBlock, srcBranchTarget->GetBasicBlock());
            Assert(srcEdge);

            BasicBlock *compensationBlock = this->InsertCompensationCodeForBlockMove(srcEdge, true /*insert compensation block to loop list*/, false /*At source*/);
            Assert(compensationBlock);
        }
    }

    IR::Instr *dstLastInstr = dstPredBlockLastInstr;

    bool assignRegionsBeforeGlobopt = this->func->HasTry() && (this->func->DoOptimizeTry() ||
        (this->func->IsSimpleJit() && this->func->hasBailout));

    if (dstLastInstr->IsBranchInstr() && dstLastInstr->AsBranchInstr()->HasFallThrough())
    {
        //There is a fallthrough in the block after which break block is inserted.
        FlowEdge *dstEdge = this->FindEdge(dstPredBlock, blockEnd->GetNext());
        Assert(dstEdge);

        BasicBlock *compensationBlock = this->InsertCompensationCodeForBlockMove(dstEdge, true /*insert compensation block to loop list*/, true /*At sink*/);
        Assert(compensationBlock);
    }

    // We have to update region info for blocks whose predecessors changed
    if (assignRegionsBeforeGlobopt)
    {
        UpdateRegionForBlockFromEHPred(dstPredBlock, true);
        UpdateRegionForBlockFromEHPred(blockStart, true);
        UpdateRegionForBlockFromEHPred(srcNextBlock, true);
    }
}

FlowEdge *
FlowGraph::FindEdge(BasicBlock *predBlock, BasicBlock *succBlock)
{
        FlowEdge *srcEdge = nullptr;
        FOREACH_SUCCESSOR_EDGE(edge, predBlock)
        {
            if (edge->GetSucc() == succBlock)
            {
                srcEdge = edge;
                break;
            }
        } NEXT_SUCCESSOR_EDGE;

        return srcEdge;
}

void
BasicBlock::InvertBranch(IR::BranchInstr *branch)
{
    Assert(this->GetLastInstr() == branch);
    Assert(this->GetSuccList()->HasTwo());

    branch->Invert();
    this->GetSuccList()->Reverse();
}

bool
FlowGraph::CanonicalizeLoops()
{
    if (this->func->HasProfileInfo())
    {
        this->implicitCallFlags = this->func->GetReadOnlyProfileInfo()->GetImplicitCallFlags();
        for (Loop *loop = this->loopList; loop; loop = loop->next)
        {
            this->implicitCallFlags = (Js::ImplicitCallFlags)(this->implicitCallFlags | loop->GetImplicitCallFlags());
        }
    }

#if DBG_DUMP
    this->Dump(true, _u("\n Before canonicalizeLoops \n"));
#endif

    bool breakBlockRelocated = false;

    for (Loop *loop = this->loopList; loop; loop = loop->next)
    {
        loop->InsertLandingPad(this);
        if (!this->func->HasTry() || this->func->DoOptimizeTry())
        {
            bool relocated = loop->RemoveBreakBlocks(this);
            if (!breakBlockRelocated && relocated)
            {
                breakBlockRelocated  = true;
            }
        }
    }

#if DBG_DUMP
    this->Dump(true, _u("\n After canonicalizeLoops \n"));
#endif

    return breakBlockRelocated;
}

// Find the loops in this function, build the loop structure, and build a linked
// list of the basic blocks in this loop (including blocks of inner loops). The
// list preserves the reverse post-order of the blocks in the flowgraph block list.
void
FlowGraph::FindLoops()
{
    if (!this->hasLoop)
    {
        return;
    }

    Func * func = this->func;

    FOREACH_BLOCK_BACKWARD_IN_FUNC(block, func)
    {
        if (block->loop != nullptr)
        {
            // Block already visited
            continue;
        }
        FOREACH_SUCCESSOR_BLOCK(succ, block)
        {
            if (succ->isLoopHeader && succ->loop == nullptr)
            {
                // Found a loop back-edge
                BuildLoop(succ, block);
            }
        } NEXT_SUCCESSOR_BLOCK;
        if (block->isLoopHeader && block->loop == nullptr)
        {
            // We would have built a loop for it if it was a loop...
            block->isLoopHeader = false;
            block->GetFirstInstr()->AsLabelInstr()->m_isLoopTop = false;
        }
    } NEXT_BLOCK_BACKWARD_IN_FUNC;
}

void
FlowGraph::BuildLoop(BasicBlock *headBlock, BasicBlock *tailBlock, Loop *parentLoop)
{
    // This function is recursive, so when jitting in the foreground, probe the stack
    if(!func->IsBackgroundJIT())
    {
        PROBE_STACK(func->GetScriptContext(), Js::Constants::MinStackDefault);
    }

    if (tailBlock->number < headBlock->number)
    {
        // Not a loop.  We didn't see any back-edge.
        headBlock->isLoopHeader = false;
        headBlock->GetFirstInstr()->AsLabelInstr()->m_isLoopTop = false;
        return;
    }

    Assert(headBlock->isLoopHeader);
    Loop *loop = JitAnewZ(this->GetFunc()->m_alloc, Loop, this->GetFunc()->m_alloc, this->GetFunc());
    loop->next = this->loopList;
    this->loopList = loop;
    headBlock->loop = loop;
    loop->headBlock = headBlock;
    loop->int32SymsOnEntry = nullptr;
    loop->lossyInt32SymsOnEntry = nullptr;

    // If parentLoop is a parent of loop, it's headBlock better appear first.
    if (parentLoop && loop->headBlock->number > parentLoop->headBlock->number)
    {
        loop->parent = parentLoop;
        parentLoop->isLeaf = false;
    }
    loop->hasDeadStoreCollectionPass = false;
    loop->hasDeadStorePrepass = false;
    loop->memOpInfo = nullptr;
    loop->doMemOp = true;

    NoRecoverMemoryJitArenaAllocator tempAlloc(_u("BE-LoopBuilder"), this->func->m_alloc->GetPageAllocator(), Js::Throw::OutOfMemory);

    WalkLoopBlocks(tailBlock, loop, &tempAlloc);

    Assert(loop->GetHeadBlock() == headBlock);

    IR::LabelInstr * firstInstr = loop->GetLoopTopInstr();

    firstInstr->SetLoop(loop);

    if (firstInstr->IsProfiledLabelInstr())
    {
        loop->SetImplicitCallFlags(firstInstr->AsProfiledLabelInstr()->loopImplicitCallFlags);
        if (this->func->HasProfileInfo() && this->func->GetReadOnlyProfileInfo()->IsLoopImplicitCallInfoDisabled())
        {
            loop->SetImplicitCallFlags(this->func->GetReadOnlyProfileInfo()->GetImplicitCallFlags());
        }
        loop->SetLoopFlags(firstInstr->AsProfiledLabelInstr()->loopFlags);
    }
    else
    {
        // Didn't collect profile information, don't do optimizations
        loop->SetImplicitCallFlags(Js::ImplicitCall_All);
    }
}

Loop::MemCopyCandidate* Loop::MemOpCandidate::AsMemCopy()
{
    Assert(this->IsMemCopy());
    return (Loop::MemCopyCandidate*)this;
}

Loop::MemSetCandidate* Loop::MemOpCandidate::AsMemSet()
{
    Assert(this->IsMemSet());
    return (Loop::MemSetCandidate*)this;
}

void
Loop::EnsureMemOpVariablesInitialized()
{
    Assert(this->doMemOp);
    if (this->memOpInfo == nullptr)
    {
        JitArenaAllocator *allocator = this->GetFunc()->GetTopFunc()->m_fg->alloc;
        this->memOpInfo = JitAnewStruct(allocator, Loop::MemOpInfo);
        this->memOpInfo->inductionVariablesUsedAfterLoop = nullptr;
        this->memOpInfo->startIndexOpndCache[0] = nullptr;
        this->memOpInfo->startIndexOpndCache[1] = nullptr;
        this->memOpInfo->startIndexOpndCache[2] = nullptr;
        this->memOpInfo->startIndexOpndCache[3] = nullptr;
        this->memOpInfo->inductionVariableChangeInfoMap = JitAnew(allocator, Loop::InductionVariableChangeInfoMap, allocator);
        this->memOpInfo->inductionVariableOpndPerUnrollMap = JitAnew(allocator, Loop::InductionVariableOpndPerUnrollMap, allocator);
        this->memOpInfo->candidates = JitAnew(allocator, Loop::MemOpList, allocator);
    }
}

// Walk the basic blocks backwards until we find the loop header.
// Mark basic blocks in the loop by looking at the predecessors
// of blocks known to be in the loop.
// Recurse on inner loops.
void
FlowGraph::WalkLoopBlocks(BasicBlock *block, Loop *loop, JitArenaAllocator *tempAlloc)
{
    AnalysisAssert(loop);

    BVSparse<JitArenaAllocator> *loopBlocksBv = JitAnew(tempAlloc, BVSparse<JitArenaAllocator>, tempAlloc);
    BasicBlock *tailBlock = block;
    BasicBlock *lastBlock;
    loopBlocksBv->Set(block->GetBlockNum());

    this->AddBlockToLoop(block, loop);

    if (block == loop->headBlock)
    {
        // Single block loop, we're done
        return;
    }

    do
    {
        BOOL isInLoop = loopBlocksBv->Test(block->GetBlockNum());

        FOREACH_SUCCESSOR_BLOCK(succ, block)
        {
            if (succ->isLoopHeader)
            {
                // Found a loop back-edge
                if (loop->headBlock == succ)
                {
                    isInLoop = true;
                }
                else if (succ->loop == nullptr || succ->loop->headBlock != succ)
                {
                    // Recurse on inner loop
                    BuildLoop(succ, block, isInLoop ? loop : nullptr);
                }
            }
        } NEXT_SUCCESSOR_BLOCK;

        if (isInLoop)
        {
            // This block is in the loop.  All of it's predecessors should be contained in the loop as well.
            FOREACH_PREDECESSOR_BLOCK(pred, block)
            {
                // Fix up loop parent if it isn't set already.
                // If pred->loop != loop, we're looking at an inner loop, which was already visited.
                // If pred->loop->parent == nullptr, this is the first time we see this loop from an outer
                // loop, so this must be an immediate child.
                if (pred->loop && pred->loop != loop && loop->headBlock->number < pred->loop->headBlock->number
                    && (pred->loop->parent == nullptr || pred->loop->parent->headBlock->number < loop->headBlock->number))
                {
                    pred->loop->parent = loop;
                    loop->isLeaf = false;
                    if (pred->loop->hasCall)
                    {
                        loop->SetHasCall();
                    }
                    loop->SetImplicitCallFlags(pred->loop->GetImplicitCallFlags());
                }
                // Add pred to loop bit vector
                loopBlocksBv->Set(pred->GetBlockNum());
            } NEXT_PREDECESSOR_BLOCK;

            if (block->loop == nullptr || block->loop->IsDescendentOrSelf(loop))
            {
                block->loop = loop;
            }

            if (block != tailBlock)
            {
                this->AddBlockToLoop(block, loop);
            }
        }
        lastBlock = block;
        block = block->GetPrev();
    } while (lastBlock != loop->headBlock);
}

// Add block to this loop, and it's parent loops.
void
FlowGraph::AddBlockToLoop(BasicBlock *block, Loop *loop)
{
    loop->blockList.Prepend(block);
    if (block->hasCall)
    {
        loop->SetHasCall();
    }
}

///----------------------------------------------------------------------------
///
/// FlowGraph::AddBlock
///
/// Finish processing of a new block: hook up successor arcs, note loops, etc.
///
///----------------------------------------------------------------------------
BasicBlock *
FlowGraph::AddBlock(
    IR::Instr * firstInstr,
    IR::Instr * lastInstr,
    BasicBlock * nextBlock,
    BasicBlock *prevBlock)
{
    BasicBlock * block;
    IR::LabelInstr * labelInstr;

    if (firstInstr->IsLabelInstr())
    {
        labelInstr = firstInstr->AsLabelInstr();
    }
    else
    {
        labelInstr = IR::LabelInstr::New(Js::OpCode::Label, firstInstr->m_func);
        labelInstr->SetByteCodeOffset(firstInstr);
        if (firstInstr->IsEntryInstr())
        {
            firstInstr->InsertAfter(labelInstr);
        }
        else
        {
            firstInstr->InsertBefore(labelInstr);
        }
        firstInstr = labelInstr;
    }

    block = labelInstr->GetBasicBlock();
    if (block == nullptr)
    {
        block = BasicBlock::New(this);
        labelInstr->SetBasicBlock(block);
        // Remember last block in function to target successor of RETs.
        if (!this->tailBlock)
        {
            this->tailBlock = block;
        }
    }

    // Hook up the successor edges
    if (lastInstr->EndsBasicBlock())
    {
        BasicBlock * blockTarget = nullptr;

        if (lastInstr->IsBranchInstr())
        {
            // Hook up a successor edge to the branch target.
            IR::BranchInstr * branchInstr = lastInstr->AsBranchInstr();

            if(branchInstr->IsMultiBranch())
            {
                BasicBlock * blockMultiBrTarget;

                IR::MultiBranchInstr * multiBranchInstr = branchInstr->AsMultiBrInstr();

                multiBranchInstr->MapUniqueMultiBrLabels([&](IR::LabelInstr * labelInstr) -> void
                {
                    blockMultiBrTarget = SetBlockTargetAndLoopFlag(labelInstr);
                    this->AddEdge(block, blockMultiBrTarget);
                });
            }
            else
            {
                IR::LabelInstr * targetLabelInstr = branchInstr->GetTarget();
                blockTarget = SetBlockTargetAndLoopFlag(targetLabelInstr);
                if (branchInstr->IsConditional())
                {
                    IR::Instr *instrNext = branchInstr->GetNextRealInstrOrLabel();

                    if (instrNext->IsLabelInstr())
                    {
                        SetBlockTargetAndLoopFlag(instrNext->AsLabelInstr());
                    }
                }
            }
        }
        else if (lastInstr->m_opcode == Js::OpCode::Ret && block != this->tailBlock)
        {
            blockTarget = this->tailBlock;
        }

        if (blockTarget)
        {
            this->AddEdge(block, blockTarget);
        }
    }

    if (lastInstr->HasFallThrough())
    {
        // Add a branch to next instruction so that we don't have to update the flow graph
        // when the glob opt tries to insert instructions.
        // We don't run the globopt with try/catch, don't need to insert branch to next for fall through blocks.
        if (!this->func->HasTry() && !lastInstr->IsBranchInstr())
        {
            IR::BranchInstr * instr = IR::BranchInstr::New(Js::OpCode::Br,
                lastInstr->m_next->AsLabelInstr(), lastInstr->m_func);
            instr->SetByteCodeOffset(lastInstr->m_next);
            lastInstr->InsertAfter(instr);
            lastInstr = instr;
        }
        this->AddEdge(block, nextBlock);
    }

    block->SetBlockNum(this->blockCount++);
    block->SetFirstInstr(firstInstr);
    block->SetLastInstr(lastInstr);

    if (!prevBlock)
    {
        if (this->blockList)
        {
            this->blockList->prev = block;
        }
        block->next = this->blockList;
        this->blockList = block;
    }
    else
    {
        prevBlock->next = block;
        block->prev = prevBlock;
        block->next = nextBlock;
        nextBlock->prev = block;
    }
    return block;
}

BasicBlock *
FlowGraph::SetBlockTargetAndLoopFlag(IR::LabelInstr * labelInstr)
{
    BasicBlock * blockTarget = nullptr;
    blockTarget = labelInstr->GetBasicBlock();

    if (blockTarget == nullptr)
    {
        blockTarget = BasicBlock::New(this);
        labelInstr->SetBasicBlock(blockTarget);
    }
    if (labelInstr->m_isLoopTop)
    {
        blockTarget->isLoopHeader = true;
        this->hasLoop = true;
    }

    return blockTarget;
}

///----------------------------------------------------------------------------
///
/// FlowGraph::AddEdge
///
/// Add an edge connecting the two given blocks.
///
///----------------------------------------------------------------------------
FlowEdge *
FlowGraph::AddEdge(BasicBlock * blockPred, BasicBlock * blockSucc)
{
    FlowEdge * edge = FlowEdge::New(this);
    edge->SetPred(blockPred);
    edge->SetSucc(blockSucc);
    blockPred->AddSucc(edge, this);
    blockSucc->AddPred(edge, this);

    return edge;
}

///----------------------------------------------------------------------------
///
/// FlowGraph::Destroy
///
/// Remove all references to FG structures from the IR in preparation for freeing
/// the FG.
///
///----------------------------------------------------------------------------
void
FlowGraph::Destroy(void)
{
    BOOL fHasTry = this->func->HasTry();
    if (fHasTry)
    {
        // Do unreachable code removal up front to avoid problems
        // with unreachable back edges, etc.
        this->RemoveUnreachableBlocks();
    }

    FOREACH_BLOCK_ALL(block, this)
    {
        IR::Instr * firstInstr = block->GetFirstInstr();
        if (block->isDeleted && !block->isDead)
        {
            if (firstInstr->IsLabelInstr())
            {
                IR::LabelInstr * labelInstr = firstInstr->AsLabelInstr();
                labelInstr->UnlinkBasicBlock();
                // Removing the label for non try blocks as we have a deleted block which has the label instruction
                // still not removed; this prevents the assert for cases where the deleted blocks fall through to a helper block,
                // i.e. helper introduced by polymorphic inlining bailout.
                // Skipping Try blocks as we have dependency on blocks to get the last instr(see below in this function)
                if (!fHasTry)
                {
                    if (this->func->GetJITFunctionBody()->IsCoroutine())
                    {
                        // the label could be a yield resume label, in which case we also need to remove it from the YieldOffsetResumeLabels list
                        this->func->MapUntilYieldOffsetResumeLabels([this, &labelInstr](int i, const YieldOffsetResumeLabel& yorl)
                        {
                            if (labelInstr == yorl.Second())
                            {
                                labelInstr->m_hasNonBranchRef = false;
                                this->func->RemoveYieldOffsetResumeLabel(yorl);
                                return true;
                            }
                            return false;
                        });
                    }

                    Assert(labelInstr->IsUnreferenced());
                    labelInstr->Remove();
                }
            }
            continue;
        }

        if (block->isLoopHeader && !block->isDead)
        {
            // Mark the tail block of this loop (the last back-edge).  The register allocator
            // uses this to lexically find loops.
            BasicBlock *loopTail = nullptr;

            AssertMsg(firstInstr->IsLabelInstr() && firstInstr->AsLabelInstr()->m_isLoopTop,
                "Label not marked as loop top...");
            FOREACH_BLOCK_IN_LOOP(loopBlock, block->loop)
            {
                FOREACH_SUCCESSOR_BLOCK(succ, loopBlock)
                {
                    if (succ == block)
                    {
                        loopTail = loopBlock;
                        break;
                    }
                } NEXT_SUCCESSOR_BLOCK;
            } NEXT_BLOCK_IN_LOOP;

            if (loopTail)
            {
                AssertMsg(loopTail->GetLastInstr()->IsBranchInstr(), "LastInstr of loop should always be a branch no?");
                block->loop->SetLoopTopInstr(block->GetFirstInstr()->AsLabelInstr());
            }
            else
            {
                // This loop doesn't have a back-edge: that is, it is not a loop
                // anymore...
                firstInstr->AsLabelInstr()->m_isLoopTop = FALSE;
            }
        }

        if (fHasTry)
        {
            this->UpdateRegionForBlock(block);
        }

        if (firstInstr->IsLabelInstr())
        {
            IR::LabelInstr * labelInstr = firstInstr->AsLabelInstr();
            labelInstr->UnlinkBasicBlock();
            if (labelInstr->IsUnreferenced() && !fHasTry)
            {
                // This is an unreferenced label, probably added by FG building.
                // Delete it now to make extended basic blocks visible.
                if (firstInstr == block->GetLastInstr())
                {
                    labelInstr->Remove();
                    continue;
                }
                else
                {
                    labelInstr->Remove();
                }
            }
        }

        // We don't run the globopt with try/catch, don't need to remove branch to next for fall through blocks
        IR::Instr * lastInstr = block->GetLastInstr();
        if (!fHasTry && lastInstr->IsBranchInstr())
        {
            IR::BranchInstr * branchInstr = lastInstr->AsBranchInstr();
            if (!branchInstr->IsConditional() && branchInstr->GetTarget() == branchInstr->m_next)
            {
                // Remove branch to next
                branchInstr->Remove();
            }
        }
    }
    NEXT_BLOCK;

#if DBG

    if (fHasTry)
    {
        // Now that all blocks have regions, we should see consistently propagated regions at all
        // block boundaries.
        FOREACH_BLOCK(block, this)
        {
            Region * region = block->GetFirstInstr()->AsLabelInstr()->GetRegion();
            Region * predRegion = nullptr;
            FOREACH_PREDECESSOR_BLOCK(predBlock, block)
            {
                predRegion = predBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (predBlock->GetLastInstr() == nullptr)
                {
                    AssertMsg(region == predRegion, "Bad region propagation through empty block");
                }
                else
                {
                    switch (predBlock->GetLastInstr()->m_opcode)
                    {
                    case Js::OpCode::TryCatch:
                    case Js::OpCode::TryFinally:
                        AssertMsg(region->GetParent() == predRegion, "Bad region prop on entry to try-catch/finally");
                        if (block->GetFirstInstr() == predBlock->GetLastInstr()->AsBranchInstr()->GetTarget())
                        {
                            if (predBlock->GetLastInstr()->m_opcode == Js::OpCode::TryCatch)
                            {
                                AssertMsg(region->GetType() == RegionTypeCatch, "Bad region type on entry to catch");
                            }
                            else
                            {
                                AssertMsg(region->GetType() == RegionTypeFinally, "Bad region type on entry to finally");
                            }
                        }
                        else
                        {
                            AssertMsg(region->GetType() == RegionTypeTry, "Bad region type on entry to try");
                        }
                        break;
                    case Js::OpCode::Leave:
                        AssertMsg(region == predRegion->GetParent() || (predRegion->GetType() == RegionTypeTry && predRegion->GetMatchingFinallyRegion(false) == region) ||
                            (region == predRegion && this->func->IsLoopBodyInTry()), "Bad region prop on leaving try-catch/finally");
                        break;
                    case Js::OpCode::LeaveNull:
                        AssertMsg(region == predRegion->GetParent() || (region == predRegion && this->func->IsLoopBodyInTry()), "Bad region prop on leaving try-catch/finally");
                        break;

                    // If the try region has a branch out of the loop,
                    // - the branch is moved out of the loop as part of break block removal, and
                    // - BrOnException is inverted to BrOnNoException and a Br is inserted after it.
                    // Otherwise,
                    // - FullJit: BrOnException is removed in the forward pass.
                    case Js::OpCode::BrOnException:
                        Assert(!this->func->DoGlobOpt());
                        break;
                    case Js::OpCode::BrOnNoException:
                        Assert(region->GetType() == RegionTypeTry || region->GetType() == RegionTypeCatch || region->GetType() == RegionTypeFinally ||
                            // A BrOnException from finally to early exit can be converted to BrOnNoException and Br
                            // The Br block maybe a common successor block for early exit along with the BrOnNoException block
                            // Region from Br block will be picked up from a predecessor which is not BrOnNoException due to early exit
                            // See test0() in test/EH/tryfinallytests.js
                            (predRegion->GetType() == RegionTypeFinally && predBlock->GetLastInstr()->AsBranchInstr()->m_brFinallyToEarlyExit));
                        break;
                    case Js::OpCode::Br:
                        if (predBlock->GetLastInstr()->AsBranchInstr()->m_leaveConvToBr)
                        {
                            // Leave converted to Br in finally region
                            AssertMsg(region == predRegion->GetParent(), "Bad region prop in finally");
                        }
                        else if (region->GetType() == RegionTypeCatch && region != predRegion)
                        {
                            AssertMsg(predRegion->GetType() == RegionTypeTry, "Bad region type for the try");
                        }
                        else if (region->GetType() == RegionTypeFinally && region != predRegion)
                        {
                            // We may be left with edges from finally region to early exit
                            AssertMsg(predRegion->IsNonExceptingFinally() || predRegion->GetType() == RegionTypeTry, "Bad region type for the try");
                        }
                        else
                        {
                            // We may be left with edges from finally region to early exit
                            AssertMsg(predRegion->IsNonExceptingFinally() || region == predRegion, "Bad region propagation through interior block");
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            NEXT_PREDECESSOR_BLOCK;

            switch (region->GetType())
            {
            case RegionTypeRoot:
                Assert(!region->GetMatchingTryRegion() && !region->GetMatchingCatchRegion() && !region->GetMatchingFinallyRegion(true) && !region->GetMatchingFinallyRegion(false));
                break;

            case RegionTypeTry:
                if ((this->func->IsSimpleJit() && this->func->hasBailout) || !this->func->DoOptimizeTry())
                {
                    Assert((region->GetMatchingCatchRegion() != nullptr) ^ (region->GetMatchingFinallyRegion(true) && !region->GetMatchingFinallyRegion(false)));
                }
                else
                {
                    Assert((region->GetMatchingCatchRegion() != nullptr) ^ (region->GetMatchingFinallyRegion(true) && region->GetMatchingFinallyRegion(false)));
                }
                break;

            case RegionTypeCatch:
            case RegionTypeFinally:
                Assert(region->GetMatchingTryRegion());
                break;
            }
        }
        NEXT_BLOCK;
        FOREACH_BLOCK_DEAD_OR_ALIVE(block, this)
        {
            if (block->GetFirstInstr()->IsLabelInstr())
            {
                IR::LabelInstr *labelInstr = block->GetFirstInstr()->AsLabelInstr();
                if (labelInstr->IsUnreferenced())
                {
                    // This is an unreferenced label, probably added by FG building.
                    // Delete it now to make extended basic blocks visible.
                    labelInstr->Remove();
                }
            }
        } NEXT_BLOCK_DEAD_OR_ALIVE;
    }
#endif

    this->func->isFlowGraphValid = false;
}

bool FlowGraph::IsEHTransitionInstr(IR::Instr *instr)
{
    Js::OpCode op = instr->m_opcode;
    return (op == Js::OpCode::TryCatch || op == Js::OpCode::TryFinally || op == Js::OpCode::Leave || op == Js::OpCode::LeaveNull);
}

BasicBlock * FlowGraph::GetPredecessorForRegionPropagation(BasicBlock *block)
{
    BasicBlock *ehPred = nullptr;
    FOREACH_PREDECESSOR_BLOCK(predBlock, block)
    {
        Region * predRegion = predBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
        if (IsEHTransitionInstr(predBlock->GetLastInstr()) && predRegion)
        {
            // MGTODO : change this to return, once you know there can exist only one eh transitioning pred
           Assert(ehPred == nullptr);
           ehPred = predBlock;
        }
        AssertMsg(predBlock->GetBlockNum() < this->blockCount, "Misnumbered block at teardown time?");
    }
    NEXT_PREDECESSOR_BLOCK;
    return ehPred;
}

// Propagate the region forward from the block's predecessor(s), tracking the effect
// of the flow transition. Record the region in the block-to-region map provided
// and on the label at the entry to the block (if any).

// We need to know the end of finally for inserting edge at the end of finally to early exit
// Store it in regToFinallyEndMap as we visit blocks instead of recomputing later while adding early exit edges
void
FlowGraph::UpdateRegionForBlock(BasicBlock * block)
{
    Region *region;
    Region * predRegion = nullptr;
    IR::Instr * tryInstr = nullptr;
    IR::Instr * firstInstr = block->GetFirstInstr();
    if (firstInstr->IsLabelInstr() && firstInstr->AsLabelInstr()->GetRegion())
    {
        Assert(this->func->HasTry() && (this->func->DoOptimizeTry() || (this->func->IsSimpleJit() && this->func->hasBailout)));
        return;
    }

    if (block == this->blockList)
    {
        // Head of the graph: create the root region.
        region = Region::New(RegionTypeRoot, nullptr, this->func);
    }
    else
    {
        // Propagate the region forward by finding a predecessor we've already processed.
        region = nullptr;
        FOREACH_PREDECESSOR_BLOCK(predBlock, block)
        {
            AssertMsg(predBlock->GetBlockNum() < this->blockCount, "Misnumbered block at teardown time?");
            predRegion = predBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
            if (predRegion != nullptr)
            {
                region = this->PropagateRegionFromPred(block, predBlock, predRegion, tryInstr);
                break;
            }
        }
        NEXT_PREDECESSOR_BLOCK;

        if (block->GetLastInstr()->m_opcode == Js::OpCode::LeaveNull || block->GetLastInstr()->m_opcode == Js::OpCode::Leave)
        {
            if (this->regToFinallyEndMap && region->IsNonExceptingFinally())
            {
                BasicBlock *endOfFinally = regToFinallyEndMap->ContainsKey(region) ? regToFinallyEndMap->Item(region) : nullptr;
                if (!endOfFinally)
                {
                    regToFinallyEndMap->Add(region, block);
                }
                else
                {
                    Assert(endOfFinally->GetLastInstr()->m_opcode != Js::OpCode::LeaveNull || block == endOfFinally);
                    regToFinallyEndMap->Item(region, block);
                }
            }
        }
    }

    Assert(region || block->GetPredList()->Count() == 0);
    if (region && !region->ehBailoutData)
    {
        region->AllocateEHBailoutData(this->func, tryInstr);
    }

    Assert(firstInstr->IsLabelInstr());
    if (firstInstr->IsLabelInstr())
    {
        // Record the region on the label and make sure it stays around as a region
        // marker if we're entering a region at this point.
        IR::LabelInstr * labelInstr = firstInstr->AsLabelInstr();
        labelInstr->SetRegion(region);
        if (region != predRegion)
        {
            labelInstr->m_hasNonBranchRef = true;
        }

        // One of the pred blocks maybe an eh region, in that case it is important to mark this label's m_hasNonBranchRef
        // If not later in codegen, this label can get deleted. And during SccLiveness, region is propagated to newly created labels in lowerer from the previous label's region
        // We can end up assigning an eh region to a label in a non eh region. And if there is a bailout in such a region, bad things will happen in the interpreter :)
        // See test2()/test3() in tryfinallytests.js
        if (!labelInstr->m_hasNonBranchRef)
        {
            FOREACH_PREDECESSOR_BLOCK(predBlock, block)
            {
                AssertMsg(predBlock->GetBlockNum() < this->blockCount, "Misnumbered block at teardown time?");
                predRegion = predBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (predRegion != region)
                {
                    labelInstr->m_hasNonBranchRef = true;
                    break;
                }
            }
            NEXT_PREDECESSOR_BLOCK;
        }
    }
}

void
FlowGraph::UpdateRegionForBlockFromEHPred(BasicBlock * block, bool reassign)
{
    Region *region = nullptr;
    Region * predRegion = nullptr;
    IR::Instr * tryInstr = nullptr;
    IR::Instr * firstInstr = block->GetFirstInstr();
    if (!reassign && firstInstr->IsLabelInstr() && firstInstr->AsLabelInstr()->GetRegion())
    {
        Assert(this->func->HasTry() && (this->func->DoOptimizeTry() || (this->func->IsSimpleJit() && this->func->hasBailout)));
        return;
    }
    if (block->isDead || block->isDeleted)
    {
        // We can end up calling this function with such blocks, return doing nothing
        // See test5() in tryfinallytests.js
        return;
    }

    if (block == this->blockList)
    {
        // Head of the graph: create the root region.
        region = Region::New(RegionTypeRoot, nullptr, this->func);
    }
    else if (block->GetPredList()->Count() == 1)
    {
        BasicBlock *predBlock = block->GetPredList()->Head()->GetPred();
        AssertMsg(predBlock->GetBlockNum() < this->blockCount, "Misnumbered block at teardown time?");
        predRegion = predBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
        Assert(predRegion);
        region = this->PropagateRegionFromPred(block, predBlock, predRegion, tryInstr);
    }
    else
    {
        // Propagate the region forward by finding a predecessor we've already processed.
        // Since we do break block remval after region propagation, we cannot pick the first predecessor which has an assigned region
        // If there is a eh transitioning pred, we pick that
        // There cannot be more than one eh transitioning pred (?)
        BasicBlock *ehPred = this->GetPredecessorForRegionPropagation(block);
        if (ehPred)
        {
            predRegion = ehPred->GetFirstInstr()->AsLabelInstr()->GetRegion();
            Assert(predRegion != nullptr);
            region = this->PropagateRegionFromPred(block, ehPred, predRegion, tryInstr);
        }
        else
        {
            FOREACH_PREDECESSOR_BLOCK(predBlock, block)
            {
                predRegion = predBlock->GetFirstInstr()->AsLabelInstr()->GetRegion();
                if (predRegion != nullptr)
                {
                    if ((predBlock->GetLastInstr()->m_opcode == Js::OpCode::BrOnException || predBlock->GetLastInstr()->m_opcode == Js::OpCode::BrOnNoException) &&
                        predBlock->GetLastInstr()->AsBranchInstr()->m_brFinallyToEarlyExit)
                    {
                        Assert(predRegion->IsNonExceptingFinally());
                        // BrOnException from finally region to early exit
                        // Skip this edge
                        continue;
                    }
                    if (predBlock->GetLastInstr()->m_opcode == Js::OpCode::Br &&
                        predBlock->GetLastInstr()->GetPrevRealInstr()->m_opcode == Js::OpCode::BrOnNoException)
                    {
                        Assert(predBlock->GetLastInstr()->GetPrevRealInstr()->AsBranchInstr()->m_brFinallyToEarlyExit);
                        Assert(predRegion->IsNonExceptingFinally());
                        // BrOnException from finally region to early exit changed to BrOnNoException and Br during break block removal
                        continue;
                    }
                    region = this->PropagateRegionFromPred(block, predBlock, predRegion, tryInstr);
                    break;
                }
            }
            NEXT_PREDECESSOR_BLOCK;
        }
    }

    Assert(region || block->GetPredList()->Count() == 0 || block->firstInstr->AsLabelInstr()->GetRegion());

    if (region)
    { 
        if (!region->ehBailoutData)
        {
            region->AllocateEHBailoutData(this->func, tryInstr);
        }

        Assert(firstInstr->IsLabelInstr());
        if (firstInstr->IsLabelInstr())
        {
            // Record the region on the label and make sure it stays around as a region
            // marker if we're entering a region at this point.
            IR::LabelInstr * labelInstr = firstInstr->AsLabelInstr();
            labelInstr->SetRegion(region);
            if (region != predRegion)
            {
                labelInstr->m_hasNonBranchRef = true;
            }
        }
    }
}

Region *
FlowGraph::PropagateRegionFromPred(BasicBlock * block, BasicBlock * predBlock, Region * predRegion, IR::Instr * &tryInstr)
{
    // Propagate predRegion to region, looking at the flow transition for an opcode
    // that affects the region.
    Region * region = nullptr;
    IR::Instr * predLastInstr = predBlock->GetLastInstr();
    IR::Instr * firstInstr = block->GetFirstInstr();
    if (predLastInstr == nullptr)
    {
        // Empty block: trivially propagate the region.
        region = predRegion;
    }
    else
    {
        Region * tryRegion = nullptr;
        IR::LabelInstr * tryInstrNext = nullptr;
        switch (predLastInstr->m_opcode)
        {
        case Js::OpCode::TryCatch:
            // Entry to a try-catch. See whether we're entering the try or the catch
            // by looking for the handler label.
            Assert(predLastInstr->m_next->IsLabelInstr());
            tryInstrNext = predLastInstr->m_next->AsLabelInstr();
            tryRegion = tryInstrNext->GetRegion();

            if (firstInstr == predLastInstr->AsBranchInstr()->GetTarget())
            {
                region = Region::New(RegionTypeCatch, predRegion, this->func);
                Assert(tryRegion);
                region->SetMatchingTryRegion(tryRegion);
                tryRegion->SetMatchingCatchRegion(region);
            }
            else
            {
                region = Region::New(RegionTypeTry, predRegion, this->func);
                tryInstr = predLastInstr;
            }
            break;

        case Js::OpCode::TryFinally:
            // Entry to a try-finally. See whether we're entering the try or the finally
            // by looking for the handler label.
            Assert(predLastInstr->m_next->IsLabelInstr());
            tryInstrNext = predLastInstr->m_next->AsLabelInstr();
            tryRegion = tryInstrNext->GetRegion();

            if (firstInstr == predLastInstr->AsBranchInstr()->GetTarget())
            {
                Assert(tryRegion && tryRegion->GetType() == RegionType::RegionTypeTry);
                region = Region::New(RegionTypeFinally, predRegion, this->func);
                region->SetMatchingTryRegion(tryRegion);
                tryRegion->SetMatchingFinallyRegion(region, true);
                tryInstr = predLastInstr;
            }
            else
            {
                region = Region::New(RegionTypeTry, predRegion, this->func);
                tryInstr = predLastInstr;
            }
            break;

        case Js::OpCode::Leave:
            if (firstInstr->m_next && firstInstr->m_next->m_opcode == Js::OpCode::Finally)
            {
                tryRegion = predRegion;
                Assert(tryRegion->GetMatchingFinallyRegion(true) != nullptr);
                region = Region::New(RegionTypeFinally, predRegion->GetParent(), this->func);
                Assert(tryRegion && tryRegion->GetType() == RegionType::RegionTypeTry);
                region->SetMatchingTryRegion(tryRegion);
                tryRegion->SetMatchingFinallyRegion(region, false);
                break;
            }

            // Exiting a try or handler. Retrieve the current region's parent.
            region = predRegion->GetParent();
            if (region == nullptr)
            {
                // We found a Leave in the root region- this can only happen when a jitted loop body
                // in a try block has a return statement.
                Assert(this->func->IsLoopBodyInTry());
                predLastInstr->AsBranchInstr()->m_isOrphanedLeave = true;
                region = predRegion;
            }
            break;
        case Js::OpCode::LeaveNull:
            // Exiting a try or handler. Retrieve the current region's parent.
            region = predRegion->GetParent();
            if (region == nullptr)
            {
                // We found a Leave in the root region- this can only happen when a jitted loop body
                // in a try block has a return statement.
                Assert(this->func->IsLoopBodyInTry());
                predLastInstr->AsBranchInstr()->m_isOrphanedLeave = true;
                region = predRegion;
            }
            break;
        case Js::OpCode::BailOnException:
            // Infinite loop, no edge to non excepting finally
            if (firstInstr->m_next && firstInstr->m_next->m_opcode == Js::OpCode::Finally)
            {
                tryRegion = predRegion->GetMatchingTryRegion();
                region = Region::New(RegionTypeFinally, predRegion->GetParent(), this->func);
                Assert(tryRegion && tryRegion->GetType() == RegionType::RegionTypeTry);
                region->SetMatchingTryRegion(tryRegion);
                tryRegion->SetMatchingFinallyRegion(region, false);
            }
            break;
        case Js::OpCode::BrOnException:
            // Infinite loop inside another EH region within finally,
            // We have added edges for all infinite loops inside a finally, identify that and transition to parent
            if (predRegion->GetType() != RegionTypeFinally && firstInstr->GetNextRealInstr()->m_opcode == Js::OpCode::LeaveNull)
            {
                region = predRegion->GetParent();
            }
            else
            {
                region = predRegion;
            }
            break;
        default:
            // Normal (non-EH) transition: just propagate the region.
            region = predRegion;
            break;
        }
    }
    return region;
}

void
FlowGraph::InsertCompBlockToLoopList(Loop *loop, BasicBlock* compBlock, BasicBlock* targetBlock, bool postTarget)
{
    if (loop)
    {
        bool found = false;
        FOREACH_BLOCK_IN_LOOP_EDITING(loopBlock, loop, iter)
        {
            if (loopBlock == targetBlock)
            {
                found = true;
                break;
            }
        } NEXT_BLOCK_IN_LOOP_EDITING;
        if (found)
        {
            if (postTarget)
            {
                iter.Next();
            }
            iter.InsertBefore(compBlock);
        }
        InsertCompBlockToLoopList(loop->parent, compBlock, targetBlock, postTarget);
    }
}

// Insert a block on the given edge
BasicBlock *
FlowGraph::InsertAirlockBlock(FlowEdge * edge)
{
    BasicBlock * airlockBlock = BasicBlock::New(this);
    BasicBlock * sourceBlock = edge->GetPred();
    BasicBlock * sinkBlock = edge->GetSucc();

    BasicBlock * sinkPrevBlock = sinkBlock->prev;
    IR::Instr *  sinkPrevBlockLastInstr = sinkPrevBlock->GetLastInstr();
    IR::Instr * sourceLastInstr = sourceBlock->GetLastInstr();

    airlockBlock->loop = sinkBlock->loop;
    airlockBlock->SetBlockNum(this->blockCount++);
#ifdef DBG
    airlockBlock->isAirLockBlock = true;
#endif
    //
    // Fixup block linkage
    //

    // airlock block is inserted right before sourceBlock
    airlockBlock->prev = sinkBlock->prev;
    sinkBlock->prev = airlockBlock;

    airlockBlock->next = sinkBlock;
    airlockBlock->prev->next = airlockBlock;

    //
    // Fixup flow edges
    //

    sourceBlock->RemoveSucc(sinkBlock, this, false);

    // Add sourceBlock -> airlockBlock
    this->AddEdge(sourceBlock, airlockBlock);

    // Add airlockBlock -> sinkBlock
    edge->SetPred(airlockBlock);
    airlockBlock->AddSucc(edge, this);

    // Fixup data use count
    airlockBlock->SetDataUseCount(1);
    sourceBlock->DecrementDataUseCount();

    //
    // Fixup IR
    //

    // Maintain the instruction region for inlining
    IR::LabelInstr *sinkLabel = sinkBlock->GetFirstInstr()->AsLabelInstr();
    Func * sinkLabelFunc = sinkLabel->m_func;

    IR::LabelInstr *airlockLabel = IR::LabelInstr::New(Js::OpCode::Label, sinkLabelFunc);

    sinkPrevBlockLastInstr->InsertAfter(airlockLabel);

    airlockBlock->SetFirstInstr(airlockLabel);
    airlockLabel->SetBasicBlock(airlockBlock);

    // Add br to sinkBlock from airlock block
    IR::BranchInstr *airlockBr = IR::BranchInstr::New(Js::OpCode::Br, sinkLabel, sinkLabelFunc);
    airlockBr->SetByteCodeOffset(sinkLabel);
    airlockLabel->InsertAfter(airlockBr);
    airlockBlock->SetLastInstr(airlockBr);

    airlockLabel->SetByteCodeOffset(sinkLabel);

    // Fixup flow out of sourceBlock
    IR::BranchInstr *sourceBr = sourceLastInstr->AsBranchInstr();
    if (sourceBr->IsMultiBranch())
    {
        const bool replaced = sourceBr->AsMultiBrInstr()->ReplaceTarget(sinkLabel, airlockLabel);
        Assert(replaced);
    }
    else if (sourceBr->GetTarget() == sinkLabel)
    {
        sourceBr->SetTarget(airlockLabel);
    }

    if (!sinkPrevBlockLastInstr->IsBranchInstr() || sinkPrevBlockLastInstr->AsBranchInstr()->HasFallThrough())
    {
        if (!sinkPrevBlock->isDeleted)
        {
            FlowEdge *dstEdge = this->FindEdge(sinkPrevBlock, sinkBlock);
            if (dstEdge) // Possibility that sourceblock may be same as sinkPrevBlock
            {
                BasicBlock* compensationBlock = this->InsertCompensationCodeForBlockMove(dstEdge, true /*insert comp block to loop list*/, true);
                compensationBlock->IncrementDataUseCount();
                // We need to skip airlock compensation block in globopt as its inserted while globopt is iteration over the blocks.
                compensationBlock->isAirLockCompensationBlock = true;
            }
        }
    }

#if DBG_DUMP
    this->Dump(true, _u("\n After insertion of airlock block \n"));
#endif

    return airlockBlock;
}

// Insert a block on the given edge
BasicBlock *
FlowGraph::InsertCompensationCodeForBlockMove(FlowEdge * edge,  bool insertToLoopList, bool sinkBlockLoop)
{
    BasicBlock * compBlock = BasicBlock::New(this);
    BasicBlock * sourceBlock = edge->GetPred();
    BasicBlock * sinkBlock = edge->GetSucc();
    BasicBlock * fallthroughBlock = sourceBlock->next;
    IR::Instr *  sourceLastInstr = sourceBlock->GetLastInstr();

    compBlock->SetBlockNum(this->blockCount++);

    if (insertToLoopList)
    {
        // For flow graph edits in
        if (sinkBlockLoop)
        {
            if (sinkBlock->loop && sinkBlock->loop->GetHeadBlock() == sinkBlock)
            {
                // BLUE 531255: sinkblock may be the head block of new loop, we shouldn't insert compensation block to that loop
                // Insert it to all the parent loop lists.
                compBlock->loop = sinkBlock->loop->parent;
                InsertCompBlockToLoopList(compBlock->loop, compBlock, sinkBlock, false);
            }
            else
            {
                compBlock->loop = sinkBlock->loop;
                InsertCompBlockToLoopList(compBlock->loop, compBlock, sinkBlock, false); // sinkBlock or fallthroughBlock?
            }
#ifdef DBG
            compBlock->isBreakCompensationBlockAtSink = true;
#endif
        }
        else
        {
            compBlock->loop = sourceBlock->loop;
            InsertCompBlockToLoopList(compBlock->loop, compBlock, sourceBlock, true);
#ifdef DBG
            compBlock->isBreakCompensationBlockAtSource = true;
#endif
        }
    }

    //
    // Fixup block linkage
    //

    // compensation block is inserted right after sourceBlock
    compBlock->next = fallthroughBlock;
    fallthroughBlock->prev = compBlock;

    compBlock->prev = sourceBlock;
    sourceBlock->next = compBlock;

    //
    // Fixup flow edges
    //
    sourceBlock->RemoveSucc(sinkBlock, this, false);

    // Add sourceBlock -> airlockBlock
    this->AddEdge(sourceBlock, compBlock);

    // Add airlockBlock -> sinkBlock
    edge->SetPred(compBlock);
    compBlock->AddSucc(edge, this);

    //
    // Fixup IR
    //

    // Maintain the instruction region for inlining
    IR::LabelInstr *sinkLabel = sinkBlock->GetFirstInstr()->AsLabelInstr();
    Func * sinkLabelFunc = sinkLabel->m_func;

    IR::LabelInstr *compLabel = IR::LabelInstr::New(Js::OpCode::Label, sinkLabelFunc);
    sourceLastInstr->InsertAfter(compLabel);
    compBlock->SetFirstInstr(compLabel);
    compLabel->SetBasicBlock(compBlock);

    // Add br to sinkBlock from compensation block
    IR::BranchInstr *compBr = IR::BranchInstr::New(Js::OpCode::Br, sinkLabel, sinkLabelFunc);
    compBr->SetByteCodeOffset(sinkLabel);
    compLabel->InsertAfter(compBr);
    compBlock->SetLastInstr(compBr);

    compLabel->SetByteCodeOffset(sinkLabel);

    // Fixup flow out of sourceBlock
    if (sourceLastInstr->IsBranchInstr())
    {
        IR::BranchInstr *sourceBr = sourceLastInstr->AsBranchInstr();
        Assert(sourceBr->IsMultiBranch() || sourceBr->IsConditional());
        if (sourceBr->IsMultiBranch())
        {
            const bool replaced = sourceBr->AsMultiBrInstr()->ReplaceTarget(sinkLabel, compLabel);
            Assert(replaced);
        }
    }

    bool assignRegionsBeforeGlobopt = this->func->HasTry() && (this->func->DoOptimizeTry() ||
        (this->func->IsSimpleJit() && this->func->hasBailout));

    if (assignRegionsBeforeGlobopt)
    {
        UpdateRegionForBlockFromEHPred(compBlock);
    }

    return compBlock;
}

void
FlowGraph::RemoveUnreachableBlocks()
{
    AnalysisAssert(this->blockList);

    FOREACH_BLOCK(block, this)
    {
        block->isVisited = false;
    }
    NEXT_BLOCK;

    this->blockList->isVisited = true;

    FOREACH_BLOCK_EDITING(block, this)
    {
        if (block->isVisited)
        {
            FOREACH_SUCCESSOR_BLOCK(succ, block)
            {
                succ->isVisited = true;
            } NEXT_SUCCESSOR_BLOCK;
        }
        else
        {
            this->RemoveBlock(block);
        }
    }
    NEXT_BLOCK_EDITING;
}

// If block has no predecessor, remove it.
bool
FlowGraph::RemoveUnreachableBlock(BasicBlock *block, GlobOpt * globOpt)
{
    bool isDead = false;

    if ((block->GetPredList() == nullptr || block->GetPredList()->Empty()) && block != this->func->m_fg->blockList)
    {
        isDead = true;
    }
    else if (block->isLoopHeader)
    {
        // A dead loop still has back-edges pointing to it...
        isDead = true;
        FOREACH_PREDECESSOR_BLOCK(pred, block)
        {
            if (!block->loop->IsDescendentOrSelf(pred->loop))
            {
                isDead = false;
            }
        } NEXT_PREDECESSOR_BLOCK;
    }

    if (isDead)
    {
        this->RemoveBlock(block, globOpt);
        return true;
    }
    return false;
}

IR::Instr *
FlowGraph::PeepTypedCm(IR::Instr *instr)
{
    // Basic pattern, peep:
    //      t1 = CmEq a, b
    //      BrTrue_I4 $L1, t1
    // Into:
    //      t1 = 1
    //      BrEq $L1, a, b
    //      t1 = 0

    IR::Instr * instrNext = instr->GetNextRealInstrOrLabel();

    // find intermediate Lds
    IR::Instr * instrLd = nullptr;
    if (instrNext->m_opcode == Js::OpCode::Ld_I4)
    {
        instrLd = instrNext;
        instrNext = instrNext->GetNextRealInstrOrLabel();
    }

    IR::Instr * instrLd2 = nullptr;
    if (instrNext->m_opcode == Js::OpCode::Ld_I4)
    {
        instrLd2 = instrNext;
        instrNext = instrNext->GetNextRealInstrOrLabel();
    }

    // Find BrTrue/BrFalse
    IR::Instr *instrBr;
    bool brIsTrue;
    if (instrNext->m_opcode == Js::OpCode::BrTrue_I4)
    {
        instrBr = instrNext;
        brIsTrue = true;
    }
    else if (instrNext->m_opcode == Js::OpCode::BrFalse_I4)
    {
        instrBr = instrNext;
        brIsTrue = false;
    }
    else
    {
        return nullptr;
    }

    AssertMsg(instrLd || (!instrLd && !instrLd2), "Either instrLd is non-null or both null");

    // if we have intermediate Lds, then make sure pattern is:
    //      t1 = CmEq a, b
    //      t2 = Ld_A t1
    //      BrTrue $L1, t2
    if (instrLd && !instrLd->GetSrc1()->IsEqual(instr->GetDst()))
    {
        return nullptr;
    }

    if (instrLd2 && !instrLd2->GetSrc1()->IsEqual(instrLd->GetDst()))
    {
        return nullptr;
    }

    // Make sure we have:
    //      t1 = CmEq a, b
    //           BrTrue/BrFalse t1
    if (!(instr->GetDst()->IsEqual(instrBr->GetSrc1()) || (instrLd && instrLd->GetDst()->IsEqual(instrBr->GetSrc1())) || (instrLd2 && instrLd2->GetDst()->IsEqual(instrBr->GetSrc1()))))
    {
        return nullptr;
    }

    IR::Opnd * src1 = instr->UnlinkSrc1();
    IR::Opnd * src2 = instr->UnlinkSrc2();

    IR::Instr * instrNew;
    IR::Opnd * tmpOpnd;
    if (instr->GetDst()->IsEqual(src1) || (instrLd && instrLd->GetDst()->IsEqual(src1)) || (instrLd2 && instrLd2->GetDst()->IsEqual(src1)))
    {
        Assert(src1->IsInt32());

        tmpOpnd = IR::RegOpnd::New(TyInt32, instr->m_func);
        instrNew = IR::Instr::New(Js::OpCode::Ld_I4, tmpOpnd, src1, instr->m_func);
        instrNew->SetByteCodeOffset(instr);
        instr->InsertBefore(instrNew);
        src1 = tmpOpnd;
    }

    if (instr->GetDst()->IsEqual(src2) || (instrLd && instrLd->GetDst()->IsEqual(src2)) || (instrLd2 && instrLd2->GetDst()->IsEqual(src2)))
    {
        Assert(src2->IsInt32());

        tmpOpnd = IR::RegOpnd::New(TyInt32, instr->m_func);
        instrNew = IR::Instr::New(Js::OpCode::Ld_I4, tmpOpnd, src2, instr->m_func);
        instrNew->SetByteCodeOffset(instr);
        instr->InsertBefore(instrNew);
        src2 = tmpOpnd;
    }

    instrBr->ReplaceSrc1(src1);
    instrBr->SetSrc2(src2);

    Js::OpCode newOpcode;
    switch (instr->m_opcode)
    {
    case Js::OpCode::CmEq_I4:
        newOpcode = Js::OpCode::BrEq_I4;
        break;
    case Js::OpCode::CmGe_I4:
        newOpcode = Js::OpCode::BrGe_I4;
        break;
    case Js::OpCode::CmGt_I4:
        newOpcode = Js::OpCode::BrGt_I4;
        break;
    case Js::OpCode::CmLt_I4:
        newOpcode = Js::OpCode::BrLt_I4;
        break;
    case Js::OpCode::CmLe_I4:
        newOpcode = Js::OpCode::BrLe_I4;
        break;
    case Js::OpCode::CmUnGe_I4:
        newOpcode = Js::OpCode::BrUnGe_I4;
        break;
    case Js::OpCode::CmUnGt_I4:
        newOpcode = Js::OpCode::BrUnGt_I4;
        break;
    case Js::OpCode::CmUnLt_I4:
        newOpcode = Js::OpCode::BrUnLt_I4;
        break;
    case Js::OpCode::CmUnLe_I4:
        newOpcode = Js::OpCode::BrUnLe_I4;
        break;
    case Js::OpCode::CmNeq_I4:
        newOpcode = Js::OpCode::BrNeq_I4;
        break;
    case Js::OpCode::CmEq_A:
        newOpcode = Js::OpCode::BrEq_A;
        break;
    case Js::OpCode::CmGe_A:
        newOpcode = Js::OpCode::BrGe_A;
        break;
    case Js::OpCode::CmGt_A:
        newOpcode = Js::OpCode::BrGt_A;
        break;
    case Js::OpCode::CmLt_A:
        newOpcode = Js::OpCode::BrLt_A;
        break;
    case Js::OpCode::CmLe_A:
        newOpcode = Js::OpCode::BrLe_A;
        break;
    case Js::OpCode::CmUnGe_A:
        newOpcode = Js::OpCode::BrUnGe_A;
        break;
    case Js::OpCode::CmUnGt_A:
        newOpcode = Js::OpCode::BrUnGt_A;
        break;
    case Js::OpCode::CmUnLt_A:
        newOpcode = Js::OpCode::BrUnLt_A;
        break;
    case Js::OpCode::CmUnLe_A:
        newOpcode = Js::OpCode::BrUnLe_A;
        break;
    case Js::OpCode::CmNeq_A:
        newOpcode = Js::OpCode::BrNeq_A;
        break;
    case Js::OpCode::CmSrEq_A:
        newOpcode = Js::OpCode::BrSrEq_A;
        break;
    case Js::OpCode::CmSrNeq_A:
        newOpcode = Js::OpCode::BrSrNeq_A;
        break;
    default:
        newOpcode = Js::OpCode::InvalidOpCode;
        Assume(UNREACHED);
    }

    instrBr->m_opcode = newOpcode;

    if (brIsTrue)
    {
        instr->SetSrc1(IR::IntConstOpnd::New(1, TyInt8, instr->m_func));
        instr->m_opcode = Js::OpCode::Ld_I4;
        instrNew = IR::Instr::New(Js::OpCode::Ld_I4, instr->GetDst(), IR::IntConstOpnd::New(0, TyInt8, instr->m_func), instr->m_func);
        instrNew->SetByteCodeOffset(instrBr);
        instrBr->InsertAfter(instrNew);
        if (instrLd)
        {
            instrLd->ReplaceSrc1(IR::IntConstOpnd::New(1, TyInt8, instr->m_func));
            instrNew = IR::Instr::New(Js::OpCode::Ld_I4, instrLd->GetDst(), IR::IntConstOpnd::New(0, TyInt8, instr->m_func), instr->m_func);
            instrNew->SetByteCodeOffset(instrBr);
            instrBr->InsertAfter(instrNew);

            if (instrLd2)
            {
                instrLd2->ReplaceSrc1(IR::IntConstOpnd::New(1, TyInt8, instr->m_func));
                instrNew = IR::Instr::New(Js::OpCode::Ld_I4, instrLd2->GetDst(), IR::IntConstOpnd::New(0, TyInt8, instr->m_func), instr->m_func);
                instrNew->SetByteCodeOffset(instrBr);
                instrBr->InsertAfter(instrNew);
            }
        }
    }
    else
    {
        instrBr->AsBranchInstr()->Invert();

        instr->SetSrc1(IR::IntConstOpnd::New(0, TyInt8, instr->m_func));
        instr->m_opcode = Js::OpCode::Ld_I4;
        instrNew = IR::Instr::New(Js::OpCode::Ld_I4, instr->GetDst(), IR::IntConstOpnd::New(1, TyInt8, instr->m_func), instr->m_func);
        instrNew->SetByteCodeOffset(instrBr);
        instrBr->InsertAfter(instrNew);
        if (instrLd)
        {
            instrLd->ReplaceSrc1(IR::IntConstOpnd::New(0, TyInt8, instr->m_func));
            instrNew = IR::Instr::New(Js::OpCode::Ld_I4, instrLd->GetDst(), IR::IntConstOpnd::New(1, TyInt8, instr->m_func), instr->m_func);
            instrNew->SetByteCodeOffset(instrBr);
            instrBr->InsertAfter(instrNew);

            if (instrLd2)
            {
                instrLd2->ReplaceSrc1(IR::IntConstOpnd::New(0, TyInt8, instr->m_func));
                instrNew = IR::Instr::New(Js::OpCode::Ld_I4, instrLd2->GetDst(), IR::IntConstOpnd::New(1, TyInt8, instr->m_func), instr->m_func);
                instrNew->SetByteCodeOffset(instrBr);
                instrBr->InsertAfter(instrNew);
            }
        }
    }

    return instrBr;
}

IR::Instr *
FlowGraph::PeepCm(IR::Instr *instr)
{
    // Basic pattern, peep:
    //      t1 = CmEq a, b
    //      t2 = Ld_A t1
    //      BrTrue $L1, t2
    // Into:
    //      t1 = True
    //      t2 = True
    //      BrEq $L1, a, b
    //      t1 = False
    //      t2 = False
    //
    //  The true/false Ld_A's will most likely end up being dead-stores...

    //  Alternate Pattern
    //      t1= CmEq a, b
    //      BrTrue $L1, t1
    // Into:
    //      BrEq $L1, a, b

    Func *func = instr->m_func;

    // Find Ld_A
    IR::Instr *instrNext = instr->GetNextRealInstrOrLabel();
    IR::Instr *inlineeEndInstr = nullptr;
    IR::Instr *instrNew;
    IR::Instr *instrLd = nullptr, *instrLd2 = nullptr;
    IR::Instr *instrByteCode = instrNext;
    bool ldFound = false;
    IR::Opnd *brSrc = instr->GetDst();

    if (instrNext->m_opcode == Js::OpCode::Ld_A && instrNext->GetSrc1()->IsEqual(instr->GetDst()))
    {
        ldFound = true;
        instrLd = instrNext;
        brSrc = instrNext->GetDst();

        if (brSrc->IsEqual(instr->GetSrc1()) || brSrc->IsEqual(instr->GetSrc2()))
        {
            return nullptr;
        }

        instrNext = instrLd->GetNextRealInstrOrLabel();

        // Is there a second Ld_A?
        if (instrNext->m_opcode == Js::OpCode::Ld_A && instrNext->GetSrc1()->IsEqual(brSrc))
        {
            // We have:
            //      t1 = Cm
            //      t2 = t1     // ldSrc = t1
            //      t3 = t2     // ldDst = t3
            //      BrTrue/BrFalse t3

            instrLd2 = instrNext;
            brSrc = instrLd2->GetDst();
            instrNext = instrLd2->GetNextRealInstrOrLabel();
            if (brSrc->IsEqual(instr->GetSrc1()) || brSrc->IsEqual(instr->GetSrc2()))
            {
                return nullptr;
            }
        }
    }

    // Skip over InlineeEnd
    if (instrNext->m_opcode == Js::OpCode::InlineeEnd)
    {
        inlineeEndInstr = instrNext;
        instrNext = inlineeEndInstr->GetNextRealInstrOrLabel();
    }

    // Find BrTrue/BrFalse
    bool brIsTrue;
    if (instrNext->m_opcode == Js::OpCode::BrTrue_A)
    {
        brIsTrue = true;
    }
    else if (instrNext->m_opcode == Js::OpCode::BrFalse_A)
    {
        brIsTrue = false;
    }
    else
    {
        return nullptr;
    }

    IR::Instr *instrBr = instrNext;

    // Make sure we have:
    //      t1 = Ld_A
    //         BrTrue/BrFalse t1
    if (!instr->GetDst()->IsEqual(instrBr->GetSrc1()) && !brSrc->IsEqual(instrBr->GetSrc1()))
    {
        return nullptr;
    }

    //
    // We have a match.  Generate the new branch
    //

    // BrTrue/BrFalse t1
    // Keep a copy of the inliner func and the bytecode offset of the original BrTrue/BrFalse if we end up inserting a new branch out of the inlinee,
    // and sym id of t1 for proper restoration on a bailout before the branch.
    Func* origBrFunc = instrBr->m_func;
    uint32 origBrByteCodeOffset = instrBr->GetByteCodeOffset();
    uint32 origBranchSrcSymId = instrBr->GetSrc1()->GetStackSym()->m_id;
    bool origBranchSrcOpndIsJITOpt = instrBr->GetSrc1()->GetIsJITOptimizedReg();

    instrBr->Unlink();
    instr->InsertBefore(instrBr);
    instrBr->ClearByteCodeOffset();
    instrBr->SetByteCodeOffset(instr);
    instrBr->FreeSrc1();
    instrBr->SetSrc1(instr->UnlinkSrc1());
    instrBr->SetSrc2(instr->UnlinkSrc2());
    instrBr->m_func = instr->m_func;

    Js::OpCode newOpcode = Js::OpCode::InvalidOpCode;

    switch(instr->m_opcode)
    {
    case Js::OpCode::CmEq_A:
        newOpcode = Js::OpCode::BrEq_A;
        break;
    case Js::OpCode::CmGe_A:
        newOpcode = Js::OpCode::BrGe_A;
        break;
    case Js::OpCode::CmGt_A:
        newOpcode = Js::OpCode::BrGt_A;
        break;
    case Js::OpCode::CmLt_A:
        newOpcode = Js::OpCode::BrLt_A;
        break;
    case Js::OpCode::CmLe_A:
        newOpcode = Js::OpCode::BrLe_A;
        break;
    case Js::OpCode::CmUnGe_A:
        newOpcode = Js::OpCode::BrUnGe_A;
        break;
    case Js::OpCode::CmUnGt_A:
        newOpcode = Js::OpCode::BrUnGt_A;
        break;
    case Js::OpCode::CmUnLt_A:
        newOpcode = Js::OpCode::BrUnLt_A;
        break;
    case Js::OpCode::CmUnLe_A:
        newOpcode = Js::OpCode::BrUnLe_A;
        break;
    case Js::OpCode::CmNeq_A:
        newOpcode = Js::OpCode::BrNeq_A;
        break;
    case Js::OpCode::CmSrEq_A:
        newOpcode = Js::OpCode::BrSrEq_A;
        break;
    case Js::OpCode::CmSrNeq_A:
        newOpcode = Js::OpCode::BrSrNeq_A;
        break;
    default:
        Assert(UNREACHED);
        __assume(UNREACHED);
    }

    instrBr->m_opcode = newOpcode;

    IR::AddrOpnd* trueOpnd = IR::AddrOpnd::New(func->GetScriptContextInfo()->GetTrueAddr(), IR::AddrOpndKindDynamicVar, func, true);
    IR::AddrOpnd* falseOpnd = IR::AddrOpnd::New(func->GetScriptContextInfo()->GetFalseAddr(), IR::AddrOpndKindDynamicVar, func, true);

    trueOpnd->SetValueType(ValueType::Boolean);
    falseOpnd->SetValueType(ValueType::Boolean);

    if (ldFound)
    {
        // Split Ld_A into "Ld_A TRUE"/"Ld_A FALSE"
        if (brIsTrue)
        {
            instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetSrc1(), trueOpnd, instrBr->m_func);
            instrNew->SetByteCodeOffset(instrBr);
            instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
            instrBr->InsertBefore(instrNew);
            instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetDst(), trueOpnd, instrBr->m_func);
            instrNew->SetByteCodeOffset(instrBr);
            instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
            instrBr->InsertBefore(instrNew);

            instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetSrc1(), falseOpnd, instrLd->m_func);
            instrLd->InsertBefore(instrNew);
            instrNew->SetByteCodeOffset(instrLd);
            instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
            instrLd->ReplaceSrc1(falseOpnd);

            if (instrLd2)
            {
                instrLd2->ReplaceSrc1(falseOpnd);

                instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd2->GetDst(), trueOpnd, instrBr->m_func);
                instrBr->InsertBefore(instrNew);
                instrNew->SetByteCodeOffset(instrBr);
                instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
            }
        }
        else
        {
            instrBr->AsBranchInstr()->Invert();

            instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetSrc1(), falseOpnd, instrBr->m_func);
            instrBr->InsertBefore(instrNew);
            instrNew->SetByteCodeOffset(instrBr);
            instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
            instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetDst(), falseOpnd, instrBr->m_func);
            instrBr->InsertBefore(instrNew);
            instrNew->SetByteCodeOffset(instrBr);
            instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;

            instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetSrc1(), trueOpnd, instrLd->m_func);
            instrLd->InsertBefore(instrNew);
            instrNew->SetByteCodeOffset(instrLd);
            instrLd->ReplaceSrc1(trueOpnd);
            instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;

            if (instrLd2)
            {
                instrLd2->ReplaceSrc1(trueOpnd);
                instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd->GetSrc1(), trueOpnd, instrBr->m_func);
                instrBr->InsertBefore(instrNew);
                instrNew->SetByteCodeOffset(instrBr);
                instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
            }
        }
    }

    // Fix InlineeEnd
    if (inlineeEndInstr)
    {
        this->InsertInlineeOnFLowEdge(instrBr->AsBranchInstr(), inlineeEndInstr, instrByteCode , origBrFunc, origBrByteCodeOffset, origBranchSrcOpndIsJITOpt, origBranchSrcSymId);
    }

    if (instr->GetDst()->AsRegOpnd()->m_sym->HasByteCodeRegSlot())
    {
        Assert(!instrBr->AsBranchInstr()->HasByteCodeReg());
        StackSym *dstSym = instr->GetDst()->AsRegOpnd()->m_sym;
        instrBr->AsBranchInstr()->SetByteCodeReg(dstSym->GetByteCodeRegSlot());
    }
    instr->Remove();

    //
    // Try optimizing through a second branch.
    // Peep:
    //
    //      t2 = True;
    //      BrTrue  $L1
    //      ...
    //   L1:
    //      t1 = Ld_A t2
    //      BrTrue  $L2
    //
    // Into:
    //      t2 = True;
    //      t1 = True;
    //      BrTrue  $L2 <---
    //      ...
    //   L1:
    //      t1 = Ld_A t2
    //      BrTrue  $L2
    //
    // This cleanup helps expose second level Cm peeps.

    IR::Instr *instrLd3 = instrBr->AsBranchInstr()->GetTarget()->GetNextRealInstrOrLabel();

    // Skip over branch to branch
    while (instrLd3->m_opcode == Js::OpCode::Br)
    {
        instrLd3 = instrLd3->AsBranchInstr()->GetTarget()->GetNextRealInstrOrLabel();
    }

    // Find Ld_A
    if (instrLd3->m_opcode != Js::OpCode::Ld_A)
    {
        return instrBr;
    }

    IR::Instr *instrBr2 = instrLd3->GetNextRealInstrOrLabel();
    IR::Instr *inlineeEndInstr2 = nullptr;

    // InlineeEnd?
    // REVIEW: Can we handle 2 inlineeEnds?
    if (instrBr2->m_opcode == Js::OpCode::InlineeEnd && !inlineeEndInstr)
    {
        inlineeEndInstr2 = instrBr2;
        instrBr2 = instrBr2->GetNextRealInstrOrLabel();
    }

    // Find branch
    bool brIsTrue2;
    if (instrBr2->m_opcode == Js::OpCode::BrTrue_A)
    {
        brIsTrue2 = true;
    }
    else if (instrBr2->m_opcode == Js::OpCode::BrFalse_A)
    {
        brIsTrue2 = false;
    }
    else
    {
        return nullptr;
    }

    // Make sure Ld_A operates on the right tmps.
    if (!instrLd3->GetDst()->IsEqual(instrBr2->GetSrc1()) || !brSrc->IsEqual(instrLd3->GetSrc1()))
    {
        return nullptr;
    }

    if (instrLd3->GetDst()->IsEqual(instrBr->GetSrc1()) || instrLd3->GetDst()->IsEqual(instrBr->GetSrc2()))
    {
        return nullptr;
    }

    // Make sure that the reg we're assigning to is not live in the intervening instructions (if this is a forward branch).
    if (instrLd3->GetByteCodeOffset() > instrBr->GetByteCodeOffset())
    {
        StackSym *symLd3 = instrLd3->GetDst()->AsRegOpnd()->m_sym;
        if (IR::Instr::FindRegUseInRange(symLd3, instrBr->m_next, instrLd3))
        {
            return nullptr;
        }
    }

    //
    // We have a match!
    //

    if(inlineeEndInstr2)
    {
        origBrFunc = instrBr2->m_func;
        origBrByteCodeOffset = instrBr2->GetByteCodeOffset();
        origBranchSrcSymId = instrBr2->GetSrc1()->GetStackSym()->m_id;
    }

    // Fix Ld_A
    if (brIsTrue)
    {
        instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd3->GetDst(), trueOpnd, instrBr->m_func);
        instrBr->InsertBefore(instrNew);
        instrNew->SetByteCodeOffset(instrBr);
        instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
    }
    else
    {
        instrNew = IR::Instr::New(Js::OpCode::Ld_A, instrLd3->GetDst(), falseOpnd, instrBr->m_func);
        instrBr->InsertBefore(instrNew);
        instrNew->SetByteCodeOffset(instrBr);
        instrNew->GetDst()->AsRegOpnd()->m_fgPeepTmp = true;
    }

    IR::LabelInstr *brTarget2;

    // Retarget branch
    if (brIsTrue2 == brIsTrue)
    {
        brTarget2 = instrBr2->AsBranchInstr()->GetTarget();
    }
    else
    {
        brTarget2 = IR::LabelInstr::New(Js::OpCode::Label, instrBr2->m_func);
        brTarget2->SetByteCodeOffset(instrBr2->m_next);
        instrBr2->InsertAfter(brTarget2);
    }

    instrBr->AsBranchInstr()->SetTarget(brTarget2);

    // InlineeEnd?
    if (inlineeEndInstr2)
    {
        this->InsertInlineeOnFLowEdge(instrBr->AsBranchInstr(), inlineeEndInstr2, instrByteCode, origBrFunc, origBrByteCodeOffset, origBranchSrcOpndIsJITOpt, origBranchSrcSymId);
    }

    return instrBr;
}

void
FlowGraph::InsertInlineeOnFLowEdge(IR::BranchInstr *instrBr, IR::Instr *inlineeEndInstr, IR::Instr *instrBytecode, Func* origBrFunc, uint32 origByteCodeOffset, bool origBranchSrcOpndIsJITOpt, uint32 origBranchSrcSymId)
{
    // Helper for PeepsCm code.
    //
    // We've skipped some InlineeEnd.  Globopt expects to see these
    // on all flow paths out of the inlinee.  Insert an InlineeEnd
    // on the new path:
    //      BrEq $L1, a, b
    // Becomes:
    //      BrNeq $L2, a, b
    //      InlineeEnd
    //      Br $L1
    //  L2:

    instrBr->AsBranchInstr()->Invert();

    IR::BranchInstr *newBr = IR::BranchInstr::New(Js::OpCode::Br, instrBr->AsBranchInstr()->GetTarget(), origBrFunc);
    newBr->SetByteCodeOffset(origByteCodeOffset);
    instrBr->InsertAfter(newBr);

    IR::LabelInstr *newLabel = IR::LabelInstr::New(Js::OpCode::Label, instrBr->m_func);
    newLabel->SetByteCodeOffset(instrBytecode);
    newBr->InsertAfter(newLabel);
    instrBr->AsBranchInstr()->SetTarget(newLabel);

    IR::Instr *newInlineeEnd = IR::Instr::New(Js::OpCode::InlineeEnd, inlineeEndInstr->m_func);
    newInlineeEnd->SetSrc1(inlineeEndInstr->GetSrc1());
    newInlineeEnd->SetSrc2(inlineeEndInstr->GetSrc2());
    newInlineeEnd->SetByteCodeOffset(instrBytecode);
    newInlineeEnd->SetIsCloned(true);  // Mark it as cloned - this is used later by the inlinee args optimization
    newBr->InsertBefore(newInlineeEnd);

    IR::ByteCodeUsesInstr * useOrigBranchSrcInstr = IR::ByteCodeUsesInstr::New(origBrFunc, origByteCodeOffset);
    useOrigBranchSrcInstr->SetRemovedOpndSymbol(origBranchSrcOpndIsJITOpt, origBranchSrcSymId);
    newBr->InsertBefore(useOrigBranchSrcInstr);

    uint newBrFnNumber = newBr->m_func->GetFunctionNumber();
    Assert(newBrFnNumber == origBrFunc->GetFunctionNumber());

    // The function numbers of the new branch and the inlineeEnd instruction should be different (ensuring that the new branch is not added in the inlinee but in the inliner).
    // Only case when they can be same is recursive calls - inlinee and inliner are the same function
    Assert(newBrFnNumber != inlineeEndInstr->m_func->GetFunctionNumber() ||
        newBrFnNumber == inlineeEndInstr->m_func->GetParentFunc()->GetFunctionNumber());
}

BasicBlock *
BasicBlock::New(FlowGraph * graph)
{
    BasicBlock * block;

    block = JitAnew(graph->alloc, BasicBlock, graph->alloc, graph->GetFunc());

    return block;
}

void
BasicBlock::AddPred(FlowEdge * edge, FlowGraph * graph)
{
    this->predList.Prepend(graph->alloc, edge);
}

void
BasicBlock::AddSucc(FlowEdge * edge, FlowGraph * graph)
{
    this->succList.Prepend(graph->alloc, edge);
}

void
BasicBlock::RemovePred(BasicBlock *block, FlowGraph * graph)
{
    this->RemovePred(block, graph, true, false);
}

void
BasicBlock::RemoveSucc(BasicBlock *block, FlowGraph * graph)
{
    this->RemoveSucc(block, graph, true, false);
}

void
BasicBlock::RemoveDeadPred(BasicBlock *block, FlowGraph * graph)
{
    this->RemovePred(block, graph, true, true);
}

void
BasicBlock::RemoveDeadSucc(BasicBlock *block, FlowGraph * graph)
{
    this->RemoveSucc(block, graph, true, true);
}

void
BasicBlock::RemovePred(BasicBlock *block, FlowGraph * graph, bool doCleanSucc, bool moveToDead)
{
    FOREACH_SLISTBASECOUNTED_ENTRY_EDITING(FlowEdge*, edge, this->GetPredList(), iter)
    {
        if (edge->GetPred() == block)
        {
            BasicBlock *blockSucc = edge->GetSucc();
            if (moveToDead)
            {
                iter.MoveCurrentTo(this->GetDeadPredList());

            }
            else
            {
                iter.RemoveCurrent(graph->alloc);
            }
            if (doCleanSucc)
            {
                block->RemoveSucc(this, graph, false, moveToDead);
            }
            if (blockSucc->isLoopHeader && blockSucc->loop && blockSucc->GetPredList()->HasOne())
            {
                Loop *loop = blockSucc->loop;
                loop->isDead = true;
            }
            return;
        }
    } NEXT_SLISTBASECOUNTED_ENTRY_EDITING;
    AssertMsg(UNREACHED, "Edge not found.");
}

void
BasicBlock::RemoveSucc(BasicBlock *block, FlowGraph * graph, bool doCleanPred, bool moveToDead)
{
    FOREACH_SLISTBASECOUNTED_ENTRY_EDITING(FlowEdge*, edge, this->GetSuccList(), iter)
    {
        if (edge->GetSucc() == block)
        {
            if (moveToDead)
            {
                iter.MoveCurrentTo(this->GetDeadSuccList());
            }
            else
            {
                iter.RemoveCurrent(graph->alloc);
            }

            if (doCleanPred)
            {
                block->RemovePred(this, graph, false, moveToDead);
            }

            if (block->isLoopHeader && block->loop && block->GetPredList()->HasOne())
            {
                Loop *loop = block->loop;
                loop->isDead = true;
            }
            return;
        }
    } NEXT_SLISTBASECOUNTED_ENTRY_EDITING;
    AssertMsg(UNREACHED, "Edge not found.");
}

void
BasicBlock::UnlinkPred(BasicBlock *block)
{
    this->UnlinkPred(block, true);
}

void
BasicBlock::UnlinkSucc(BasicBlock *block)
{
    this->UnlinkSucc(block, true);
}

void
BasicBlock::UnlinkPred(BasicBlock *block, bool doCleanSucc)
{
    FOREACH_SLISTBASECOUNTED_ENTRY_EDITING(FlowEdge*, edge, this->GetPredList(), iter)
    {
        if (edge->GetPred() == block)
        {
            iter.UnlinkCurrent();
            if (doCleanSucc)
            {
                block->UnlinkSucc(this, false);
            }
            return;
        }
    } NEXT_SLISTBASECOUNTED_ENTRY_EDITING;
    AssertMsg(UNREACHED, "Edge not found.");
}

void
BasicBlock::UnlinkSucc(BasicBlock *block, bool doCleanPred)
{
    FOREACH_SLISTBASECOUNTED_ENTRY_EDITING(FlowEdge*, edge, this->GetSuccList(), iter)
    {
        if (edge->GetSucc() == block)
        {
            iter.UnlinkCurrent();
            if (doCleanPred)
            {
                block->UnlinkPred(this, false);
            }
            return;
        }
    } NEXT_SLISTBASECOUNTED_ENTRY_EDITING;
    AssertMsg(UNREACHED, "Edge not found.");
}

bool
BasicBlock::IsLandingPad()
{
    BasicBlock * nextBlock = this->GetNext();
    return nextBlock && nextBlock->loop && nextBlock->isLoopHeader && nextBlock->loop->landingPad == this;
}

IR::Instr *
FlowGraph::RemoveInstr(IR::Instr *instr, GlobOpt * globOpt)
{
    IR::Instr *instrPrev = instr->m_prev;
    if (globOpt)
    {
        // Removing block during glob opt.  Need to maintain the graph so that
        // bailout will record the byte code use in case the dead code is exposed
        // by dyno-pogo optimization (where bailout need the byte code uses from
        // the dead blocks where it may not be dead after bailing out)
        if (instr->IsLabelInstr())
        {
            instr->AsLabelInstr()->m_isLoopTop = false;
            return instr;
        }
        else if (instr->IsByteCodeUsesInstr())
        {
            return instr;
        }

        /*
        *   Scope object has to be implicitly live whenever Heap Arguments object is live.
        *       - When we restore HeapArguments object in the bail out path, it expects the scope object also to be restored - if one was created.
        */
        Js::OpCode opcode = instr->m_opcode;
        if (opcode == Js::OpCode::LdElemI_A && instr->DoStackArgsOpt(this->func) &&
            globOpt->CurrentBlockData()->IsArgumentsOpnd(instr->GetSrc1()) && instr->m_func->GetScopeObjSym())
        {
            IR::ByteCodeUsesInstr * byteCodeUsesInstr = IR::ByteCodeUsesInstr::New(instr);
            byteCodeUsesInstr->SetNonOpndSymbol(instr->m_func->GetScopeObjSym()->m_id);
            instr->InsertAfter(byteCodeUsesInstr);
        }

        IR::ByteCodeUsesInstr * newByteCodeUseInstr = globOpt->ConvertToByteCodeUses(instr);
        if (newByteCodeUseInstr != nullptr)
        {
            // We don't care about property used in these instruction
            // It is only necessary for field copy prop so that we will keep the implicit call
            // up to the copy prop location.
            newByteCodeUseInstr->propertySymUse = nullptr;

            if (opcode == Js::OpCode::Yield)
            {
                IR::Instr *instrLabel = newByteCodeUseInstr->m_next;
                while (instrLabel->m_opcode != Js::OpCode::Label)
                {
                    instrLabel = instrLabel->m_next;
                }
                func->RemoveDeadYieldOffsetResumeLabel(instrLabel->AsLabelInstr());
                instrLabel->AsLabelInstr()->m_hasNonBranchRef = false;
            }

            // Save the last instruction to update the block with
            return newByteCodeUseInstr;
        }
        else
        {
            return instrPrev;
        }
    }
    else
    {
        instr->Remove();
        return instrPrev;
    }
}

void
FlowGraph::RemoveBlock(BasicBlock *block, GlobOpt * globOpt, bool tailDuping)
{
    Assert(!block->isDead && !block->isDeleted);
    IR::Instr * lastInstr = nullptr;
    FOREACH_INSTR_IN_BLOCK_EDITING(instr, instrNext, block)
    {
        if (instr->m_opcode == Js::OpCode::FunctionExit)
        {
            // Removing FunctionExit causes problems downstream...
            // We could change the opcode, or have FunctionEpilog/FunctionExit to get
            // rid of the epilog.
            break;
        }
        if (instr == block->GetFirstInstr())
        {
            Assert(instr->IsLabelInstr());
            instr->AsLabelInstr()->m_isLoopTop = false;
        }
        else
        {
            lastInstr = this->RemoveInstr(instr, globOpt);
        }
    } NEXT_INSTR_IN_BLOCK_EDITING;

    if (lastInstr)
    {
        block->SetLastInstr(lastInstr);
    }
    FOREACH_SLISTBASECOUNTED_ENTRY(FlowEdge*, edge, block->GetPredList())
    {
        edge->GetPred()->RemoveSucc(block, this, false, globOpt != nullptr);
    } NEXT_SLISTBASECOUNTED_ENTRY;

    FOREACH_SLISTBASECOUNTED_ENTRY(FlowEdge*, edge, block->GetSuccList())
    {
        edge->GetSucc()->RemovePred(block, this, false, globOpt != nullptr);
    } NEXT_SLISTBASECOUNTED_ENTRY;

    if (block->isLoopHeader && this->loopList)
    {
        // If loop graph is built, remove loop from loopList
        Loop **pPrevLoop = &this->loopList;

        while (*pPrevLoop != block->loop)
        {
            pPrevLoop = &((*pPrevLoop)->next);
        }
        *pPrevLoop = (*pPrevLoop)->next;
        this->hasLoop = (this->loopList != nullptr);
    }
    if (globOpt != nullptr)
    {
        block->isDead = true;
        block->GetPredList()->MoveTo(block->GetDeadPredList());
        block->GetSuccList()->MoveTo(block->GetDeadSuccList());
    }
    if (tailDuping)
    {
        block->isDead = true;
    }
    block->isDeleted = true;
    block->SetDataUseCount(0);
}

void
BasicBlock::UnlinkInstr(IR::Instr * instr)
{
    Assert(this->Contains(instr));
    Assert(this->GetFirstInstr() != this->GetLastInstr());
    if (instr == this->GetFirstInstr())
    {
        Assert(!this->GetFirstInstr()->IsLabelInstr());
        this->SetFirstInstr(instr->m_next);
    }
    else if (instr == this->GetLastInstr())
    {
        this->SetLastInstr(instr->m_prev);
    }

    instr->Unlink();
}

void
BasicBlock::RemoveInstr(IR::Instr * instr)
{
    Assert(this->Contains(instr));
    if (instr == this->GetFirstInstr())
    {
        this->SetFirstInstr(instr->m_next);
    }
    else if (instr == this->GetLastInstr())
    {
        this->SetLastInstr(instr->m_prev);
    }

    instr->Remove();
}

void
BasicBlock::InsertInstrBefore(IR::Instr *newInstr, IR::Instr *beforeThisInstr)
{
    Assert(this->Contains(beforeThisInstr));
    beforeThisInstr->InsertBefore(newInstr);

    if(this->GetFirstInstr() == beforeThisInstr)
    {
        Assert(!beforeThisInstr->IsLabelInstr());
        this->SetFirstInstr(newInstr);
    }
}

void
BasicBlock::InsertInstrAfter(IR::Instr *newInstr, IR::Instr *afterThisInstr)
{
    Assert(this->Contains(afterThisInstr));
    afterThisInstr->InsertAfter(newInstr);

    if (this->GetLastInstr() == afterThisInstr)
    {
        Assert(afterThisInstr->HasFallThrough());
        this->SetLastInstr(newInstr);
    }
}

void
BasicBlock::InsertAfter(IR::Instr *newInstr)
{
    Assert(this->GetLastInstr()->HasFallThrough());
    this->GetLastInstr()->InsertAfter(newInstr);
    this->SetLastInstr(newInstr);
}

void
Loop::SetHasCall()
{
    Loop * current = this;
    do
    {
        if (current->hasCall)
        {
#if DBG
            current = current->parent;
            while (current)
            {
                Assert(current->hasCall);
                current = current->parent;
            }
#endif
            break;
        }
        current->hasCall = true;
        current = current->parent;
    }
    while (current != nullptr);
}

void
Loop::SetImplicitCallFlags(Js::ImplicitCallFlags newFlags)
{
    Loop * current = this;
    do
    {
        if ((current->implicitCallFlags & newFlags) == newFlags)
        {
#if DBG
            current = current->parent;
            while (current)
            {
                Assert((current->implicitCallFlags & newFlags) == newFlags);
                current = current->parent;
            }
#endif
            break;
        }
        newFlags = (Js::ImplicitCallFlags)(implicitCallFlags | newFlags);
        current->implicitCallFlags = newFlags;
        current = current->parent;
    }
    while (current != nullptr);
}

Js::ImplicitCallFlags
Loop::GetImplicitCallFlags()
{
    if (this->implicitCallFlags == Js::ImplicitCall_HasNoInfo)
    {
        if (this->parent == nullptr)
        {
            // We don't have any information, and we don't have any parent, so just assume that there aren't any implicit calls
            this->implicitCallFlags = Js::ImplicitCall_None;
        }
        else
        {
            // We don't have any information, get it from the parent and hope for the best
            this->implicitCallFlags = this->parent->GetImplicitCallFlags();
        }
    }
    return this->implicitCallFlags;
}

bool
Loop::CanDoFieldCopyProp()
{
#if DBG_DUMP
    if (((this->implicitCallFlags & ~(Js::ImplicitCall_External)) == 0) &&
        Js::Configuration::Global.flags.Trace.IsEnabled(Js::HostOptPhase))
    {
        Output::Print(_u("fieldcopyprop disabled because external: loop count: %d"), GetLoopNumber());
        GetFunc()->DumpFullFunctionName();
        Output::Print(_u("\n"));
        Output::Flush();
    }
#endif
    return GlobOpt::ImplicitCallFlagsAllowOpts(this);
}

bool
Loop::CanDoFieldHoist()
{
    // We can do field hoist wherever we can do copy prop
    return CanDoFieldCopyProp();
}

bool
Loop::CanHoistInvariants() const
{
    Func * func = this->GetHeadBlock()->firstInstr->m_func->GetTopFunc();

    if (PHASE_OFF(Js::InvariantsPhase, func))
    {
        return false;
    }

    return true;
}

IR::LabelInstr *
Loop::GetLoopTopInstr() const
{
    IR::LabelInstr * instr = nullptr;
    if (this->topFunc->isFlowGraphValid)
    {
        instr = this->GetHeadBlock()->GetFirstInstr()->AsLabelInstr();
    }
    else
    {
        // Flowgraph gets torn down after the globopt, so can't get the loopTop from the head block.
        instr = this->loopTopLabel;
    }
    if (instr)
    {
        Assert(instr->m_isLoopTop);
    }
    return instr;
}

void
Loop::SetLoopTopInstr(IR::LabelInstr * loopTop)
{
    this->loopTopLabel = loopTop;
}

#if DBG_DUMP
uint
Loop::GetLoopNumber() const
{
    IR::LabelInstr * loopTopInstr = this->GetLoopTopInstr();
    if (loopTopInstr->IsProfiledLabelInstr())
    {
        return loopTopInstr->AsProfiledLabelInstr()->loopNum;
    }
    return Js::LoopHeader::NoLoop;
}

bool
BasicBlock::Contains(IR::Instr * instr)
{
    FOREACH_INSTR_IN_BLOCK(blockInstr, this)
    {
        if (instr == blockInstr)
        {
            return true;
        }
    }
    NEXT_INSTR_IN_BLOCK;
    return false;
}
#endif

FlowEdge *
FlowEdge::New(FlowGraph * graph)
{
    FlowEdge * edge;

    edge = JitAnew(graph->alloc, FlowEdge);

    return edge;
}

bool
Loop::IsDescendentOrSelf(Loop const * loop) const
{
    Loop const * currentLoop = loop;
    while (currentLoop != nullptr)
    {
        if (currentLoop == this)
        {
            return true;
        }
        currentLoop = currentLoop->parent;
    }
    return false;
}

void FlowGraph::SafeRemoveInstr(IR::Instr *instr)
{
    BasicBlock *block;

    if (instr->m_next->IsLabelInstr())
    {
        block = instr->m_next->AsLabelInstr()->GetBasicBlock()->GetPrev();
        block->RemoveInstr(instr);
    }
    else if (instr->IsLabelInstr())
    {
        block = instr->AsLabelInstr()->GetBasicBlock();
        block->RemoveInstr(instr);
    }
    else
    {
        Assert(!instr->EndsBasicBlock() && !instr->StartsBasicBlock());
        instr->Remove();
    }
}

bool FlowGraph::IsUnsignedOpnd(IR::Opnd *src, IR::Opnd **pShrSrc1)
{
    // Look for an unsigned constant, or the result of an unsigned shift by zero
    if (!src->IsRegOpnd())
    {
        return false;
    }
    if (!src->AsRegOpnd()->m_sym->IsSingleDef())
    {
        return false;
    }

    if (src->AsRegOpnd()->m_sym->IsIntConst())
    {
        int32 intConst = src->AsRegOpnd()->m_sym->GetIntConstValue();

        if (intConst >= 0)
        {
            *pShrSrc1 = src;
            return true;
        }
        else
        {
            return false;
        }
    }

    IR::Instr * shrUInstr = src->AsRegOpnd()->m_sym->GetInstrDef();

    if (shrUInstr->m_opcode != Js::OpCode::ShrU_A)
    {
        return false;
    }

    IR::Opnd *shrCnt = shrUInstr->GetSrc2();

    if (!shrCnt->IsRegOpnd() || !shrCnt->AsRegOpnd()->m_sym->IsTaggableIntConst() || shrCnt->AsRegOpnd()->m_sym->GetIntConstValue() != 0)
    {
        return false;
    }

    IR::Opnd *shrSrc = shrUInstr->GetSrc1();

    *pShrSrc1 = shrSrc;
    return true;
}

bool FlowGraph::UnsignedCmpPeep(IR::Instr *cmpInstr)
{
    IR::Opnd *cmpSrc1 = cmpInstr->GetSrc1();
    IR::Opnd *cmpSrc2 = cmpInstr->GetSrc2();
    IR::Opnd *newSrc1;
    IR::Opnd *newSrc2;

    // Look for something like:
    //  t1 = ShrU_A x, 0
    //  t2 = 10;
    //  BrGt t1, t2, L
    //
    // Peep to:
    //
    //  t1 = ShrU_A x, 0
    //  t2 = 10;
    //  t3 = Or_A x, 0
    //  BrUnGt x, t3, L
    //       ByteCodeUse t1
    //
    // Hopefully dead-store can get rid of the ShrU

    if (!this->func->DoGlobOpt() || !GlobOpt::DoAggressiveIntTypeSpec(this->func) || !GlobOpt::DoLossyIntTypeSpec(this->func))
    {
        return false;
    }

    if (cmpInstr->IsBranchInstr() && !cmpInstr->AsBranchInstr()->IsConditional())
    {
        return false;
    }

    if (!cmpInstr->GetSrc2())
    {
        return false;
    }

    if (!this->IsUnsignedOpnd(cmpSrc1, &newSrc1))
    {
        return false;
    }
    if (!this->IsUnsignedOpnd(cmpSrc2, &newSrc2))
    {
        return false;
    }

    switch(cmpInstr->m_opcode)
    {
    case Js::OpCode::BrEq_A:
    case Js::OpCode::BrNeq_A:
    case Js::OpCode::BrSrEq_A:
    case Js::OpCode::BrSrNeq_A:
    case Js::OpCode::CmEq_A:
    case Js::OpCode::CmNeq_A:
    case Js::OpCode::CmSrEq_A:
    case Js::OpCode::CmSrNeq_A:
        break;
    case Js::OpCode::BrLe_A:
        cmpInstr->m_opcode = Js::OpCode::BrUnLe_A;
        break;
    case Js::OpCode::BrLt_A:
        cmpInstr->m_opcode = Js::OpCode::BrUnLt_A;
        break;
    case Js::OpCode::BrGe_A:
        cmpInstr->m_opcode = Js::OpCode::BrUnGe_A;
        break;
    case Js::OpCode::BrGt_A:
        cmpInstr->m_opcode = Js::OpCode::BrUnGt_A;
        break;
    case Js::OpCode::CmLe_A:
        cmpInstr->m_opcode = Js::OpCode::CmUnLe_A;
        break;
    case Js::OpCode::CmLt_A:
        cmpInstr->m_opcode = Js::OpCode::CmUnLt_A;
        break;
    case Js::OpCode::CmGe_A:
        cmpInstr->m_opcode = Js::OpCode::CmUnGe_A;
        break;
    case Js::OpCode::CmGt_A:
        cmpInstr->m_opcode = Js::OpCode::CmUnGt_A;
        break;

    default:
        return false;
    }

    IR::ByteCodeUsesInstr * bytecodeInstr = IR::ByteCodeUsesInstr::New(cmpInstr);
    cmpInstr->InsertAfter(bytecodeInstr);

    if (cmpSrc1 != newSrc1)
    {
        if (cmpSrc1->IsRegOpnd() && !cmpSrc1->GetIsJITOptimizedReg())
        {
            bytecodeInstr->Set(cmpSrc1);
        }

        IR::RegOpnd * unsignedSrc = IR::RegOpnd::New(newSrc1->GetType(), cmpInstr->m_func);
        IR::Instr * orZero = IR::Instr::New(Js::OpCode::Or_A, unsignedSrc, newSrc1, IR::IntConstOpnd::New(0, TyMachReg, cmpInstr->m_func), cmpInstr->m_func);
        orZero->SetByteCodeOffset(cmpInstr);
        cmpInstr->InsertBefore(orZero);
        cmpInstr->ReplaceSrc1(unsignedSrc);
        if (newSrc1->IsRegOpnd())
        {
            cmpInstr->GetSrc1()->AsRegOpnd()->SetIsJITOptimizedReg(true);
            orZero->GetSrc1()->SetIsJITOptimizedReg(true);
        }
    }
    if (cmpSrc2 != newSrc2)
    {
        if (cmpSrc2->IsRegOpnd() && !cmpSrc2->GetIsJITOptimizedReg())
        {
            bytecodeInstr->Set(cmpSrc2);
        }

        IR::RegOpnd * unsignedSrc = IR::RegOpnd::New(newSrc2->GetType(), cmpInstr->m_func);
        IR::Instr * orZero = IR::Instr::New(Js::OpCode::Or_A, unsignedSrc, newSrc2, IR::IntConstOpnd::New(0, TyMachReg, cmpInstr->m_func), cmpInstr->m_func);
        orZero->SetByteCodeOffset(cmpInstr);
        cmpInstr->InsertBefore(orZero);
        cmpInstr->ReplaceSrc2(unsignedSrc);
        if (newSrc2->IsRegOpnd())
        {
            cmpInstr->GetSrc2()->AsRegOpnd()->SetIsJITOptimizedReg(true);
            orZero->GetSrc1()->SetIsJITOptimizedReg(true);
        }
    }

    return true;
}


#if DBG

void
FlowGraph::VerifyLoopGraph()
{
    FOREACH_BLOCK(block, this)
    {
        Loop *loop = block->loop;
        FOREACH_SUCCESSOR_BLOCK(succ, block)
        {
            if (loop == succ->loop)
            {
                Assert(succ->isLoopHeader == false || loop->GetHeadBlock() == succ);
                continue;
            }
            if (succ->isLoopHeader)
            {
                Assert(succ->loop->parent == loop
                    || (!loop->IsDescendentOrSelf(succ->loop)));
                continue;
            }
            Assert(succ->loop == nullptr || succ->loop->IsDescendentOrSelf(loop));
        } NEXT_SUCCESSOR_BLOCK;

        if (!PHASE_OFF(Js::RemoveBreakBlockPhase, this->GetFunc()))
        {
            // Make sure all break blocks have been removed.
            if (loop && !block->isLoopHeader && !(this->func->HasTry() && !this->func->DoOptimizeTry()))
            {
                Assert(loop->IsDescendentOrSelf(block->GetPrev()->loop));
            }
        }
    } NEXT_BLOCK;
}

#endif

#if DBG_DUMP

void
FlowGraph::Dump(bool onlyOnVerboseMode, const char16 *form)
{
    if(PHASE_DUMP(Js::FGBuildPhase, this->GetFunc()))
    {
        if (!onlyOnVerboseMode || Js::Configuration::Global.flags.Verbose)
        {
            if (form)
            {
                Output::Print(form);
            }
            this->Dump();
        }
    }
}

void
FlowGraph::Dump()
{
    Output::Print(_u("\nFlowGraph\n"));
    FOREACH_BLOCK(block, this)
    {
        Loop * loop = block->loop;
        while (loop)
        {
            Output::Print(_u("    "));
            loop = loop->parent;
        }
        block->DumpHeader(false);
    } NEXT_BLOCK;

    Output::Print(_u("\nLoopGraph\n"));

    for (Loop *loop = this->loopList; loop; loop = loop->next)
    {
        Output::Print(_u("\nLoop\n"));
        FOREACH_BLOCK_IN_LOOP(block, loop)
        {
            block->DumpHeader(false);
        }NEXT_BLOCK_IN_LOOP;
        Output::Print(_u("Loop  Ends\n"));
    }
}

void
BasicBlock::DumpHeader(bool insertCR)
{
    if (insertCR)
    {
        Output::Print(_u("\n"));
    }
    Output::Print(_u("BLOCK %d:"), this->number);

    if (this->isDead)
    {
        Output::Print(_u(" **** DEAD ****"));
    }

    if (this->isBreakBlock)
    {
        Output::Print(_u(" **** Break Block ****"));
    }
    else if (this->isAirLockBlock)
    {
        Output::Print(_u(" **** Air lock Block ****"));
    }
    else if (this->isBreakCompensationBlockAtSource)
    {
        Output::Print(_u(" **** Break Source Compensation Code ****"));
    }
    else if (this->isBreakCompensationBlockAtSink)
    {
        Output::Print(_u(" **** Break Sink Compensation Code ****"));
    }
    else if (this->isAirLockCompensationBlock)
    {
        Output::Print(_u(" **** Airlock block Compensation Code ****"));
    }

    if (!this->predList.Empty())
    {
        BOOL fFirst = TRUE;
        Output::Print(_u(" In("));
        FOREACH_PREDECESSOR_BLOCK(blockPred, this)
        {
            if (!fFirst)
            {
                Output::Print(_u(", "));
            }
            Output::Print(_u("%d"), blockPred->GetBlockNum());
            fFirst = FALSE;
        }
        NEXT_PREDECESSOR_BLOCK;
        Output::Print(_u(")"));
    }


    if (!this->succList.Empty())
    {
        BOOL fFirst = TRUE;
        Output::Print(_u(" Out("));
        FOREACH_SUCCESSOR_BLOCK(blockSucc, this)
        {
            if (!fFirst)
            {
                Output::Print(_u(", "));
            }
            Output::Print(_u("%d"), blockSucc->GetBlockNum());
            fFirst = FALSE;
        }
        NEXT_SUCCESSOR_BLOCK;
        Output::Print(_u(")"));
    }

    if (!this->deadPredList.Empty())
    {
        BOOL fFirst = TRUE;
        Output::Print(_u(" DeadIn("));
        FOREACH_DEAD_PREDECESSOR_BLOCK(blockPred, this)
        {
            if (!fFirst)
            {
                Output::Print(_u(", "));
            }
            Output::Print(_u("%d"), blockPred->GetBlockNum());
            fFirst = FALSE;
        }
        NEXT_DEAD_PREDECESSOR_BLOCK;
        Output::Print(_u(")"));
    }

    if (!this->deadSuccList.Empty())
    {
        BOOL fFirst = TRUE;
        Output::Print(_u(" DeadOut("));
        FOREACH_DEAD_SUCCESSOR_BLOCK(blockSucc, this)
        {
            if (!fFirst)
            {
                Output::Print(_u(", "));
            }
            Output::Print(_u("%d"), blockSucc->GetBlockNum());
            fFirst = FALSE;
        }
        NEXT_DEAD_SUCCESSOR_BLOCK;
        Output::Print(_u(")"));
    }

    if (this->loop)
    {
        Output::Print(_u("   Loop(%d) header: %d"), this->loop->loopNumber, this->loop->GetHeadBlock()->GetBlockNum());

        if (this->loop->parent)
        {
            Output::Print(_u(" parent(%d): %d"), this->loop->parent->loopNumber, this->loop->parent->GetHeadBlock()->GetBlockNum());
        }

        if (this->loop->GetHeadBlock() == this)
        {
            Output::SkipToColumn(50);
            Output::Print(_u("Call Exp/Imp: "));
            if (this->loop->GetHasCall())
            {
                Output::Print(_u("yes/"));
            }
            else
            {
                Output::Print(_u(" no/"));
            }
            Output::Print(Js::DynamicProfileInfo::GetImplicitCallFlagsString(this->loop->GetImplicitCallFlags()));
        }
    }

    Output::Print(_u("\n"));
    if (insertCR)
    {
        Output::Print(_u("\n"));
    }
}

void
BasicBlock::Dump()
{
    // Dumping the first instruction (label) will dump the block header as well.
    FOREACH_INSTR_IN_BLOCK(instr, this)
    {
        instr->Dump();
    }
    NEXT_INSTR_IN_BLOCK;
}

void
AddPropertyCacheBucket::Dump() const
{
    Assert(this->initialType != nullptr);
    Assert(this->finalType != nullptr);
    Output::Print(_u(" initial type: 0x%x, final type: 0x%x "), this->initialType->GetAddr(), this->finalType->GetAddr());
}

void
ObjTypeGuardBucket::Dump() const
{
    Assert(this->guardedPropertyOps != nullptr);
    this->guardedPropertyOps->Dump();
}

void
ObjWriteGuardBucket::Dump() const
{
    Assert(this->writeGuards != nullptr);
    this->writeGuards->Dump();
}

#endif

void
BasicBlock::CleanUpValueMaps()
{
    // Don't do cleanup if it's been done recently.
    // Landing pad could get optimized twice...
    // We want the same info out the first and second time. So always cleanup.
    // Increasing the cleanup threshold count for asmjs to 500
    uint cleanupCount = (!this->globOptData.globOpt->GetIsAsmJSFunc()) ? CONFIG_FLAG(GoptCleanupThreshold) : CONFIG_FLAG(AsmGoptCleanupThreshold);
    if (!this->IsLandingPad() && this->globOptData.globOpt->instrCountSinceLastCleanUp < cleanupCount)
    {
        return;
    }
    this->globOptData.globOpt->instrCountSinceLastCleanUp = 0;

    JitArenaAllocator* tempAlloc = this->globOptData.globOpt->tempAlloc;

    GlobHashTable *thisTable = this->globOptData.symToValueMap;
    BVSparse<JitArenaAllocator> deadSymsBv(tempAlloc);
    BVSparse<JitArenaAllocator> keepAliveSymsBv(tempAlloc);
    BVSparse<JitArenaAllocator> availableValueNumbers(tempAlloc);
    availableValueNumbers.Copy(this->globOptData.globOpt->byteCodeConstantValueNumbersBv);
    BVSparse<JitArenaAllocator> *upwardExposedUses = this->upwardExposedUses;
    BVSparse<JitArenaAllocator> *upwardExposedFields = this->upwardExposedFields;
    bool isInLoop = !!this->loop;

    BVSparse<JitArenaAllocator> symsInCallSequence(tempAlloc);
    SListBase<IR::Opnd *> * callSequence = this->globOptData.callSequence;
    if (callSequence && !callSequence->Empty())
    {
        FOREACH_SLISTBASE_ENTRY(IR::Opnd *, opnd, callSequence)
        {
            StackSym * sym = opnd->GetStackSym();
            symsInCallSequence.Set(sym->m_id);
        }
    }
    NEXT_SLISTBASE_ENTRY;

    for (uint i = 0; i < thisTable->tableSize; i++)
    {
        FOREACH_SLISTBASE_ENTRY_EDITING(GlobHashBucket, bucket, &thisTable->table[i], iter)
        {
            bool isSymUpwardExposed = upwardExposedUses->Test(bucket.value->m_id) || upwardExposedFields->Test(bucket.value->m_id);
            if (!isSymUpwardExposed && symsInCallSequence.Test(bucket.value->m_id))
            {
                // Don't remove/shrink sym-value pair if the sym is referenced in callSequence even if the sym is dead according to backward data flow.
                // This is possible in some edge cases that an infinite loop is involved when evaluating parameter for a function (between StartCall and Call),
                // there is no backward data flow into the infinite loop block, but non empty callSequence still populates to it in this (forward) pass
                // which causes error when looking up value for the syms in callSequence (cannot find the value).
                // It would cause error to fill out the bailout information for the loop blocks.
                // Remove dead syms from callSequence has some risk because there are various associated counters which need to be consistent.
                continue;
            }
            // Make sure symbol was created before backward pass.
            // If symbols isn't upward exposed, mark it as dead.
            // If a symbol was copy-prop'd in a loop prepass, the upwardExposedUses info could be wrong.  So wait until we are out of the loop before clearing it.
            if ((SymID)bucket.value->m_id <= this->globOptData.globOpt->maxInitialSymID && !isSymUpwardExposed
                && (!isInLoop || !this->globOptData.globOpt->prePassCopyPropSym->Test(bucket.value->m_id)))
            {
                Value *val = bucket.element;
                ValueInfo *valueInfo = val->GetValueInfo();

                Sym * sym = bucket.value;
                Sym *symStore = valueInfo->GetSymStore();

                if (symStore && symStore == bucket.value)
                {
                    // Keep constants around, as we don't know if there will be further uses
                    if (!bucket.element->GetValueInfo()->IsVarConstant() && !bucket.element->GetValueInfo()->HasIntConstantValue())
                    {
                        // Symbol may still be a copy-prop candidate.  Wait before deleting it.
                        deadSymsBv.Set(bucket.value->m_id);

                        // Make sure the type sym is added to the dead syms vector as well, because type syms are
                        // created in backward pass and so their symIds > maxInitialSymID.
                        if (sym->IsStackSym() && sym->AsStackSym()->HasObjectTypeSym())
                        {
                            deadSymsBv.Set(sym->AsStackSym()->GetObjectTypeSym()->m_id);
                        }
                    }
                    availableValueNumbers.Set(val->GetValueNumber());
                }
                else
                {
                    // Make sure the type sym is added to the dead syms vector as well, because type syms are
                    // created in backward pass and so their symIds > maxInitialSymID. Perhaps we could remove
                    // it explicitly here, but would it work alright with the iterator?
                    if (sym->IsStackSym() && sym->AsStackSym()->HasObjectTypeSym())
                    {
                        deadSymsBv.Set(sym->AsStackSym()->GetObjectTypeSym()->m_id);
                    }

                    // Not a copy-prop candidate; delete it right away.
                    iter.RemoveCurrent(thisTable->alloc);
                    this->globOptData.liveInt32Syms->Clear(sym->m_id);
                    this->globOptData.liveLossyInt32Syms->Clear(sym->m_id);
                    this->globOptData.liveFloat64Syms->Clear(sym->m_id);
                }
            }
            else
            {
                Sym * sym = bucket.value;

                if (sym->IsPropertySym() && !this->globOptData.liveFields->Test(sym->m_id))
                {
                    // Remove propertySyms which are not live anymore.
                    iter.RemoveCurrent(thisTable->alloc);
                    this->globOptData.liveInt32Syms->Clear(sym->m_id);
                    this->globOptData.liveLossyInt32Syms->Clear(sym->m_id);
                    this->globOptData.liveFloat64Syms->Clear(sym->m_id);
                }
                else
                {
                    // Look at the copy-prop candidate. We don't want to get rid of the data for a symbol which is
                    // a copy-prop candidate.
                    Value *val = bucket.element;
                    ValueInfo *valueInfo = val->GetValueInfo();

                    Sym *symStore = valueInfo->GetSymStore();

                    if (symStore && symStore != bucket.value)
                    {
                        keepAliveSymsBv.Set(symStore->m_id);
                        if (symStore->IsStackSym() && symStore->AsStackSym()->HasObjectTypeSym())
                        {
                            keepAliveSymsBv.Set(symStore->AsStackSym()->GetObjectTypeSym()->m_id);
                        }
                    }
                    availableValueNumbers.Set(val->GetValueNumber());
                }
            }
        } NEXT_SLISTBASE_ENTRY_EDITING;
    }

    deadSymsBv.Minus(&keepAliveSymsBv);

    // Now cleanup exprToValueMap table
    ExprHashTable *thisExprTable = this->globOptData.exprToValueMap;
    bool oldHasCSECandidatesValue = this->globOptData.hasCSECandidates;  // Could be false if none need bailout.
    this->globOptData.hasCSECandidates = false;

    for (uint i = 0; i < thisExprTable->tableSize; i++)
    {
        FOREACH_SLISTBASE_ENTRY_EDITING(ExprHashBucket, bucket, &thisExprTable->table[i], iter)
        {
            ExprHash hash = bucket.value;
            ValueNumber src1ValNum = hash.GetSrc1ValueNumber();
            ValueNumber src2ValNum = hash.GetSrc2ValueNumber();

            // If src1Val or src2Val are not available anymore, no point keeping this CSE candidate
            bool removeCurrent = false;
            if ((src1ValNum && !availableValueNumbers.Test(src1ValNum))
                || (src2ValNum && !availableValueNumbers.Test(src2ValNum)))
            {
                removeCurrent = true;
            }
            else
            {
                // If we are keeping this value, make sure we also keep the symStore in the value table
                removeCurrent = true; // Remove by default, unless it's set to false later below.
                Value *val = bucket.element;
                if (val)
                {
                    Sym *symStore = val->GetValueInfo()->GetSymStore();
                    if (symStore)
                    {
                        Value *symStoreVal = this->globOptData.FindValue(symStore);

                        if (symStoreVal && symStoreVal->GetValueNumber() == val->GetValueNumber())
                        {
                            removeCurrent = false;
                            deadSymsBv.Clear(symStore->m_id);
                            if (symStore->IsStackSym() && symStore->AsStackSym()->HasObjectTypeSym())
                            {
                                deadSymsBv.Clear(symStore->AsStackSym()->GetObjectTypeSym()->m_id);
                            }
                        }
                    }
                }
            }

            if(removeCurrent)
            {
                iter.RemoveCurrent(thisExprTable->alloc);
            }
            else
            {
                this->globOptData.hasCSECandidates = oldHasCSECandidatesValue;
            }
        } NEXT_SLISTBASE_ENTRY_EDITING;
    }

    FOREACH_BITSET_IN_SPARSEBV(dead_id, &deadSymsBv)
    {
        thisTable->Clear(dead_id);
    }
    NEXT_BITSET_IN_SPARSEBV;

    if (!deadSymsBv.IsEmpty())
    {
        if (this->func->IsJitInDebugMode())
        {
            // Do not remove non-temp local vars from liveVarSyms (i.e. do not let them become dead).
            // We will need to restore all initialized/used so far non-temp local during bail out.
            // (See BackwardPass::ProcessBailOutInfo)
            Assert(this->func->m_nonTempLocalVars);
            BVSparse<JitArenaAllocator> tempBv(tempAlloc);
            tempBv.Minus(&deadSymsBv, this->func->m_nonTempLocalVars);
            this->globOptData.liveVarSyms->Minus(&tempBv);
#if DBG
            tempBv.And(this->globOptData.liveInt32Syms, this->func->m_nonTempLocalVars);
            AssertMsg(tempBv.IsEmpty(), "Type spec is disabled under debugger. How come did we get a non-temp local in liveInt32Syms?");
            tempBv.And(this->globOptData.liveLossyInt32Syms, this->func->m_nonTempLocalVars);
            AssertMsg(tempBv.IsEmpty(), "Type spec is disabled under debugger. How come did we get a non-temp local in liveLossyInt32Syms?");
            tempBv.And(this->globOptData.liveFloat64Syms, this->func->m_nonTempLocalVars);
            AssertMsg(tempBv.IsEmpty(), "Type spec is disabled under debugger. How come did we get a non-temp local in liveFloat64Syms?");
#endif
        }
        else
        {
            this->globOptData.liveVarSyms->Minus(&deadSymsBv);
        }

        this->globOptData.liveInt32Syms->Minus(&deadSymsBv);
        this->globOptData.liveLossyInt32Syms->Minus(&deadSymsBv);
        this->globOptData.liveFloat64Syms->Minus(&deadSymsBv);
    }

    JitAdelete(this->globOptData.globOpt->alloc, upwardExposedUses);
    this->upwardExposedUses = nullptr;
    JitAdelete(this->globOptData.globOpt->alloc, upwardExposedFields);
    this->upwardExposedFields = nullptr;
    if (this->cloneStrCandidates)
    {
        JitAdelete(this->globOptData.globOpt->alloc, this->cloneStrCandidates);
        this->cloneStrCandidates = nullptr;
    }
}

void
BasicBlock::MergePredBlocksValueMaps(GlobOpt* globOpt)
{
    Assert(!globOpt->isCallHelper);
    // We keep a local temporary copy for the merge
    GlobOptBlockData blockData(this->globOptData.curFunc);

    if (!globOpt->isRecursiveCallOnLandingPad)
    {
        blockData.NullOutBlockData(globOpt, globOpt->func);
    }
    else
    {
        // If we are going over the landing pad again after field PRE, just start again
        // with the value table where we left off.
        blockData.CopyBlockData(&this->globOptData);
        return;
    }

    BVSparse<JitArenaAllocator> symsRequiringCompensation(globOpt->tempAlloc);
    {
        BVSparse<JitArenaAllocator> symsCreatedForMerge(globOpt->tempAlloc);
        bool forceTypeSpecOnLoopHeader = true;
        FOREACH_PREDECESSOR_BLOCK(pred, this)
        {
            if (pred->globOptData.callSequence && pred->globOptData.callSequence->Empty())
            {
                JitAdelete(globOpt->alloc, pred->globOptData.callSequence);
                pred->globOptData.callSequence = nullptr;
            }
            if (this->isLoopHeader && globOpt->IsLoopPrePass() && globOpt->prePassLoop == this->loop && this->loop->IsDescendentOrSelf(pred->loop))
            {
                // Loop back-edge.
                // First pass on loop runs optimistically, without doing transforms.
                // Skip this edge for now.
                continue;
            }

            PathDependentInfo *const pathDependentInfo = __edge->GetPathDependentInfo();
            PathDependentInfoToRestore pathDependentInfoToRestore;
            if (pathDependentInfo)
            {
                pathDependentInfoToRestore = globOpt->UpdatePathDependentInfo(pathDependentInfo);
            }

            Assert(pred->GetDataUseCount());

            // First pred?
            if (blockData.symToValueMap == nullptr)
            {
                // Only one edge?
                if (pred->GetSuccList()->HasOne() && this->GetPredList()->HasOne() && this->loop == nullptr)
                {
                    blockData.ReuseBlockData(&pred->globOptData);

                    // Don't need to restore the old value info
                    pathDependentInfoToRestore.Clear();
                }
                else
                {
                    blockData.CloneBlockData(this, pred);
                }
            }
            else
            {
                const bool isLoopPrePass = globOpt->IsLoopPrePass();
                blockData.MergeBlockData(
                    this,
                    pred,
                    isLoopPrePass ? nullptr : &symsRequiringCompensation,
                    isLoopPrePass ? nullptr : &symsCreatedForMerge,
                    forceTypeSpecOnLoopHeader);
                forceTypeSpecOnLoopHeader = false; // can force type-spec on the loop header only for the first back edge.
            }

            // Restore the value for the next edge
            if (pathDependentInfo)
            {
                globOpt->RestorePathDependentInfo(pathDependentInfo, pathDependentInfoToRestore);
                __edge->ClearPathDependentInfo(globOpt->alloc);
            }

        } NEXT_PREDECESSOR_BLOCK;
    }

    // Consider: We can recreate values for hoisted field so it can copy prop out of the loop
    if (blockData.symToValueMap == nullptr)
    {
        Assert(blockData.hoistableFields == nullptr);
        blockData.InitBlockData(globOpt, globOpt->func);
    }
    else if (blockData.hoistableFields)
    {
        Assert(globOpt->TrackHoistableFields());
        blockData.hoistableFields->And(this->globOptData.liveFields);
    }

    if (!globOpt->DoObjTypeSpec())
    {
        // Object type specialization is off, but if copy prop is on (e.g., /force:fieldhoist) we're not clearing liveFields,
        // so we may be letting type syms slip through this block.
        globOpt->KillAllObjectTypes(this->globOptData.liveFields);
    }

    this->globOptData.CopyBlockData(&blockData);

    if (globOpt->IsLoopPrePass())
    {
        Assert(this->loop);

        if(globOpt->DoBoundCheckHoist())
        {
            globOpt->SetInductionVariableValueNumbers(&blockData);
        }

        if (this->isLoopHeader && globOpt->rootLoopPrePass == this->loop)
        {
            // Capture bail out info in case we have optimization that needs it
            Assert(this->loop->bailOutInfo == nullptr);
            IR::Instr * firstInstr = this->GetFirstInstr();
            this->loop->bailOutInfo = JitAnew(globOpt->func->m_alloc, BailOutInfo,
                firstInstr->GetByteCodeOffset(), firstInstr->m_func);
            globOpt->FillBailOutInfo(this, this->loop->bailOutInfo);
#if ENABLE_DEBUG_CONFIG_OPTIONS
            this->loop->bailOutInfo->bailOutOpcode = Js::OpCode::LoopBodyStart;
#endif
        }

        // If loop pre-pass, don't insert convert from type-spec to var
        return;
    }

    this->CleanUpValueMaps();
    Sym *symIV = nullptr;

    // Clean up the syms requiring compensation by checking the final value in the merged block to see if the sym still requires
    // compensation. All the while, create a mapping from sym to value info in the merged block. This dictionary helps avoid a
    // value lookup in the merged block per predecessor.
    SymToValueInfoMap symsRequiringCompensationToMergedValueInfoMap(globOpt->tempAlloc);
    if(!symsRequiringCompensation.IsEmpty())
    {
        const SymTable *const symTable = func->m_symTable;
        FOREACH_BITSET_IN_SPARSEBV(id, &symsRequiringCompensation)
        {
            Sym *const sym = symTable->Find(id);
            Assert(sym);

            Value *const value = blockData.FindValue(sym);
            if(!value)
            {
                continue;
            }

            ValueInfo *const valueInfo = value->GetValueInfo();
            if(!valueInfo->IsArrayValueInfo())
            {
                continue;
            }

            // At least one new sym was created while merging and associated with the merged value info, so those syms will
            // require compensation in predecessors. For now, the dead store phase is relied upon to remove compensation that is
            // dead due to no further uses of the new sym.
            symsRequiringCompensationToMergedValueInfoMap.Add(sym, valueInfo);
        } NEXT_BITSET_IN_SPARSEBV;
        symsRequiringCompensation.ClearAll();
    }

    if (this->isLoopHeader)
    {
        BVSparse<JitArenaAllocator> * tempBv = globOpt->tempBv;
        // Values on the back-edge in the prepass may be conservative for syms defined in the loop, and type specialization in
        // the prepass is not reflective of the value, but rather, is used to determine whether the sym should be specialized
        // around the loop. Additionally, some syms that are used before defined in the loop may be specialized in the loop
        // header despite not being specialized in the landing pad. Now that the type specialization bit-vectors are merged,
        // specialize the corresponding value infos in the loop header too.

        Assert(tempBv->IsEmpty());
        Loop *const loop = this->loop;
        SymTable *const symTable = func->m_symTable;
        JitArenaAllocator *const alloc = globOpt->alloc;

        // Int-specialized syms
        tempBv->Or(loop->likelyIntSymsUsedBeforeDefined, loop->symsDefInLoop);
        tempBv->And(blockData.liveInt32Syms);
        tempBv->Minus(blockData.liveLossyInt32Syms);
        FOREACH_BITSET_IN_SPARSEBV(id, tempBv)
        {
            StackSym *const varSym = symTable->FindStackSym(id);
            Assert(varSym);
            Value *const value = blockData.FindValue(varSym);
            Assert(value);
            ValueInfo *const valueInfo = value->GetValueInfo();
            if(!valueInfo->IsInt())
            {
                globOpt->ChangeValueInfo(nullptr, value, valueInfo->SpecializeToInt32(alloc));
            }
        } NEXT_BITSET_IN_SPARSEBV;

        // Float-specialized syms
        tempBv->Or(loop->likelyNumberSymsUsedBeforeDefined, loop->symsDefInLoop);
        tempBv->Or(loop->forceFloat64SymsOnEntry);
        tempBv->And(blockData.liveFloat64Syms);
        GlobOptBlockData &landingPadBlockData = loop->landingPad->globOptData;
        FOREACH_BITSET_IN_SPARSEBV(id, tempBv)
        {
            StackSym *const varSym = symTable->FindStackSym(id);
            Assert(varSym);

            // If the type-spec sym is null or if the sym is not float-specialized in the loop landing pad, the sym may have
            // been merged to float on a loop back-edge when it was live as float on the back-edge, and live as int in the loop
            // header. In this case, compensation inserted in the loop landing pad will use BailOutNumberOnly, and so it is
            // guaranteed that the value will be float. Otherwise, if the type-spec sym exists, its field can be checked to see
            // if it's prevented from being anything but a number.
            StackSym *const typeSpecSym = varSym->GetFloat64EquivSym(nullptr);
            if(!typeSpecSym ||
                typeSpecSym->m_requiresBailOnNotNumber ||
                !landingPadBlockData.IsFloat64TypeSpecialized(varSym))
            {
                Value *const value = blockData.FindValue(varSym);
                if(value)
                {
                    ValueInfo *const valueInfo = value->GetValueInfo();
                    if(!valueInfo->IsNumber())
                    {
                        globOpt->ChangeValueInfo(this, value, valueInfo->SpecializeToFloat64(alloc));
                    }
                }
                else
                {
                    this->globOptData.SetValue(globOpt->NewGenericValue(ValueType::Float), varSym);
                }
            }
        } NEXT_BITSET_IN_SPARSEBV;

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        // Simd128 type-spec syms
        BVSparse<JitArenaAllocator> tempBv2(globOpt->tempAlloc);

        // For syms we made alive in loop header because of hoisting, use-before-def, or def in Loop body, set their valueInfo to definite.
        // Make live on header AND in one of forceSimd128* or likelySimd128* vectors.
        tempBv->Or(loop->likelySimd128F4SymsUsedBeforeDefined, loop->symsDefInLoop);
        tempBv->Or(loop->likelySimd128I4SymsUsedBeforeDefined);
        tempBv->Or(loop->forceSimd128F4SymsOnEntry);
        tempBv->Or(loop->forceSimd128I4SymsOnEntry);
        tempBv2.Or(blockData.liveSimd128F4Syms, blockData.liveSimd128I4Syms);
        tempBv->And(&tempBv2);

        FOREACH_BITSET_IN_SPARSEBV(id, tempBv)
        {
            StackSym * typeSpecSym = nullptr;
            StackSym *const varSym = symTable->FindStackSym(id);
            Assert(varSym);

            if (blockData.liveSimd128F4Syms->Test(id))
            {
                typeSpecSym = varSym->GetSimd128F4EquivSym(nullptr);


                if (!typeSpecSym || !landingPadBlockData.IsSimd128F4TypeSpecialized(varSym))
                {
                    Value *const value = blockData.FindValue(varSym);
                    if (value)
                    {
                        ValueInfo *const valueInfo = value->GetValueInfo();
                        if (!valueInfo->IsSimd128Float32x4())
                        {
                            globOpt->ChangeValueInfo(this, value, valueInfo->SpecializeToSimd128F4(alloc));
                        }
                    }
                    else
                    {
                        this->globOptData.SetValue(globOpt->NewGenericValue(ValueType::GetSimd128(ObjectType::Simd128Float32x4), varSym), varSym);
                    }
                }
            }
            else if (blockData.liveSimd128I4Syms->Test(id))
            {

                typeSpecSym = varSym->GetSimd128I4EquivSym(nullptr);
                if (!typeSpecSym || !landingPadBlockData.IsSimd128I4TypeSpecialized(varSym))
                {
                    Value *const value = blockData.FindValue(varSym);
                    if (value)
                    {
                        ValueInfo *const valueInfo = value->GetValueInfo();
                        if (!valueInfo->IsSimd128Int32x4())
                        {
                            globOpt->ChangeValueInfo(this, value, valueInfo->SpecializeToSimd128I4(alloc));
                        }
                    }
                    else
                    {
                        this->globOptData.SetValue(globOpt->NewGenericValue(ValueType::GetSimd128(ObjectType::Simd128Int32x4), varSym), varSym);
                    }
                }
            }
            else
            {
                Assert(UNREACHED);
            }
        } NEXT_BITSET_IN_SPARSEBV;
#endif

        tempBv->ClearAll();
    }

    // We need to handle the case where a symbol is type-spec'd coming from some predecessors,
    // but not from others.
    //
    // We can do this by inserting the right conversion in the predecessor block, but we
    // can only do this if we are the first successor of that block, since the previous successors
    // would have already been processed.  Instead, we'll need to break the edge and insert a block
    // (airlock block) to put in the conversion code.
    Assert(globOpt->tempBv->IsEmpty());

    BVSparse<JitArenaAllocator> tempBv2(globOpt->tempAlloc);
    BVSparse<JitArenaAllocator> tempBv3(globOpt->tempAlloc);
    BVSparse<JitArenaAllocator> tempBv4(globOpt->tempAlloc);

#ifdef ENABLE_SIMDJS
    // SIMD_JS
    BVSparse<JitArenaAllocator> simd128F4SymsToUnbox(globOpt->tempAlloc);
    BVSparse<JitArenaAllocator> simd128I4SymsToUnbox(globOpt->tempAlloc);
#endif

    FOREACH_PREDECESSOR_EDGE_EDITING(edge, this, iter)
    {
        BasicBlock *pred = edge->GetPred();

        if (pred->loop && pred->loop->GetHeadBlock() == this)
        {
            pred->DecrementDataUseCount();
            // Skip loop back-edges. We will handle these when we get to the exit blocks.
            continue;
        }

        BasicBlock *orgPred = nullptr;
        if (pred->isAirLockCompensationBlock)
        {
            Assert(pred->GetPredList()->HasOne());
            orgPred = pred;
            pred = (pred->GetPredList()->Head())->GetPred();
        }

        // Lossy int in the merged block, and no int in the predecessor - need a lossy conversion to int
        tempBv2.Minus(blockData.liveLossyInt32Syms, pred->globOptData.liveInt32Syms);

        // Lossless int in the merged block, and no lossless int in the predecessor - need a lossless conversion to int
        tempBv3.Minus(blockData.liveInt32Syms, this->globOptData.liveLossyInt32Syms);
        globOpt->tempBv->Minus(pred->globOptData.liveInt32Syms, pred->globOptData.liveLossyInt32Syms);
        tempBv3.Minus(globOpt->tempBv);

        globOpt->tempBv->Minus(blockData.liveVarSyms, pred->globOptData.liveVarSyms);
        tempBv4.Minus(blockData.liveFloat64Syms, pred->globOptData.liveFloat64Syms);

        bool symIVNeedsSpecializing = (symIV && !pred->globOptData.liveInt32Syms->Test(symIV->m_id) && !tempBv3.Test(symIV->m_id));

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        simd128F4SymsToUnbox.Minus(blockData.liveSimd128F4Syms, pred->globOptData.liveSimd128F4Syms);
        simd128I4SymsToUnbox.Minus(blockData.liveSimd128I4Syms, pred->globOptData.liveSimd128I4Syms);
#endif

        if (!globOpt->tempBv->IsEmpty() ||
            !tempBv2.IsEmpty() ||
            !tempBv3.IsEmpty() ||
            !tempBv4.IsEmpty() ||
#ifdef ENABLE_SIMDJS
            !simd128F4SymsToUnbox.IsEmpty() ||
            !simd128I4SymsToUnbox.IsEmpty() ||
#endif
            symIVNeedsSpecializing ||
            symsRequiringCompensationToMergedValueInfoMap.Count() != 0)
        {
            // We can't un-specialize a symbol in a predecessor if we've already processed
            // a successor of that block. Instead, insert a new block on the flow edge
            // (an airlock block) and do the un-specialization there.
            //
            // Alternatively, the current block could be an exit block out of this loop, and so the predecessor may exit the
            // loop. In that case, if the predecessor may continue into the loop without exiting, then we need an airlock block
            // to do the appropriate conversions only on the exit path (preferring not to do the conversions inside the loop).
            // If, on the other hand, the predecessor always flows into the current block, then it always exits, so we don't need
            // an airlock block and can just do the conversions in the predecessor.
            if (pred->GetSuccList()->Head()->GetSucc() != this||
                (pred->loop && pred->loop->parent == this->loop && pred->GetSuccList()->Count() > 1))
            {
                BasicBlock *airlockBlock = nullptr;
                if (!orgPred)
                {
                    if (PHASE_TRACE(Js::GlobOptPhase, this->func) && !globOpt->IsLoopPrePass())
                    {
                        Output::Print(_u("TRACE: "));
                        Output::Print(_u("Inserting airlock block to convert syms to var between block %d and %d\n"),
                            pred->GetBlockNum(), this->GetBlockNum());
                        Output::Flush();
                    }
                    airlockBlock = globOpt->func->m_fg->InsertAirlockBlock(edge);
                }
                else
                {
                    Assert(orgPred->isAirLockCompensationBlock);
                    airlockBlock = orgPred;
                    pred->DecrementDataUseCount();
                    airlockBlock->isAirLockCompensationBlock = false; // This is airlock block now. So remove the attribute.
                }
                globOpt->CloneBlockData(airlockBlock, pred);

                pred = airlockBlock;
            }
            if (!globOpt->tempBv->IsEmpty())
            {
                globOpt->ToVar(globOpt->tempBv, pred);
            }
            if (!tempBv2.IsEmpty())
            {
                globOpt->ToInt32(&tempBv2, pred, true /* lossy */);
            }
            if (!tempBv3.IsEmpty())
            {
                globOpt->ToInt32(&tempBv3, pred, false /* lossy */);
            }
            if (!tempBv4.IsEmpty())
            {
                globOpt->ToFloat64(&tempBv4, pred);
            }
            if (symIVNeedsSpecializing)
            {
                globOpt->tempBv->ClearAll();
                globOpt->tempBv->Set(symIV->m_id);
                globOpt->ToInt32(globOpt->tempBv, pred, false /* lossy */);
            }
            if(symsRequiringCompensationToMergedValueInfoMap.Count() != 0)
            {
                globOpt->InsertValueCompensation(pred, symsRequiringCompensationToMergedValueInfoMap);
            }

#ifdef ENABLE_SIMDJS
            // SIMD_JS
            if (!simd128F4SymsToUnbox.IsEmpty())
            {
                globOpt->ToTypeSpec(&simd128F4SymsToUnbox, pred, TySimd128F4, IR::BailOutSimd128F4Only);
            }

            if (!simd128I4SymsToUnbox.IsEmpty())
            {
                globOpt->ToTypeSpec(&simd128I4SymsToUnbox, pred, TySimd128I4, IR::BailOutSimd128I4Only);
            }
#endif
        }
    } NEXT_PREDECESSOR_EDGE_EDITING;

    FOREACH_PREDECESSOR_EDGE(edge, this)
    {
        // Peak Memory optimization:
        // These are in an arena, but putting them on the free list greatly reduces
        // the peak memory used by the global optimizer for complex flow graphs.
        BasicBlock *pred = edge->GetPred();
        if (!this->isLoopHeader || this->loop != pred->loop)
        {
            // Skip airlock compensation block as we are not going to walk this block.
            if (pred->isAirLockCompensationBlock)
            {
                pred->DecrementDataUseCount();
                Assert(pred->GetPredList()->HasOne());
                pred = (pred->GetPredList()->Head())->GetPred();
            }

            if (pred->DecrementDataUseCount() == 0 && (!this->loop || this->loop->landingPad != pred))
            {
                if (!(pred->GetSuccList()->HasOne() && this->GetPredList()->HasOne() && this->loop == nullptr))
                {
                    pred->globOptData.DeleteBlockData();
                }
                else
                {
                    pred->globOptData.NullOutBlockData(globOpt, globOpt->func);
                }
            }
        }
    } NEXT_PREDECESSOR_EDGE;

    globOpt->tempBv->ClearAll();
    Assert(!globOpt->IsLoopPrePass());   // We already early return if we are in prepass

    if (this->isLoopHeader)
    {
        Loop *const loop = this->loop;

        // Save values live on loop entry, such that we can adjust the state of the
        // values on the back-edge to match.
        loop->varSymsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->varSymsOnEntry->Copy(this->globOptData.liveVarSyms);

        loop->int32SymsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->int32SymsOnEntry->Copy(this->globOptData.liveInt32Syms);

        loop->lossyInt32SymsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->lossyInt32SymsOnEntry->Copy(this->globOptData.liveLossyInt32Syms);

        loop->float64SymsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->float64SymsOnEntry->Copy(this->globOptData.liveFloat64Syms);

#ifdef ENABLE_SIMDJS
        // SIMD_JS
        loop->simd128F4SymsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->simd128F4SymsOnEntry->Copy(this->globOptData.liveSimd128F4Syms);

        loop->simd128I4SymsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->simd128I4SymsOnEntry->Copy(this->globOptData.liveSimd128I4Syms);
#endif

        loop->liveFieldsOnEntry = JitAnew(globOpt->alloc, BVSparse<JitArenaAllocator>, globOpt->alloc);
        loop->liveFieldsOnEntry->Copy(this->globOptData.liveFields);

        if(globOpt->DoBoundCheckHoist() && loop->inductionVariables)
        {
            globOpt->FinalizeInductionVariables(loop, &blockData);
            if(globOpt->DoLoopCountBasedBoundCheckHoist())
            {
                globOpt->DetermineDominatingLoopCountableBlock(loop, this);
            }
        }
    }
    else if (!this->loop)
    {
        this->SetDataUseCount(this->GetSuccList()->Count());
    }
    else if(this == this->loop->dominatingLoopCountableBlock)
    {
        globOpt->DetermineLoopCount(this->loop);
    }
}

void GlobOpt::CloneBlockData(BasicBlock *const toBlock, BasicBlock *const fromBlock)
{
    toBlock->globOptData.CloneBlockData(toBlock, fromBlock);
}

void
GlobOpt::CloneValues(BasicBlock *const toBlock, GlobOptBlockData *toData, GlobOptBlockData *fromData)
{
    ValueSet *const valuesToKillOnCalls = JitAnew(this->alloc, ValueSet, this->alloc);
    toData->valuesToKillOnCalls = valuesToKillOnCalls;

    // Values are shared between symbols with the same ValueNumber.
    // Use a dictionary to share the clone values.
    ValueSetByValueNumber *const valuesCreatedForClone = this->valuesCreatedForClone;
    Assert(valuesCreatedForClone);
    Assert(valuesCreatedForClone->Count() == 0);
    DebugOnly(ValueSetByValueNumber originalValues(tempAlloc, 64));

    const uint tableSize = toData->symToValueMap->tableSize;
    SListBase<GlobHashBucket> *const table = toData->symToValueMap->table;
    for (uint i = 0; i < tableSize; i++)
    {
        FOREACH_SLISTBASE_ENTRY(GlobHashBucket, bucket, &table[i])
        {
            Value *value = bucket.element;
            ValueNumber valueNum = value->GetValueNumber();
#if DBG
            // Ensure that the set of values in fromData contains only one value per value number. Byte-code constant values
            // are reused in multiple blocks without cloning, so exclude those value numbers.
            {
                Value *const previouslyClonedOriginalValue = originalValues.Lookup(valueNum);
                if (previouslyClonedOriginalValue)
                {
                    if (!byteCodeConstantValueNumbersBv->Test(valueNum))
                    {
                        Assert(value == previouslyClonedOriginalValue);
                    }
                }
                else
                {
                    originalValues.Add(value);
                }
            }
#endif

            Value *newValue = valuesCreatedForClone->Lookup(valueNum);
            if (!newValue)
            {
                newValue = CopyValue(value, valueNum);
                TrackMergedValueForKills(newValue, toData, nullptr);
                valuesCreatedForClone->Add(newValue);
            }
            bucket.element = newValue;
        } NEXT_SLISTBASE_ENTRY;
    }

    valuesCreatedForClone->Clear();

    ProcessValueKills(toBlock, toData);
}

PRECandidatesList * GlobOpt::FindBackEdgePRECandidates(BasicBlock *block, JitArenaAllocator *alloc)
{
    // Iterate over the value table looking for propertySyms which are candidates to
    // pre-load in the landing pad for field PRE

    GlobHashTable *valueTable = block->globOptData.symToValueMap;
    Loop *loop = block->loop;
    PRECandidatesList *candidates = nullptr;

    for (uint i = 0; i < valueTable->tableSize; i++)
    {
        FOREACH_SLISTBASE_ENTRY(GlobHashBucket, bucket, &valueTable->table[i])
        {
            Sym *sym = bucket.value;

            if (!sym->IsPropertySym())
            {
                continue;
            }

            PropertySym *propertySym = sym->AsPropertySym();

            // Field should be live on the back-edge
            if (!block->globOptData.liveFields->Test(propertySym->m_id))
            {
                continue;
            }

            // Field should be live in the landing pad as well
            if (!loop->landingPad->globOptData.liveFields->Test(propertySym->m_id))
            {
                continue;
            }

            Value *value = bucket.element;
            Sym *symStore = value->GetValueInfo()->GetSymStore();

            if (!symStore || !symStore->IsStackSym())
            {
                continue;
            }

            // Check upwardExposed in case of:
            //  s1 = 0;
            // loop:
            //        = o.x;
            //        foo();
            //    o.x = s1;
            // Can't thrash s1 in loop top.
            if (!symStore->AsStackSym()->IsSingleDef() || loop->GetHeadBlock()->upwardExposedUses->Test(symStore->m_id))
            {
                // If symStore isn't singleDef, we need to make sure it still has the same value.
                // This usually fails if we are not aggressive at transferring values in the prepass.
                Value **pSymStoreFromValue = valueTable->Get(symStore->m_id);

                // Consider: We should be fine if symStore isn't live in landing pad...
                if (!pSymStoreFromValue || (*pSymStoreFromValue)->GetValueNumber() != value->GetValueNumber())
                {
                    continue;
                }
            }

            BasicBlock *landingPad = loop->landingPad;
            Value *landingPadValue = landingPad->globOptData.FindValue(propertySym);

            if (!landingPadValue)
            {
                // Value should be added as initial value or already be there.
                return nullptr;
            }

            IR::Instr * ldInstr = this->prePassInstrMap->Lookup(propertySym->m_id, nullptr);

            if (!ldInstr)
            {
                continue;
            }

            if (!candidates)
            {
                candidates = Anew(alloc, PRECandidatesList, alloc);
            }

            candidates->Prepend(&bucket);

        } NEXT_SLISTBASE_ENTRY;
    }

    return candidates;
}

void GlobOpt::InsertCloneStrs(BasicBlock *toBlock, GlobOptBlockData *toData, GlobOptBlockData *fromData)
{
    if (toBlock->isLoopHeader   // isLoopBackEdge
        && toBlock->cloneStrCandidates
        && !IsLoopPrePass())
    {
        Loop *loop = toBlock->loop;
        BasicBlock *landingPad = loop->landingPad;
        const SymTable *const symTable = func->m_symTable;
        Assert(tempBv->IsEmpty());
        tempBv->And(toBlock->cloneStrCandidates, fromData->isTempSrc);
        FOREACH_BITSET_IN_SPARSEBV(id, tempBv)
        {
            StackSym *const sym = (StackSym *)symTable->Find(id);
            Assert(sym);

            if (!landingPad->globOptData.liveVarSyms->Test(id)
                || !fromData->liveVarSyms->Test(id))
            {
                continue;
            }

            Value * landingPadValue = landingPad->globOptData.FindValue(sym);
            if (landingPadValue == nullptr)
            {
                continue;
            }

            Value * loopValue = fromData->FindValue(sym);
            if (loopValue == nullptr)
            {
                continue;
            }

            ValueInfo *landingPadValueInfo = landingPadValue->GetValueInfo();
            ValueInfo *loopValueInfo = loopValue->GetValueInfo();

            if (landingPadValueInfo->IsLikelyString()
                && loopValueInfo->IsLikelyString())
            {
                IR::Instr *cloneStr = IR::Instr::New(Js::OpCode::CloneStr, this->func);
                IR::RegOpnd *opnd = IR::RegOpnd::New(sym, IRType::TyVar, this->func);
                cloneStr->SetDst(opnd);
                cloneStr->SetSrc1(opnd);
                if (loop->bailOutInfo->bailOutInstr)
                {
                    loop->bailOutInfo->bailOutInstr->InsertBefore(cloneStr);
                }
                else
                {
                    landingPad->InsertAfter(cloneStr);
                }
                toData->isTempSrc->Set(id);
            }
        }
        NEXT_BITSET_IN_SPARSEBV;
        tempBv->ClearAll();
    }
}
