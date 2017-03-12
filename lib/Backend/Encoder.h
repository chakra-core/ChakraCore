//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "EncoderMD.h"
#include "Backend.h"

///---------------------------------------------------------------------------
///
/// class Encoder
///
///---------------------------------------------------------------------------

typedef JsUtil::List<NativeOffsetInlineeFramePair, ArenaAllocator> InlineeFrameMap;
typedef JsUtil::List<IR::PragmaInstr*, ArenaAllocator> PragmaInstrList;
typedef JsUtil::List<uint32, ArenaAllocator> OffsetList;
typedef JsUtil::List<BranchJumpTableWrapper*, ArenaAllocator> JmpTableList;

class Encoder
{
    friend class EncoderMD;
public:
    Encoder(Func * func) : m_func(func), m_encoderMD(func), m_inlineeFrameMap(nullptr) {}

    void            Encode();
    void            RecordInlineeFrame(Func* inlinee, uint32 currentOffset);
    void            RecordBailout(IR::Instr* instr, uint32 currentOffset);
private:
    bool            DoTrackAllStatementBoundary() const;

    Func *          m_func;
    EncoderMD       m_encoderMD;
    BYTE *          m_encodeBuffer;
    BYTE *          m_pc;
    uint32          m_encodeBufferSize;
    ArenaAllocator *m_tempAlloc;

    InlineeFrameMap* m_inlineeFrameMap;
    uint32 m_inlineeFrameMapDataOffset;
    uint32 m_inlineeFrameMapRecordCount;
    typedef JsUtil::List<LazyBailOutRecord, ArenaAllocator> BailoutRecordMap;
    BailoutRecordMap* m_bailoutRecordMap;
#if DBG_DUMP
    void DumpInlineeFrameMap(size_t baseAddress);
    uint32 *        m_offsetBuffer;
    uint32          m_instrNumber;
#endif

    PragmaInstrList     *m_pragmaInstrToRecordOffset;
    PragmaInstrList     *m_pragmaInstrToRecordMap;
#if defined(_M_IX86) || defined(_M_X64)
    InlineeFrameRecords *m_inlineeFrameRecords;

    BOOL            ShortenBranchesAndLabelAlign(BYTE **codeStart, ptrdiff_t *codeSize, uint * brShortenedBufferCRC, uint bufferCrcToValidate, size_t jumpTableSize);
    void            revertRelocList();
    template <bool restore> void  CopyMaps(OffsetList **m_origInlineeFrameRecords, OffsetList **m_origInlineeFrameMap, OffsetList **m_origPragmaInstrToRecordOffset, OffsetList **m_origOffsetBuffer);
#endif
    void            InsertNopsForLabelAlignment(int nopCount, BYTE ** pDstBuffer);
    void            CopyPartialBufferAndCalculateCRC(BYTE ** ptrDstBuffer, size_t &dstSize, BYTE * srcStart, BYTE * srcEnd, uint* pBufferCRC, size_t jumpTableSize = 0);
    BYTE            FindNopCountFor16byteAlignment(size_t address);

    uint32          GetCurrentOffset() const;
    void            TryCopyAndAddRelocRecordsForSwitchJumpTableEntries(BYTE *codeStart, size_t codeSize, JmpTableList * jumpTableListForSwitchStatement, size_t totalJmpTableSizeInBytes);

    void            ValidateCRC(uint bufferCRC, uint initialCRCSeed, _In_reads_bytes_(count) void* buffer, size_t count);
    static uint     CalculateCRC(uint bufferCRC, size_t count, _In_reads_bytes_(count) void * buffer);
    static uint     CalculateCRC(uint bufferCRC, size_t data);
    static void     EnsureRelocEntryIntegrity(size_t newBufferStartAddress, size_t codeSize, size_t oldBufferAddress, size_t relocAddress, uint offsetBytes, ptrdiff_t opndData, bool isRelativeAddr = true);
#if defined(_M_IX86) || defined(_M_X64)
    void            ValidateCRCOnFinalBuffer(_In_reads_bytes_(finalCodeSize) BYTE * finalCodeBufferStart, size_t finalCodeSize, size_t jumpTableSize, _In_reads_bytes_(finalCodeSize) BYTE * oldCodeBufferStart, uint initialCrcSeed, uint bufferCrcToValidate, BOOL isSuccessBrShortAndLoopAlign);
#endif
};

