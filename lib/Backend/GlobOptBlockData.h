#pragma once

class ExprAttributes
{
protected:
    uint32 attributes;

public:
    ExprAttributes(const uint32 attributes = 0) : attributes(attributes)
    {
    }

    uint32 Attributes() const
    {
        return attributes;
    }

private:
    static const uint32 BitMask(const uint index)
    {
        return 1u << index;
    }

protected:
    void SetBitAttribute(const uint index, const bool bit)
    {
        if(bit)
        {
            attributes |= BitMask(index);
        }
        else
        {
            attributes &= ~BitMask(index);
        }
    }
};

class ExprHash
{
public:
    ExprHash() { this->opcode = 0; }
    ExprHash(int init) { Assert(init == 0); this->opcode = 0; }

    void Init(Js::OpCode opcode, ValueNumber src1Val, ValueNumber src2Val, ExprAttributes exprAttributes)
    {
        extern uint8 OpCodeToHash[(int)Js::OpCode::Count];

        uint32 opCodeHash = OpCodeToHash[(int)opcode];
        this->opcode = opCodeHash;
        this->src1Val = src1Val;
        this->src2Val = src2Val;
        this->attributes = exprAttributes.Attributes();

        // Assert too many opcodes...
        AssertMsg(this->opcode == (uint32)opCodeHash, "Opcode value too large for CSEs");
        AssertMsg(this->attributes == exprAttributes.Attributes(), "Not enough bits for expr attributes");

        // If value numbers are too large, just give up
        if (this->src1Val != src1Val || this->src2Val != src2Val)
        {
            this->opcode = 0;
            this->src1Val = 0;
            this->src2Val = 0;
            this->attributes = 0;
        }
    }

    Js::OpCode  GetOpcode()             { return (Js::OpCode)this->opcode; }
    ValueNumber GetSrc1ValueNumber()    { return this->src1Val; }
    ValueNumber GetSrc2ValueNumber()    { return this->src2Val; }
    ExprAttributes GetExprAttributes()  { return this->attributes; }
    bool        IsValid()               { return this->opcode != 0; }

    operator    uint()                  { return *(uint*)this; }

private:
    uint32  opcode: 8;
    uint32  src1Val: 11;
    uint32  src2Val: 11;
    uint32  attributes: 2;
};

#include "GlobHashTable.h"

typedef ValueHashTable<Sym *, Value *> GlobHashTable;
typedef HashBucket<Sym *, Value *> GlobHashBucket;

typedef ValueHashTable<ExprHash, Value *> ExprHashTable;
typedef HashBucket<ExprHash, Value *> ExprHashBucket;

typedef JsUtil::BaseHashSet<Value *, JitArenaAllocator> ValueSet;

struct StackLiteralInitFldData
{
    const Js::PropertyIdArray * propIds;
    uint currentInitFldCount;
};

typedef JsUtil::BaseDictionary<StackSym *, StackLiteralInitFldData, JitArenaAllocator> StackLiteralInitFldDataMap;

class GlobOptBlockData
{
    friend class GlobOpt; // REMOVE IN FUTURE COMMIT IN CHANGESET - USED FOR INITIALIZERS
public:
    GlobOptBlockData(Func *func) :
        symToValueMap(nullptr),
        exprToValueMap(nullptr),
        liveFields(nullptr),
        liveArrayValues(nullptr),
        maybeWrittenTypeSyms(nullptr),
        liveVarSyms(nullptr),
        liveInt32Syms(nullptr),
        liveLossyInt32Syms(nullptr),
        liveFloat64Syms(nullptr),
        liveSimd128F4Syms(nullptr),
        liveSimd128I4Syms(nullptr),
        hoistableFields(nullptr),
        argObjSyms(nullptr),
        maybeTempObjectSyms(nullptr),
        canStoreTempObjectSyms(nullptr),
        valuesToKillOnCalls(nullptr),
        inductionVariables(nullptr),
        availableIntBoundChecks(nullptr),
        startCallCount(0),
        argOutCount(0),
        totalOutParamCount(0),
        callSequence(nullptr),
        capturedValuesCandidate(nullptr),
        capturedValues(nullptr),
        changedSyms(nullptr),
        hasCSECandidates(false),
        curFunc(func),
        hasDataRef(nullptr),
        stackLiteralInitFldDataMap(nullptr),
        globOpt(nullptr)
    {
    }

    // Data
    GlobHashTable *                         symToValueMap;
    ExprHashTable *                         exprToValueMap;
    BVSparse<JitArenaAllocator> *           liveFields;
    BVSparse<JitArenaAllocator> *           liveArrayValues;
    BVSparse<JitArenaAllocator> *           maybeWrittenTypeSyms;
    BVSparse<JitArenaAllocator> *           isTempSrc;
    BVSparse<JitArenaAllocator> *           liveVarSyms;
    BVSparse<JitArenaAllocator> *           liveInt32Syms;
    // 'liveLossyInt32Syms' includes only syms that contain an int value that may not fully represent the value of the
    // equivalent var sym. The set (liveInt32Syms - liveLossyInt32Syms) includes only syms that contain an int value that fully
    // represents the value of the equivalent var sym, such as when (a + 1) is type-specialized. Among other things, this
    // bit-vector is used, based on the type of conversion that is needed, to determine whether conversion is necessary, and if
    // so, whether a bailout is needed. For instance, after type-specializing (a | 0), the int32 sym of 'a' cannot be reused in
    // (a + 1) during type-specialization. It needs to be converted again using a lossless conversion with a bailout.
    // Conversely, a lossless int32 sym can be reused to avoid a lossy conversion.
    BVSparse<JitArenaAllocator> *           liveLossyInt32Syms;
    BVSparse<JitArenaAllocator> *           liveFloat64Syms;

    // SIMD_JS
    BVSparse<JitArenaAllocator> *           liveSimd128F4Syms;
    BVSparse<JitArenaAllocator> *           liveSimd128I4Syms;

    BVSparse<JitArenaAllocator> *           hoistableFields;
    BVSparse<JitArenaAllocator> *           argObjSyms;
    BVSparse<JitArenaAllocator> *           maybeTempObjectSyms;
    BVSparse<JitArenaAllocator> *           canStoreTempObjectSyms;

    // This is the func that this block comes from, so the inlinee if from an inlined function
    Func *                                  curFunc;

    // 'valuesToKillOnCalls' includes values whose value types need to be killed upon a call. Upon a call, the value types of
    // values in the set are updated and removed from the set as appropriate.
    ValueSet *                              valuesToKillOnCalls;

    InductionVariableSet *                  inductionVariables;
    IntBoundCheckSet *                      availableIntBoundChecks;

    // Bailout data
    uint                                    startCallCount;
    uint                                    argOutCount;
    uint                                    totalOutParamCount;
    SListBase<IR::Opnd *> *                 callSequence;
    StackLiteralInitFldDataMap *            stackLiteralInitFldDataMap;

    CapturedValues *                        capturedValuesCandidate;
    CapturedValues *                        capturedValues;
    BVSparse<JitArenaAllocator> *           changedSyms;

    uint                                    inlinedArgOutCount;

    bool                                    hasCSECandidates;

private:
    bool *                                  hasDataRef;

    GlobOpt *                               globOpt;

public:
    void OnDataInitialized(JitArenaAllocator *const allocator)
    {
        Assert(allocator);

        hasDataRef = JitAnew(allocator, bool, true);
    }

    void OnDataReused(GlobOptBlockData *const fromData)
    {
        // If a block's data is deleted, *hasDataRef will be set to false. Since these two blocks are pointing to the same data,
        // they also need to point to the same has-data info.
        hasDataRef = fromData->hasDataRef;
    }

    void OnDataUnreferenced()
    {
        // Other blocks may still be using the data, we should only un-reference the previous data
        hasDataRef = nullptr;
    }

    void OnDataDeleted()
    {
        if(hasDataRef)
            *hasDataRef = false;
        OnDataUnreferenced();
    }

    bool HasData()
    {
        if(!hasDataRef)
            return false;
        if(*hasDataRef)
            return true;
        OnDataUnreferenced();
        return false;
    }

    // SIMD_JS
    BVSparse<JitArenaAllocator> * GetSimd128LivenessBV(IRType type)
    {
        switch (type)
        {
        case TySimd128F4:
            return liveSimd128F4Syms;
        case TySimd128I4:
            return liveSimd128I4Syms;
        default:
            Assert(UNREACHED);
            return nullptr;
        }
    }
};
