//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum RegionType : BYTE
{
    RegionTypeInvalid,
    RegionTypeRoot,
    RegionTypeTry,
    RegionTypeCatch,
    RegionTypeFinally
};

class Region
{
public:
    Region() : type(RegionTypeInvalid),
               parent(NULL), matchingTryRegion(nullptr), matchingCatchRegion(nullptr), matchingFinallyOnExceptRegion(nullptr), matchingFinallyOnNoExceptRegion(nullptr), selfOrFirstTryAncestor(nullptr),
               start(NULL), end(NULL),
               writeThroughSymbolsSet(nullptr),
               ehBailoutData(nullptr), bailoutReturnThunkLabel(nullptr), returnThunkEmitted(false),
               exceptionObjectSym(nullptr) {}
    static Region * New(RegionType, Region *, Func *);

public:
    inline RegionType GetType() const                   { return this->type; }
    inline void SetType(RegionType type)                { this->type = type; }

    inline Region * GetParent() const                   { return this->parent; }
    inline void SetParent(Region* parent)               { this->parent = parent; }

    inline Region * GetMatchingTryRegion() const        { return this->matchingTryRegion; }
    inline void SetMatchingTryRegion(Region* tryRegion) { this->matchingTryRegion = tryRegion; }

    inline Region * GetMatchingCatchRegion() const      { return this->matchingCatchRegion; }
    inline void SetMatchingCatchRegion(Region* catchRegion) { this->matchingCatchRegion = catchRegion; }

    inline Region * GetMatchingFinallyRegion(bool isExcept) const
    {
        return isExcept ? this->matchingFinallyOnExceptRegion : this->matchingFinallyOnNoExceptRegion;
    }
    inline void SetMatchingFinallyRegion(Region* finallyRegion, bool isExcept)
    {
        if (isExcept)
        {
            this->matchingFinallyOnExceptRegion = finallyRegion;
        }
        else
        {
            this->matchingFinallyOnNoExceptRegion = finallyRegion;
        }
    }
    bool IsNonExceptingFinally()
    {
        return (this->GetType() == RegionTypeFinally && this->GetMatchingTryRegion()->GetMatchingFinallyRegion(false) == this);
    }

    inline IR::Instr * GetStart() const                 { return this->start; }
    inline void SetStart(IR::Instr * instr)             { this->start = instr; }
    inline IR::Instr * GetEnd() const                   { return this->end; }
    inline void SetEnd(IR::Instr * instr)               { this->end = instr; }
    inline IR::LabelInstr * GetBailoutReturnThunkLabel() const { return this->bailoutReturnThunkLabel; }
    inline StackSym * GetExceptionObjectSym() const     { return this->exceptionObjectSym; }
    inline void SetExceptionObjectSym(StackSym * sym)   { this->exceptionObjectSym = sym; }
    void   AllocateEHBailoutData(Func * func, IR::Instr * tryInstr);
    Region * GetSelfOrFirstTryAncestor();
    Region * GetFirstAncestorOfNonExceptingFinallyParent();
    Region * GetFirstAncestorOfNonExceptingFinally();

private:
    RegionType                      type;
    Region *                        parent;
    Region *                        matchingTryRegion;
    Region *                        matchingCatchRegion;
    Region *                        matchingFinallyOnExceptRegion;
    Region *                        matchingFinallyOnNoExceptRegion;
    // We need to mark a non-expecting finally region we execute in the JIT, as in EH region.
    // We can bailout from the non excepting EH region, in that case we need ehBailoutData to reconstruct eh frames in the interpreter

    Region *                        selfOrFirstTryAncestor; // = self, if try region, otherwise
                                                            // = first try ancestor
    IR::Instr *                     start;
    IR::Instr *                     end;
    StackSym *                      exceptionObjectSym;
    IR::LabelInstr *                bailoutReturnThunkLabel;
    // bailoutReturnThunkLabel is the Label denoting start of return thunk for this region.

    // The JIT'ed code of a function having EH may have multiple frames on the stack, pertaining to the JIT'ed code and the TryCatch helper.
    // After a bailout in an EH region, we want to jump to the epilog of the function, but we have to do this via a series of returns (to clear out the frames on the stack).

    // To achieve this, post bailout, we jump to the return thunk of that region, which loads the appropriate return address into eax and executes a RET.
    // This has the effect of returning that address to the TryCatch helper, which, in turn, returns it to its caller JIT'ed code.
    // Non-top-level EH regions return the address of their parent's return thunk, and top level EH regions return the address
    // where the return value from a bailout is loaded into eax (restoreReturnValueFromBailoutLabel in EHBailoutPatchUp::Emit).
    // (Control should go to a return thunk only in case of a bailout from an EH region.)

public:
    BVSparse<JitArenaAllocator> *   writeThroughSymbolsSet;
    Js::EHBailoutData *             ehBailoutData;
    bool                            returnThunkEmitted;
};
