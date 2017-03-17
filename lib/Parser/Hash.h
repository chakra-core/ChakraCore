//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// StaticSym contains a string literal at the end (flexible array) and is
// meant to be initialized statically. However, flexible array initialization
// is not allowed in standard C++. We declare each StaticSym with length
// instead and cast to common StaticSymLen<0>* (StaticSym*) to access.
template <uint32 N>
struct StaticSymLen
{
    uint32 luHash;
    uint32 cch;
    OLECHAR sz[N];
};

typedef StaticSymLen<0> StaticSym;

/***************************************************************************
Hashing functions. Definitions in core\hashfunc.cpp.
***************************************************************************/
ULONG CaseSensitiveComputeHash(LPCOLESTR prgch, LPCOLESTR end);
ULONG CaseSensitiveComputeHash(LPCUTF8 prgch, LPCUTF8 end);
ULONG CaseInsensitiveComputeHash(LPCOLESTR posz);

enum
{
    fidNil          = 0x0000,
    fidKwdRsvd      = 0x0001,     // the keyword is a reserved word
    fidKwdFutRsvd   = 0x0002,     // a future reserved word, but only in strict mode

    // Flags to identify tracked aliases of "eval"
    fidEval         = 0x0008,
    // Flags to identify tracked aliases of "let"
    fidLetOrConst   = 0x0010,     // ID has previously been used in a block-scoped declaration

    // This flag is used by the Parser CountDcls and FillDcls methods.
    // CountDcls sets the bit as it walks through the var decls so that
    // it can skip duplicates. FillDcls clears the bit as it walks through
    // again to skip duplicates.
    fidGlobalDcl    = 0x2000,

    fidUsed         = 0x4000,  // name referenced by source code

    fidModuleExport = 0x8000    // name is module export
};

struct BlockIdsStack
{
    int id;
    BlockIdsStack *prev;
};

class Span
{
    charcount_t m_ichMin;
    charcount_t m_ichLim;

public:
    Span(): m_ichMin((charcount_t)-1), m_ichLim((charcount_t)-1) { }
    Span(charcount_t ichMin, charcount_t ichLim): m_ichMin(ichMin), m_ichLim(ichLim) { }

    charcount_t GetIchMin() { return m_ichMin; }
    charcount_t GetIchLim() { Assert(m_ichMin != (charcount_t)-1); return m_ichLim; }
    void Set(charcount_t ichMin, charcount_t ichLim)
    {
        m_ichMin = ichMin;
        m_ichLim = ichLim;
    }

    operator bool() { return m_ichMin != -1; }
};

struct PidRefStack
{
    PidRefStack() : isAsg(false), isDynamic(false), id(0), funcId(0), sym(nullptr), prev(nullptr), isEscape(false), isModuleExport(false), isFuncAssignment(false), isUsedInLdElem(false) {}
    PidRefStack(int id, Js::LocalFunctionId funcId) : isAsg(false), isDynamic(false), id(id), funcId(funcId), sym(nullptr), prev(nullptr), isEscape(false), isModuleExport(false), isFuncAssignment(false), isUsedInLdElem(false) {}

    int GetScopeId() const    { return id; }
    Js::LocalFunctionId GetFuncScopeId() const { return funcId; }
    Symbol *GetSym() const    { return sym; }
    void SetSym(Symbol *sym)  { this->sym = sym; }
    bool IsAssignment() const { return isAsg; }
    bool IsFuncAssignment() const { return isFuncAssignment; }
    bool IsEscape() const { return isEscape; }
    void SetIsEscape(bool is) { isEscape = is; }
    bool IsDynamicBinding() const { return isDynamic; }
    void SetDynamicBinding()  { isDynamic = true; }
    bool IsUsedInLdElem() const { return isUsedInLdElem; }
    void SetIsUsedInLdElem(bool is) { isUsedInLdElem = is; }

    Symbol **GetSymRef()
    {
        return &sym;
    }

    bool           isAsg;
    bool           isDynamic;
    bool           isModuleExport;
    bool           isEscape;
    bool           isFuncAssignment;
    bool           isUsedInLdElem;
    int            id;
    Js::LocalFunctionId funcId;
    Symbol        *sym;
    PidRefStack   *prev;
};

enum AssignmentState : byte {
    NotAssigned,
    AssignedOnce,
    AssignedMultipleTimes
};

struct Ident
{
    friend class HashTbl;

private:
    Ident * m_pidNext;   // next identifier in this hash bucket
    PidRefStack *m_pidRefStack;
    ushort m_tk;         // token# if identifier is a keyword
    ushort m_grfid;      // see fidXXX above
    uint32 m_luHash;      // hash value

    uint32 m_cch;                   // length of the identifier spelling
    Js::PropertyId m_propertyId;

    AssignmentState assignmentState;
    bool isUsedInLdElem;

    OLECHAR m_sz[]; // the spelling follows (null terminated)

    void SetTk(tokens tk, ushort grfid);
public:
    LPCOLESTR Psz(void)
    { return m_sz; }
    uint32 Cch(void)
    { return m_cch; }
    tokens Tk(bool isStrictMode);
    uint32 Hash(void)
    { return m_luHash; }

    PidRefStack *GetTopRef() const
    {
        return m_pidRefStack;
    }

    PidRefStack *GetTopRef(uint maxBlockId) const
    {
        PidRefStack *ref;
        for (ref = m_pidRefStack; ref && (uint)ref->id > maxBlockId; ref = ref->prev)
        {
            ; // nothing
        }
        return ref;
    }

    void SetTopRef(PidRefStack *ref)
    {
        m_pidRefStack = ref;
    }

    void PromoteAssignmentState()
    {
        if (assignmentState == NotAssigned)
        {
            assignmentState = AssignedOnce;
        }
        else if (assignmentState == AssignedOnce)
        {
            assignmentState = AssignedMultipleTimes;
        }
    }

    bool IsUsedInLdElem() const
    {
        return this->isUsedInLdElem;
    }

    void SetIsUsedInLdElem(bool is)
    {
        this->isUsedInLdElem = is;
    }

    static void TrySetIsUsedInLdElem(ParseNode * pnode)
    {
        if (pnode && pnode->nop == knopStr)
        {
            pnode->sxPid.pid->SetIsUsedInLdElem(true);
        }
    }

    bool IsSingleAssignment()
    {
        return assignmentState == AssignedOnce;
    }

    PidRefStack *GetPidRefForScopeId(int scopeId)
    {
        PidRefStack *ref;
        for (ref = m_pidRefStack; ref; ref = ref->prev)
        {
            int refId = ref->GetScopeId();
            if (refId == scopeId)
            {
                return ref;
            }
            if (refId < scopeId)
            {
                break;
            }
        }
        return nullptr;
    }

    void PushPidRef(int blockId, Js::LocalFunctionId funcId, PidRefStack *newRef)
    {
        AssertMsg(blockId >= 0, "Block Id's should be greater than 0");
        newRef->id = blockId;
        newRef->funcId = funcId;
        newRef->prev = m_pidRefStack;
        m_pidRefStack = newRef;
    }

    PidRefStack * RemovePrevPidRef(PidRefStack *ref)
    {
        PidRefStack *prevRef;
        if (ref == nullptr)
        {
            prevRef = m_pidRefStack;
            Assert(prevRef);
            m_pidRefStack = prevRef->prev;
        }
        else
        {
            prevRef = ref->prev;
            Assert(prevRef);
            ref->prev = prevRef->prev;
        }
        return prevRef;
    }

    PidRefStack * FindOrAddPidRef(ArenaAllocator *alloc, int scopeId, Js::LocalFunctionId funcId)
    {
        // If the stack is empty, or we are pushing to the innermost scope already,
        // we can go ahead and push a new PidRef on the stack.
        if (m_pidRefStack == nullptr)
        {
            PidRefStack *newRef = Anew(alloc, PidRefStack, scopeId, funcId);
            if (newRef == nullptr)
            {
                return nullptr;
            }
            newRef->prev = m_pidRefStack;
            m_pidRefStack = newRef;
            return newRef;
        }

        // Search for the corresponding PidRef, or the position to insert the new PidRef.
        PidRefStack *ref = m_pidRefStack;
        PidRefStack *prevRef = nullptr;
        while (1)
        {
            // We may already have a ref for this scopeId.
            if (ref->id == scopeId)
            {
                return ref;
            }

            if (ref->prev == nullptr || ref->id  < scopeId)
            {
                // No existing PidRef for this scopeId, so create and insert one at this position.
                PidRefStack *newRef = Anew(alloc, PidRefStack, scopeId, funcId);
                if (newRef == nullptr)
                {
                    return nullptr;
                }

                if (ref->id < scopeId)
                {
                    if (prevRef != nullptr)
                    {
                        // Param scope has a reference to the same pid as the one we are inserting into the body.
                        // There is a another reference (prevRef), probably from an inner block in the body.
                        // So we should insert the new reference between them.
                        newRef->prev = prevRef->prev;
                        prevRef->prev = newRef;
                    }
                    else
                    {
                        // When we have code like below, prevRef will be null,
                        // function (a = x) { var x = 1; }
                        newRef->prev = m_pidRefStack;
                        m_pidRefStack = newRef;
                    }
                }
                else
                {
                    newRef->prev = ref->prev;
                    ref->prev = newRef;
                }
                return newRef;
            }

            Assert(ref->prev->id <= ref->id);
            prevRef = ref;
            ref = ref->prev;
        }
    }

    Js::PropertyId GetPropertyId() const { return m_propertyId; }
    void SetPropertyId(Js::PropertyId id) { m_propertyId = id; }

    void SetIsEval() { m_grfid |= fidEval; }
    BOOL GetIsEval() const { return m_grfid & fidEval; }

    void SetIsLetOrConst() { m_grfid |= fidLetOrConst; }
    BOOL GetIsLetOrConst() const { return m_grfid & fidLetOrConst; }

    void SetIsModuleExport() { m_grfid |= fidModuleExport; }
    BOOL GetIsModuleExport() const { return m_grfid & fidModuleExport; }

    static tokens TkFromNameLen(uint32 luHash, _In_reads_(cch) LPCOLESTR prgch, uint32 cch, bool isStrictMode, ushort * pgrfid, ushort * ptk);

#if DBG
    static tokens TkFromNameLen(_In_reads_(cch) LPCOLESTR prgch, uint32 cch, bool isStrictMode);
#endif
};


/*****************************************************************************/

class HashTbl
{
public:
    static HashTbl * Create(uint cidHash);

    void Release(void)
    {
        delete this;  // invokes overrided operator delete
    }


    BOOL TokIsBinop(tokens tk, int *popl, OpCode *pnop)
    {
        const KWD *pkwd = KwdOfTok(tk);

        if (nullptr == pkwd)
            return FALSE;
        *popl = pkwd->prec2;
        *pnop = pkwd->nop2;
        return TRUE;
    }

    BOOL TokIsUnop(tokens tk, int *popl, OpCode *pnop)
    {
        const KWD *pkwd = KwdOfTok(tk);

        if (nullptr == pkwd)
            return FALSE;
        *popl = pkwd->prec1;
        *pnop = pkwd->nop1;
        return TRUE;
    }

    IdentPtr PidFromTk(tokens tk);
    IdentPtr PidHashName(LPCOLESTR psz)
    {
        size_t csz = wcslen(psz);
        Assert(csz <= ULONG_MAX);
        return PidHashNameLen(psz, static_cast<uint32>(csz));
    }

    template <typename CharType>
    IdentPtr PidHashNameLen(CharType const * psz, CharType const * end, uint32 cch);
    template <typename CharType>
    IdentPtr PidHashNameLen(CharType const * psz, uint32 cch);
    template <typename CharType>
    IdentPtr PidHashNameLenWithHash(_In_reads_(cch) CharType const * psz, CharType const * end, int32 cch, uint32 luHash);


    template <typename CharType>
    inline IdentPtr FindExistingPid(
        CharType const * prgch,
        CharType const * end,
        int32 cch,
        uint32 luHash,
        IdentPtr **pppInsert,
        int32 *pBucketCount
#if PROFILE_DICTIONARY
        , int& depth
#endif
        );

    NoReleaseAllocator* GetAllocator() {return &m_noReleaseAllocator;}

    bool Contains(_In_reads_(cch) LPCOLESTR prgch, int32 cch);
    void ClearPidRefStacks();

private:
    NoReleaseAllocator m_noReleaseAllocator;            // to allocate identifiers
    Ident ** m_prgpidName;        // hash table for names

    uint32 m_luMask;                // hash mask
    uint32 m_luCount;              // count of the number of entires in the hash table    
    IdentPtr m_rpid[tkLimKwd];

    HashTbl()
    {
        m_prgpidName = nullptr;
        memset(&m_rpid, 0, sizeof(m_rpid));
    }
    ~HashTbl(void) {}

    void operator delete(void* p, size_t size)
    {
        HeapFree(p, size);
    }

    // Called to grow the number of buckets in the table to reduce the table density.
    void Grow();

    // Automatically grow the table if a bucket's length grows beyond BucketLengthLimit and the table is densely populated.
    static const uint BucketLengthLimit = 5;

    // When growing the bucket size we'll grow by GrowFactor. GrowFactor MUST be a power of 2.
    static const uint GrowFactor = 4;

#if DEBUG
    uint CountAndVerifyItems(IdentPtr *buckets, uint bucketCount, uint mask);
#endif

    static bool CharsAreEqual(__in_z LPCOLESTR psz1, __in_ecount(psz2end - psz2) LPCOLESTR psz2, LPCOLESTR psz2end)
    {
        return memcmp(psz1, psz2, (psz2end - psz2) * sizeof(OLECHAR)) == 0;
    }
    static bool CharsAreEqual(__in_z LPCOLESTR psz1, LPCUTF8 psz2, LPCUTF8 psz2end)
    {
        return utf8::CharsAreEqual(psz1, psz2, psz2end, utf8::doAllowThreeByteSurrogates);
    }
    static bool CharsAreEqual(__in_z LPCOLESTR psz1, __in_ecount(psz2end - psz2) char const * psz2, char const * psz2end)
    {
        while (psz2 < psz2end)
        {
            if (*psz1++ != *psz2++)
            {
                return false;
            }
        }
        return true;
    }
    static void CopyString(__in_ecount((psz2end - psz2) + 1) LPOLESTR psz1, __in_ecount(psz2end - psz2) LPCOLESTR psz2, LPCOLESTR psz2end)
    {
        size_t cch = psz2end - psz2;
        js_memcpy_s(psz1, cch * sizeof(OLECHAR), psz2, cch * sizeof(OLECHAR));
        psz1[cch] = 0;
    }
    static void CopyString(LPOLESTR psz1, LPCUTF8 psz2, LPCUTF8 psz2end)
    {
        utf8::DecodeUnitsIntoAndNullTerminate(psz1, psz2, psz2end);
    }
    static void CopyString(LPOLESTR psz1, char const * psz2, char const * psz2end)
    {
        while (psz2 < psz2end)
        {
            *(psz1++) = *psz2++;
        }
        *psz1 = 0;
    }

    // note: on failure this may throw or return FALSE, depending on
    // where the failure occurred.
    BOOL Init(uint cidHash);

    /*************************************************************************/
    /* The following members are related to the keyword descriptor tables    */
    /*************************************************************************/
    struct KWD
    {
        OpCode nop2;
        byte prec2;
        OpCode nop1;
        byte prec1;
    };
    struct ReservedWordInfo
    {
        StaticSym const * sym;
        ushort grfid;
    };
    static const ReservedWordInfo s_reservedWordInfo[tkID];
    static const KWD g_mptkkwd[tkLimKwd];
    static const KWD * KwdOfTok(tokens tk)
    { return (unsigned int)tk < tkLimKwd ? g_mptkkwd + tk : nullptr; }

    static __declspec(noreturn) void OutOfMemory();
#if PROFILE_DICTIONARY
    DictionaryStats *stats;
#endif
};

