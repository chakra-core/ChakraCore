//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"
#if PROFILE_DICTIONARY
#include "DictionaryStats.h"
#endif


const HashTbl::KWD HashTbl::g_mptkkwd[tkLimKwd] =
{
    { knopNone,0,knopNone,0 },
#define KEYWORD(tk,f,prec2,nop2,prec1,nop1,name) \
    { nop2,kopl##prec2,nop1,kopl##prec1 },
#define TOK_DCL(tk,prec2,nop2,prec1,nop1) \
    { nop2,kopl##prec2,nop1,kopl##prec1 },
#include "keywords.h"
};

const HashTbl::ReservedWordInfo HashTbl::s_reservedWordInfo[tkID] =
{
    { nullptr, fidNil },
#define KEYWORD(tk,f,prec2,nop2,prec1,nop1,name) \
        { reinterpret_cast<const StaticSym*>(&g_ssym_##name), f },
#include "keywords.h"
};

HashTbl * HashTbl::Create(uint cidHash)
{
    HashTbl * phtbl;

    if (nullptr == (phtbl = HeapNewNoThrow(HashTbl)))
        return nullptr;
    if (!phtbl->Init(cidHash))
    {
        delete phtbl;  // invokes overrided operator delete
        return nullptr;
    }

    return phtbl;
}


BOOL HashTbl::Init(uint cidHash)
{
    // cidHash must be a power of two
    Assert(cidHash > 0 && 0 == (cidHash & (cidHash - 1)));

    int32 cb;

    /* Allocate and clear the hash bucket table */
    m_luMask = cidHash - 1;
    m_luCount = 0;

    // (Bug 1117873 - Windows OS Bugs)
    // Prefast: Verify that cidHash * sizeof(Ident *) does not cause an integer overflow
    // NoReleaseAllocator( ) takes int32 - so check for LONG_MAX
    // Win8 730594 - Use intsafe function to check for overflow.
    uint cbTemp;
    if (FAILED(UIntMult(cidHash, sizeof(Ident *), &cbTemp)) || cbTemp > LONG_MAX)
        return FALSE;

    cb = cbTemp;
    if (nullptr == (m_prgpidName = (Ident **)m_noReleaseAllocator.Alloc(cb)))
        return FALSE;
    memset(m_prgpidName, 0, cb);

#if PROFILE_DICTIONARY
    stats = DictionaryStats::Create(typeid(this).name(), cidHash);
#endif

    return TRUE;
}

void HashTbl::Grow()
{
    // Grow the bucket size by grow factor
    // Has the side-effect of inverting the order the pids appear in their respective buckets.
    uint cidHash = m_luMask + 1;
    uint n_cidHash = cidHash * GrowFactor;
    Assert(n_cidHash > 0 && 0 == (n_cidHash & (n_cidHash - 1)));

    // Win8 730594 - Use intsafe function to check for overflow.
    uint cbTemp;
    if (FAILED(UIntMult(n_cidHash, sizeof(Ident *), &cbTemp)) || cbTemp > LONG_MAX)
        // It is fine to exit early here, we will just have a potentially densely populated hash table
        return;
    int32 cb = cbTemp;
    uint n_luMask = n_cidHash - 1;

    IdentPtr *n_prgpidName = (IdentPtr *)m_noReleaseAllocator.Alloc(cb);
    if (n_prgpidName == nullptr)
        // It is fine to exit early here, we will just have a potentially densely populated hash table
        return;

    // Clear the array
    memset(n_prgpidName, 0, cb);

    // Place each entry its new bucket.
    for (uint i = 0; i < cidHash; i++)
    {
        for (IdentPtr pid = m_prgpidName[i], next = pid ? pid->m_pidNext : nullptr; pid; pid = next, next = pid ? pid->m_pidNext : nullptr)
        {
            uint32 luHash = pid->m_luHash;
            uint32 luIndex = luHash & n_luMask;
            pid->m_pidNext = n_prgpidName[luIndex];
            n_prgpidName[luIndex] = pid;
        }
    }

    Assert(CountAndVerifyItems(n_prgpidName, n_cidHash, n_luMask) == m_luCount);

    // Update the table fields.
    m_prgpidName = n_prgpidName;
    m_luMask= n_luMask;

#if PROFILE_DICTIONARY
    if(stats)
    {
        int emptyBuckets = 0;
        for (uint i = 0; i < n_cidHash; i++)
        {
            if(m_prgpidName[i] == nullptr)
            {
                emptyBuckets++;
            }
        }
        stats->Resize(n_cidHash, emptyBuckets);
    }
#endif
}

void HashTbl::ClearPidRefStacks()
{
    // Clear pidrefstack pointers from all existing pid's.
    for (uint i = 0; i < m_luMask; i++)
    {
        for (IdentPtr pid = m_prgpidName[i]; pid; pid = pid->m_pidNext)
        {
            pid->m_pidRefStack = nullptr;
        }
    }
}

#if DEBUG
uint HashTbl::CountAndVerifyItems(IdentPtr *buckets, uint bucketCount, uint mask)
{
    uint count = 0;
    for (uint i = 0; i < bucketCount; i++)
        for (IdentPtr pid = buckets[i]; pid; pid = pid->m_pidNext)
        {
            Assert((pid->m_luHash & mask) == i);
            count++;
        }
    return count;
}
#endif


#pragma warning(push)
#pragma warning(disable:4740)  // flow in or out of inline asm code suppresses global optimization

// Decide if token is keyword by string matching -
// This method is used during colorizing when scanner isn't interested in storing the actual id and does not care about conversion of escape sequences
tokens Ident::TkFromNameLen(uint32 luHash, _In_reads_(cch) LPCOLESTR prgch, uint32 cch, bool isStrictMode, ushort * pgrfid, ushort * ptk)
{
    // look for a keyword
    #include "kwds_sw.h"

    #define KEYWORD(tk,f,prec2,nop2,prec1,nop1,name) \
    LEqual_##name: \
        if (cch == g_ssym_##name.cch && \
                0 == memcmp(g_ssym_##name.sz, prgch, cch * sizeof(OLECHAR))) \
        { \
            if (f) \
                *pgrfid |= f; \
            *ptk = tk; \
            return ((f & fidKwdRsvd) || (isStrictMode && (f & fidKwdFutRsvd))) ? tk : tkID; \
        } \
        goto LDefault;
    #include "keywords.h"

LDefault:
    return tkID;
}

#pragma warning(pop)

#if DBG
tokens Ident::TkFromNameLen(_In_reads_(cch) LPCOLESTR prgch, uint32 cch, bool isStrictMode)
{
    uint32 luHash = CaseSensitiveComputeHash(prgch, prgch + cch);
    ushort grfid;
    ushort tk;
    return TkFromNameLen(luHash, prgch, cch, isStrictMode, &grfid, &tk);
}
#endif

tokens Ident::Tk(bool isStrictMode)
{
    const tokens token = (tokens)m_tk;
    if (token == tkLim)
    {
        m_tk = tkNone;
        const uint32 luHash = this->m_luHash;
        const LPCOLESTR prgch = Psz();
        const uint32 cch = Cch();

        return TkFromNameLen(luHash, prgch, cch, isStrictMode, &this->m_grfid, &this->m_tk);
    }
    else if (token == tkNone || !(m_grfid & fidKwdRsvd))
    {
        if ( !isStrictMode || !(m_grfid & fidKwdFutRsvd))
        {
            return tkID;
        }
    }
    return token;
}

void Ident::SetTk(tokens token, ushort grfid)
{
    Assert(token != tkNone && token < tkID);
    if (m_tk == tkLim)
    {
        m_tk = (ushort)token;
        m_grfid |= grfid;
    }
    else
    {
        Assert(m_tk == token);
        Assert((m_grfid & grfid) == grfid);
    }
}

IdentPtr HashTbl::PidFromTk(tokens token)
{
    Assert(token > tkNone && token < tkID);
    __analysis_assume(token > tkNone && token < tkID);
    // Create a pid so we can create a name node
    IdentPtr rpid = m_rpid[token];
    if (nullptr == rpid)
    {
        StaticSym const * sym = s_reservedWordInfo[token].sym;
        Assert(sym != nullptr);
        rpid = this->PidHashNameLenWithHash(sym->sz, sym->sz + sym->cch, sym->cch, sym->luHash);
        rpid->SetTk(token, s_reservedWordInfo[token].grfid);
        m_rpid[token] = rpid;
    }
    return rpid;
}

template <typename CharType>
IdentPtr HashTbl::PidHashNameLen(CharType const * prgch, CharType const * end, uint32 cch)
{
    // NOTE: We use case sensitive hash during compilation, but the runtime
    // uses case insensitive hashing so it can do case insensitive lookups.

    uint32 luHash = CaseSensitiveComputeHash(prgch, end);
    return PidHashNameLenWithHash(prgch, end, cch, luHash);
}
template IdentPtr HashTbl::PidHashNameLen<utf8char_t>(utf8char_t const * prgch, utf8char_t const * end, uint32 cch);
template IdentPtr HashTbl::PidHashNameLen<char>(char const * prgch, char const * end, uint32 cch);
template IdentPtr HashTbl::PidHashNameLen<char16>(char16 const * prgch, char16 const * end, uint32 cch);

template <typename CharType>
IdentPtr HashTbl::PidHashNameLen(CharType const * prgch, uint32 cch)
{
    Assert(sizeof(CharType) == 2);
    return PidHashNameLen(prgch, prgch + cch, cch);
};
template IdentPtr HashTbl::PidHashNameLen<utf8char_t>(utf8char_t const * prgch, uint32 cch);
template IdentPtr HashTbl::PidHashNameLen<char>(char const * prgch, uint32 cch);
template IdentPtr HashTbl::PidHashNameLen<char16>(char16 const * prgch, uint32 cch);

template <typename CharType>
IdentPtr HashTbl::PidHashNameLenWithHash(_In_reads_(cch) CharType const * prgch, CharType const * end, int32 cch, uint32 luHash)
{
    Assert(cch >= 0);
    AssertArrMemR(prgch, cch);
    Assert(luHash == CaseSensitiveComputeHash(prgch, end));

    IdentPtr * ppid = nullptr;
    IdentPtr pid;
    LONG cb;
    int32 bucketCount;


#if PROFILE_DICTIONARY
    int depth = 0;
#endif

    pid = this->FindExistingPid(prgch, end, cch, luHash, &ppid, &bucketCount
#if PROFILE_DICTIONARY
                                , depth
#endif
        );
    if (pid)
    {
        return pid;
    }

    if (bucketCount > BucketLengthLimit && m_luCount > m_luMask)
    {
        Grow();

        // ppid is now invalid because the Grow() moves the entries around.
        // Find the correct ppid by repeating the find of the end of the bucket
        // the new item will be placed in.
        // Note this is similar to the main find loop but does not count nor does it
        // look at the entries because we already proved above the entry is not in the
        // table, we just want to find the end of the bucket.
        ppid = &m_prgpidName[luHash & m_luMask];
        while (*ppid)
            ppid = &(*ppid)->m_pidNext;
    }


#if PROFILE_DICTIONARY
    ++depth;
    if (stats)
        stats->Insert(depth);
#endif

    //Windows OS Bug 1795286 : CENTRAL PREFAST RUN: inetcore\scriptengines\src\src\core\hash.cpp :
    //               'sizeof((*pid))+((cch+1))*sizeof(OLECHAR)' may be smaller than
    //               '((cch+1))*sizeof(OLECHAR)'. This can be caused by integer overflows
    //               or underflows. This could yield an incorrect buffer all
    /* Allocate space for the identifier */
    ULONG Len;

    if (FAILED(ULongAdd(cch, 1, &Len)) ||
        FAILED(ULongMult(Len, sizeof(OLECHAR), &Len)) ||
        FAILED(ULongAdd(Len, sizeof(*pid), &Len)) ||
        FAILED(ULongToLong(Len, &cb)))
    {
        cb = 0;
        OutOfMemory();
    }


    if (nullptr == (pid = (IdentPtr)m_noReleaseAllocator.Alloc(cb)))
        OutOfMemory();

    /* Insert the identifier into the hash list */
    *ppid = pid;

    // Increment the number of entries in the table.
    m_luCount++;

    /* Fill in the identifier record */
    pid->m_pidNext = nullptr;
    pid->m_tk = tkLim;
    pid->m_grfid = fidNil;
    pid->m_luHash = luHash;
    pid->m_cch = cch;
    pid->m_pidRefStack = nullptr;
    pid->m_propertyId = Js::Constants::NoProperty;
    pid->assignmentState = NotAssigned;
    pid->isUsedInLdElem = false;

    HashTbl::CopyString(pid->m_sz, prgch, end);

    return pid;
}

template <typename CharType>
IdentPtr HashTbl::FindExistingPid(
    CharType const * prgch,
    CharType const * end,
    int32 cch,
    uint32 luHash,
    IdentPtr **pppInsert,
    int32 *pBucketCount
#if PROFILE_DICTIONARY
    , int& depth
#endif
    )
{
    int32 bucketCount;
    IdentPtr pid;
    IdentPtr *ppid = &m_prgpidName[luHash & m_luMask];

    /* Search the hash table for an existing match */
    ppid = &m_prgpidName[luHash & m_luMask];

    for (bucketCount = 0; nullptr != (pid = *ppid); ppid = &pid->m_pidNext, bucketCount++)
    {
        if (pid->m_luHash == luHash && (int)pid->m_cch == cch &&
            HashTbl::CharsAreEqual(pid->m_sz, prgch, end))
        {
            return pid;
        }
#if PROFILE_DICTIONARY
        ++depth;
#endif
    }

    if (pBucketCount)
    {
        *pBucketCount = bucketCount;
    }
    if (pppInsert)
    {
        *pppInsert = ppid;
    }

    return nullptr;
}

template IdentPtr HashTbl::FindExistingPid<utf8char_t>(
    utf8char_t const * prgch, utf8char_t const * end, int32 cch, uint32 luHash, IdentPtr **pppInsert, int32 *pBucketCount
#if PROFILE_DICTIONARY
    , int& depth
#endif
    );
template IdentPtr HashTbl::FindExistingPid<char>(
    char const * prgch, char const * end, int32 cch, uint32 luHash, IdentPtr **pppInsert, int32 *pBucketCount
#if PROFILE_DICTIONARY
    , int& depth
#endif
    );
template IdentPtr HashTbl::FindExistingPid<char16>(
    char16 const * prgch, char16 const * end, int32 cch, uint32 luHash, IdentPtr **pppInsert, int32 *pBucketCount
#if PROFILE_DICTIONARY
    , int& depth
#endif
    );

bool HashTbl::Contains(_In_reads_(cch) LPCOLESTR prgch, int32 cch)
{
    uint32 luHash = CaseSensitiveComputeHash(prgch, prgch + cch);

    for (auto pid = m_prgpidName[luHash & m_luMask]; pid; pid = pid->m_pidNext)
    {
        if (pid->m_luHash == luHash && (int)pid->m_cch == cch &&
            HashTbl::CharsAreEqual(pid->m_sz, prgch + cch, prgch))
        {
            return true;
        }
    }

    return false;
}

#include "HashFunc.cpp"


__declspec(noreturn) void HashTbl::OutOfMemory()
{
    throw ParseExceptionObject(ERRnoMemory);
}
