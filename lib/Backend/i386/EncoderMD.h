//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
class Encoder;

enum RelocType {
    RelocTypeBranch,                // cond, uncond branch
    RelocTypeCallPcrel,             // calls
    RelocTypeLabelUse,              // direct use of a label
    RelocTypeLabel,                 // points to label instr
    RelocTypeAlignedLabel,          // points to loop-top label instr that needs alignment
    RelocTypeInlineeEntryOffset,    // points to offset immediate in buffer
};

///---------------------------------------------------------------------------
///
/// class EncoderReloc
///
///---------------------------------------------------------------------------

class EncodeRelocAndLabels
{
public:
    RelocType           m_type;
    void *              m_ptr;              // points to encoded buffer byte or labelInstr
    void *              m_origPtr;          // original offset without shortening

private:
    // these are type specific fields
    union
    {
        IR::LabelInstr *    m_shortBrLabel;     // NULL if not a short branch
        uint32              m_InlineeOffset;
        BYTE                m_nopCount;         // for AlignedLabel, how many nops do we need to be 16-byte aligned
    };

    union
    {
        IR::LabelInstr       *    m_labelInstr;
        const void           *    m_fnAddress;
    };

public:
    void                init(RelocType type, void* ptr, IR::LabelInstr* labelInstr, const void * fnAddress)
    {
        m_type = type;
        m_ptr = ptr;
        m_InlineeOffset = 0;
        m_labelInstr = nullptr;

        if (type == RelocTypeLabel)
        {
            // preserve original PC for labels
            m_origPtr = (void*)((IR::LabelInstr*)ptr)->GetPC();
            m_nopCount = 0;
        }
        else
        {
            m_origPtr = ptr;
            if (type == RelocTypeBranch)
            {
                m_shortBrLabel = NULL;
                m_labelInstr = labelInstr;
            }
            else if (type == RelocTypeLabelUse)
            {
                Assert(labelInstr);
                m_labelInstr = labelInstr;
            }
            else if (type == RelocTypeCallPcrel)
            {
                Assert(fnAddress);
                m_fnAddress = fnAddress;
            }
        }
    }

    void                revert()
    {

        if (isLabel())
        {
            // recover old label PC and reset alignment nops
            // we keep aligned labels type so we align them on the second attempt.
            setLabelCurrPC(getLabelOrigPC());
            m_nopCount = 0;

            return;
        }

        if (m_type == RelocTypeBranch)
        {
            m_shortBrLabel = NULL;
        }

        m_ptr = m_origPtr;
    }

    bool                isLabel()           const { return isAlignedLabel() || m_type == RelocTypeLabel; }
    bool                isAlignedLabel()    const { return m_type == RelocTypeAlignedLabel; }
    bool                isLongBr()          const { return m_type == RelocTypeBranch && m_shortBrLabel == NULL; }
    bool                isShortBr()         const { return m_type == RelocTypeBranch && m_shortBrLabel != NULL; }

    BYTE*               getBrOpCodeByte()   const {
        Assert(m_type == RelocTypeBranch);
        return (BYTE*)m_origPtr - 1;
    }

    IR::LabelInstr *    getBrTargetLabel()  const
    {
        Assert(m_type == RelocTypeBranch);
        return m_shortBrLabel == NULL ? m_labelInstr : m_shortBrLabel;
    }

    IR::LabelInstr *    getLabel()  const
    {
        Assert(isLabel());
        return (IR::LabelInstr*) m_ptr;
    }

    IR::LabelInstr * GetLabelInstrForRelocTypeLabelUse()
    {
        Assert(m_type == RelocTypeLabelUse && m_labelInstr);
        return m_labelInstr;
    }

    const void * GetFnAddress()
    {
        Assert(m_type == RelocTypeCallPcrel && m_fnAddress);
        return m_fnAddress;
    }

    // get label original PC without shortening/alignment
    BYTE *  getLabelOrigPC()  const
    {
        Assert(isLabel());
        return ((BYTE*) m_origPtr);
    }

    // get label PC after shortening/alignment
    BYTE *  getLabelCurrPC()  const
    {
        Assert(isLabel());
        return getLabel()->GetPC();
    }

    BYTE    getLabelNopCount() const
    {
        Assert(isAlignedLabel());
        return m_nopCount;
    }

    void    setLabelCurrPC(BYTE* pc)
    {
        Assert(isLabel());
        getLabel()->SetPC(pc);
    }

    void    setLabelNopCount(BYTE nopCount)
    {
        Assert(isAlignedLabel());
        Assert (nopCount >= 0 && nopCount < 16);
        m_nopCount = nopCount;
    }

    // Marks this entry as a short Br entry
    void    setAsShortBr(IR::LabelInstr* label)
    {
        Assert(label != NULL);
        m_shortBrLabel = label;
    }
    // Validates if the branch is short and its target PC fits in one byte
    bool    validateShortBrTarget() const
    {
        return isShortBr() &&
            m_shortBrLabel->GetPC() - ((BYTE*)m_ptr + 1) >= -128 &&
            m_shortBrLabel->GetPC() - ((BYTE*)m_ptr + 1) <= 127;
    }

    uint32 GetInlineOffset()
    {
        return m_InlineeOffset;
    }

    void SetInlineOffset(uint32 offset)
    {
        m_InlineeOffset = offset;
    }
};


///---------------------------------------------------------------------------
///
/// class EncoderMD
///
///---------------------------------------------------------------------------

enum Forms : BYTE;

typedef JsUtil::List<InlineeFrameRecord*, ArenaAllocator> InlineeFrameRecords;
typedef JsUtil::List<EncodeRelocAndLabels, ArenaAllocator> RelocList;

class EncoderMD
{
public:
    EncoderMD(Func * func) : m_func(func) { }

    ptrdiff_t       Encode(IR::Instr * instr, BYTE *pc, BYTE* beginCodeAddress = nullptr);
    void            Init(Encoder *encoder);
    void            ApplyRelocs(uint32 codeBufferAddress, size_t codeSize, uint * bufferCRC, BOOL isBrShorteningSucceeded, bool isFinalBufferValidation = false);
    uint            GetRelocDataSize(EncodeRelocAndLabels *reloc);
    void            EncodeInlineeCallInfo(IR::Instr *instr, uint32 offset);
    static bool     TryConstFold(IR::Instr *instr, IR::RegOpnd *regOpnd);
    static bool     TryFold(IR::Instr *instr, IR::RegOpnd *regOpnd);
    static bool     SetsConditionCode(IR::Instr *instr);
    static bool     UsesConditionCode(IR::Instr *instr);
    static bool     IsOPEQ(IR::Instr *instr);
    static bool     IsSHIFT(IR::Instr *instr);
    RelocList*      GetRelocList() const { return m_relocList; }
    int             AppendRelocEntry(RelocType type, void *ptr, IR::LabelInstr * labelInstr = nullptr, const void * fnAddress = nullptr);
    int             FixRelocListEntry(uint32 index, int32 totalBytesSaved, BYTE *buffStart, BYTE* buffEnd);
    void            FixMaps(uint32 brOffset, int32 bytesSaved, uint32 *inlineeFrameRecordsIndex, uint32 *inlineeFrameMapIndex,  uint32 *pragmaInstToRecordOffsetIndex, uint32 *offsetBuffIndex);
    void            UpdateRelocListWithNewBuffer(RelocList * relocList, BYTE * newBuffer, BYTE * oldBufferStart, BYTE * oldBufferEnd);
#ifdef DBG
    void            VerifyRelocList(BYTE *buffStart, BYTE *buffEnd);
#endif
    void            AddLabelReloc(BYTE* relocAddress);
    BYTE *          GetRelocBufferAddress(EncodeRelocAndLabels * reloc);

private:
    const BYTE      GetOpcodeByte2(IR::Instr *instr);
    const BYTE *    GetFormTemplate(IR::Instr *instr);
    static Forms    GetInstrForm(IR::Instr *instr);
    const BYTE *    GetOpbyte(IR::Instr *instr);
    const BYTE      GetRegEncode(IR::RegOpnd *regOpnd);
    const uint32    GetLeadIn(IR::Instr * instr);
    static const uint32 GetOpdope(IR::Instr *instr);
    void            EmitModRM(IR::Instr * instr, IR::Opnd *opnd, BYTE reg1);
    void            EmitConst(size_t val, int size);
    int             EmitImmed(IR::Opnd * opnd, int opSize, int sbit);
    void            EmitCondBranch(IR::BranchInstr * branchInstr);
    bool            FitsInByte(size_t value);
    BYTE            GetMod(size_t offset,  bool baseRegIsEBP, int * pDispSize);
    BYTE            GetMod(IR::SymOpnd * opnd, int * pDispSize, RegNum& rmReg);
    BYTE            GetMod(IR::IndirOpnd * opnd, int * pDispSize);

private:
    Func *          m_func;
    Encoder *       m_encoder;
    BYTE *          m_pc;
    RelocList*      m_relocList;
    int32           m_lastLoopLabelPosition;

};
