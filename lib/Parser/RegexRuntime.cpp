//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

namespace UnifiedRegex
{
    // ----------------------------------------------------------------------
    // CountDomain
    // ----------------------------------------------------------------------

#if ENABLE_REGEX_CONFIG_OPTIONS
    void CountDomain::Print(DebugWriter* w) const
    {
        if (upper != CharCountFlag && lower == (CharCount)upper)
        {
            w->Print(_u("[%u]"), lower);
        }
        else
        {
            w->Print(_u("[%u-"), lower);
            if (upper == CharCountFlag)
                w->Print(_u("inf]"));
            else
                w->Print(_u("%u]"), (CharCount)upper);
        }
    }
#endif

    // ----------------------------------------------------------------------
    // Matcher (inlined, called from instruction Exec methods)
    // ----------------------------------------------------------------------

#define PUSH(contStack, T, ...) (new (contStack.Push<T>()) T(__VA_ARGS__))
#define PUSHA(assertionStack, T, ...) (new (assertionStack.Push()) T(__VA_ARGS__))
#define L2I(O, label) LabelToInstPointer<O##Inst>(Inst::InstTag::O, label)

#define FAIL_PARAMETERS input, inputOffset, instPointer, contStack, assertionStack, qcTicks
#define HARDFAIL_PARAMETERS(mode) input, inputLength, matchStart, inputOffset, instPointer, contStack, assertionStack, qcTicks, mode

    // Regex QC heuristics:
    // - TicksPerQC
    //     - Number of ticks from a previous QC needed to cause another QC. The value affects how often QC will be triggered, so
    //       on slower machines or debug builds, the value needs to be smaller to maintain a reasonable frequency of QCs.
    // - TicksPerQcTimeCheck
    //     - Number of ticks from a previous QC needed to trigger a time check. Elapsed time from the previous QC is checked to
    //       see if a QC needs to be triggered. The value must be less than TicksPerQc and small enough to reasonably guarantee
    //       a QC every TimePerQc milliseconds without affecting perf.
    // - TimePerQc
    //     - The target time between QCs

#if defined(_M_ARM)
    const uint Matcher::TicksPerQc = 1u << 19
#else
    const uint Matcher::TicksPerQc = 1u << (AutoSystemInfo::ShouldQCMoreFrequently() ? 17 : 21)
#endif
#if DBG
        >> 2
#endif
        ;

    const uint Matcher::TicksPerQcTimeCheck = Matcher::TicksPerQc >> 2;
    const uint Matcher::TimePerQc = AutoSystemInfo::ShouldQCMoreFrequently() ? 50 : 100; // milliseconds

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Matcher::PushStats(ContStack& contStack, const Char* const input) const
    {
        if (stats != 0)
        {
            stats->numPushes++;
            if (contStack.Position() > stats->stackHWM)
                stats->stackHWM = contStack.Position();
        }
        if (w != 0)
        {
            w->Print(_u("PUSH "));
            contStack.Top()->Print(w, input);
        }
    }

    void Matcher::PopStats(ContStack& contStack, const Char* const input) const
    {
        if (stats != 0)
        {
            stats->numPops++;
        }
        if (w != 0)
        {
            const Cont* top = contStack.Top();
            if (top == 0)
                w->PrintEOL(_u("<empty stack>"));
            else
            {
                w->Print(_u("POP "));
                top->Print(w, input);
            }
        }
    }

    void Matcher::UnPopStats(ContStack& contStack, const Char* const input) const
    {
        if (stats != 0)
        {
            stats->numPops--;
        }
        if (w != 0)
        {
            const Cont* top = contStack.Top();
            if (top == 0)
                w->PrintEOL(_u("<empty stack>"));
            else
            {
                w->Print(_u("UNPOP "));
                top->Print(w, input);
            }
        }
    }

    void Matcher::CompStats() const
    {
        if (stats != 0)
        {
            stats->numCompares++;
        }
    }

    void Matcher::InstStats() const
    {
        if (stats != 0)
        {
            stats->numInsts++;
        }
    }
#endif

    inline void Matcher::QueryContinue(uint &qcTicks)
    {
        // See definition of TimePerQc for description of regex QC heuristics

        Assert(!(TicksPerQc & TicksPerQc - 1)); // must be a power of 2
        Assert(!(TicksPerQcTimeCheck & TicksPerQcTimeCheck - 1)); // must be a power of 2
        Assert(TicksPerQcTimeCheck < TicksPerQc);

        if (PHASE_OFF1(Js::RegexQcPhase))
        {
            return;
        }
        if (++qcTicks & TicksPerQcTimeCheck - 1)
        {
            return;
        }
        DoQueryContinue(qcTicks);
    }

    inline bool Matcher::HardFail(
        const Char* const input
        , const CharCount inputLength
        , CharCount &matchStart
        , CharCount &inputOffset
        , const uint8 *&instPointer
        , ContStack &contStack
        , AssertionStack &assertionStack
        , uint &qcTicks
        , HardFailMode mode)
    {
        switch (mode)
        {
        case HardFailMode::BacktrackAndLater:
            return Fail(FAIL_PARAMETERS);
        case HardFailMode::BacktrackOnly:
            if (Fail(FAIL_PARAMETERS))
            {
                // No use trying any more start positions
                matchStart = inputLength;
                return true; // STOP EXECUTING
            }
            else
            {
                return false;
            }
        case HardFailMode::LaterOnly:
#if ENABLE_REGEX_CONFIG_OPTIONS
            if (w != 0)
            {
                w->PrintEOL(_u("CLEAR"));
            }
#endif
            contStack.Clear();
            assertionStack.Clear();
            return true; // STOP EXECUTING
        case HardFailMode::ImmediateFail:
            // No use trying any more start positions
            matchStart = inputLength;
            return true; // STOP EXECUTING
        default:
            Assume(false);
        }

        return true;
    }

    inline bool Matcher::PopAssertion(CharCount &inputOffset, const uint8 *&instPointer, ContStack &contStack, AssertionStack &assertionStack, bool succeeded)
    {
        AssertionInfo* info = assertionStack.Top();
        Assert(info != 0);
        assertionStack.Pop();
        BeginAssertionInst* begin = L2I(BeginAssertion, info->beginLabel);

        // Cut the existing continuations (we never backtrack into an assertion)
        // NOTE: We don't include the effective pops in the stats
#if ENABLE_REGEX_CONFIG_OPTIONS
        if (w != 0)
        {
            w->PrintEOL(_u("POP TO %llu"), (unsigned long long)info->contStackPosition);
        }
#endif
        contStack.PopTo(info->contStackPosition);

        // succeeded  isNegation  action
        // ---------  ----------  ----------------------------------------------------------------------------------
        // false      false       Fail into outer continuations (inner group bindings will have been undone)
        // true       false       Jump to next label (inner group bindings are now frozen)
        // false      true        Jump to next label (inner group bindings will have been undone and are now frozen)
        // true       true        Fail into outer continuations (inner group binding MUST BE CLEARED)

        if (succeeded && begin->isNegation)
        {
            ResetInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId);
        }

        if (succeeded == begin->isNegation)
        {
            // Assertion failed
            return false;
        }
        else
        {
            // Continue with next label but from original input position
            inputOffset = info->startInputOffset;
            instPointer = LabelToInstPointer(begin->nextLabel);

            return true;
        }
    }

    inline void Matcher::SaveInnerGroups(
        const int fromGroupId,
        const int toGroupId,
        const bool reset,
        const Char *const input,
        ContStack &contStack)
    {
        if (toGroupId >= 0)
        {
            DoSaveInnerGroups(fromGroupId, toGroupId, reset, input, contStack);
        }
    }

    void Matcher::DoSaveInnerGroups(
        const int fromGroupId,
        const int toGroupId,
        const bool reset,
        const Char *const input,
        ContStack &contStack)
    {
        Assert(fromGroupId >= 0);
        Assert(toGroupId >= 0);
        Assert(fromGroupId <= toGroupId);

        int undefinedRangeFromId = -1;
        int groupId = fromGroupId;
        do
        {
            GroupInfo *const groupInfo = GroupIdToGroupInfo(groupId);
            if (groupInfo->IsUndefined())
            {
                if (undefinedRangeFromId < 0)
                {
                    undefinedRangeFromId = groupId;
                }
                continue;
            }

            if (undefinedRangeFromId >= 0)
            {
                Assert(groupId > 0);
                DoSaveInnerGroups_AllUndefined(undefinedRangeFromId, groupId - 1, input, contStack);
                undefinedRangeFromId = -1;
            }

            PUSH(contStack, RestoreGroupCont, groupId, *groupInfo);
#if ENABLE_REGEX_CONFIG_OPTIONS
            PushStats(contStack, input);
#endif

            if (reset)
            {
                groupInfo->Reset();
            }
        } while (++groupId <= toGroupId);
        if (undefinedRangeFromId >= 0)
        {
            Assert(toGroupId >= 0);
            DoSaveInnerGroups_AllUndefined(undefinedRangeFromId, toGroupId, input, contStack);
        }
    }

    inline void Matcher::SaveInnerGroups_AllUndefined(
        const int fromGroupId,
        const int toGroupId,
        const Char *const input,
        ContStack &contStack)
    {
        if (toGroupId >= 0)
        {
            DoSaveInnerGroups_AllUndefined(fromGroupId, toGroupId, input, contStack);
        }
    }

    void Matcher::DoSaveInnerGroups_AllUndefined(
        const int fromGroupId,
        const int toGroupId,
        const Char *const input,
        ContStack &contStack)
    {
        Assert(fromGroupId >= 0);
        Assert(toGroupId >= 0);
        Assert(fromGroupId <= toGroupId);

#if DBG
        for (int groupId = fromGroupId; groupId <= toGroupId; ++groupId)
        {
            Assert(GroupIdToGroupInfo(groupId)->IsUndefined());
        }
#endif

        if (fromGroupId == toGroupId)
        {
            PUSH(contStack, ResetGroupCont, fromGroupId);
        }
        else
        {
            PUSH(contStack, ResetGroupRangeCont, fromGroupId, toGroupId);
        }

#if ENABLE_REGEX_CONFIG_OPTIONS
        PushStats(contStack, input);
#endif
    }

    inline void Matcher::ResetGroup(int groupId)
    {
        GroupInfo* info = GroupIdToGroupInfo(groupId);
        info->Reset();
    }

    inline void Matcher::ResetInnerGroups(int minGroupId, int maxGroupId)
    {
        for (int i = minGroupId; i <= maxGroupId; i++)
        {
            ResetGroup(i);
        }
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    bool Inst::IsBaselineMode()
    {
        return Js::Configuration::Global.flags.BaselineMode;
    }

    Label Inst::GetPrintLabel(Label label)
    {
        return IsBaselineMode() ? (Label)0xFFFF : label;
    }

    template <typename T>
    void Inst::PrintBytes(DebugWriter *w, Inst *inst, T *that, const char16 *annotation) const
    {
        T *start = (T*)that;
        byte *startByte = (byte *)start;
        byte *baseByte = (byte *)inst;
        ptrdiff_t offset = startByte - baseByte;
        size_t size = sizeof(*((T *)that));
        byte *endByte = startByte + size;
        byte *currentByte = startByte;
        w->Print(_u("0x%p[+0x%03x](0x%03x) [%s]:"), startByte, offset, size, annotation);

        for (; currentByte < endByte; ++currentByte)
        {
            if ((currentByte - endByte) % 4 == 0)
            {
                w->Print(_u(" "), *currentByte);
            }
            w->Print(_u("%02x"), *currentByte);
        }
        w->PrintEOL(_u(""));
    }

    template <>
    void Inst::PrintBytes(DebugWriter *w, Inst *inst, Inst *that, const char16 *annotation) const
    {
        Inst *start = (Inst *)that;

        size_t baseSize = sizeof(*(Inst *)that);
        ptrdiff_t offsetToData = (byte *)&(start->tag) - ((byte *)start);
        size_t size = baseSize - offsetToData;

        byte *startByte = (byte *)(&(start->tag));
        byte *endByte = startByte + size;
        byte *currentByte = startByte;
        w->Print(_u("0x%p[+0x%03x](0x%03x) [%s]:"), startByte, offsetToData, size, annotation);
        for (; currentByte < endByte; ++currentByte)
        {
            if ((currentByte - endByte) % 4 == 0)
            {
                w->Print(_u(" "), *currentByte);
            }
            w->Print(_u("%02x"), *currentByte);
        }
        w->PrintEOL(_u(""));
    }

#define PRINT_BYTES(InstType) \
    Inst::PrintBytes<InstType>(w, (Inst *)this, (InstType *)this, _u(#InstType))

#define PRINT_BYTES_ANNOTATED(InstType, Annotation) \
    Inst::PrintBytes<InstType>(w, (Inst *)this, (InstType *)this, (Annotation))

#define PRINT_MIXIN(Mixin) \
    ((Mixin *)this)->Print(w, litbuf)

#define PRINT_MIXIN_ARGS(Mixin, ...) \
    ((Mixin *)this)->Print(w, litbuf, __VA_ARGS__)

#define PRINT_MIXIN_COMMA(Mixin) \
    PRINT_MIXIN(Mixin); \
    w->Print(_u(", "));

#define PRINT_RE_BYTECODE_BEGIN(Name) \
    w->Print(_u("L%04x: "), label); \
    if (REGEX_CONFIG_FLAG(RegexBytecodeDebug)) \
    { \
        w->Print(_u("(0x%03x bytes) "), sizeof(*this)); \
    } \
    w->Print(_u(Name)); \
    w->Print(_u("("));

#define PRINT_RE_BYTECODE_MID() \
    w->PrintEOL(_u(")")); \
    if (REGEX_CONFIG_FLAG(RegexBytecodeDebug)) \
    { \
        w->Indent(); \
        PRINT_BYTES(Inst);

#define PRINT_RE_BYTECODE_END() \
        w->Unindent(); \
    } \
    return sizeof(*this);

#endif

    // ----------------------------------------------------------------------
    // Mixins
    // ----------------------------------------------------------------------

#if ENABLE_REGEX_CONFIG_OPTIONS
    void BackupMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("backup: "));
        backup.Print(w);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void CharMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("c: "));
        w->PrintQuotedChar(c);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Char2Mixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("c0: "));
        w->PrintQuotedChar(cs[0]);
        w->Print(_u(", c1: "));
        w->PrintQuotedChar(cs[1]);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Char3Mixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("c0: "));
        w->PrintQuotedChar(cs[0]);
        w->Print(_u(", c1: "));
        w->PrintQuotedChar(cs[1]);
        w->Print(_u(", c2: "));
        w->PrintQuotedChar(cs[2]);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Char4Mixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("c0: "));
        w->PrintQuotedChar(cs[0]);
        w->Print(_u(", c1: "));
        w->PrintQuotedChar(cs[1]);
        w->Print(_u(", c2: "));
        w->PrintQuotedChar(cs[2]);
        w->Print(_u(", c3: "));
        w->PrintQuotedChar(cs[3]);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void LiteralMixin::Print(DebugWriter* w, const char16* litbuf, bool isEquivClass) const
    {
        if (isEquivClass)
        {
            w->Print(_u("equivLiterals: "));
            for (int i = 0; i < CaseInsensitive::EquivClassSize; i++)
            {
                if (i > 0)
                {
                    w->Print(_u(", "));
                }
                w->Print(_u("\""));
                for (CharCount j = 0; j < length; j++)
                {
                    w->PrintEscapedChar(litbuf[offset + j * CaseInsensitive::EquivClassSize + i]);
                }
                w->Print(_u("\""));
            }
        }
        else
        {
            w->Print(_u("literal: "));
            w->PrintQuotedString(litbuf + offset, length);
        }
    }
#endif

    // ----------------------------------------------------------------------
    // Char2LiteralScannerMixin
    // ----------------------------------------------------------------------

    bool Char2LiteralScannerMixin::Match(Matcher& matcher, const char16* const input, const CharCount inputLength, CharCount& inputOffset) const
    {
        if (inputLength == 0)
        {
            return false;
        }

        const uint matchC0 = Chars<char16>::CTU(cs[0]);
        const uint matchC1 = Chars<char16>::CTU(cs[1]);

        const char16 * currentInput = input + inputOffset;
        const char16 * endInput = input + inputLength - 1;

        while (currentInput < endInput)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            while (true)
            {
                const uint c1 = Chars<char16>::CTU(currentInput[1]);
                if (c1 != matchC1)
                {
                    if (c1 == matchC0)
                    {
                        break;
                    }
                    currentInput += 2;
                    if (currentInput >= endInput)
                    {
                        return false;
                    }
                    continue;
                }
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.CompStats();
#endif
                // Check the first character
                const uint c0 = Chars<char16>::CTU(*currentInput);
                if (c0 == matchC0)
                {
                    inputOffset = (CharCount)(currentInput - input);
                    return true;
                }
                if (matchC0 == matchC1)
                {
                    break;
                }
                currentInput +=2;
                if (currentInput >= endInput)
                {
                    return false;
                }
            }

            // If the second character in the buffer matches the first in the pattern, continue
            // to see if the next character has the second in the pattern
            currentInput++;
            while (currentInput < endInput)
            {
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.CompStats();
#endif
                const uint c1 = Chars<char16>::CTU(currentInput[1]);
                if (c1 == matchC1)
                {
                    inputOffset = (CharCount)(currentInput - input);
                    return true;
                }
                if (c1 != matchC0)
                {
                    currentInput += 2;
                    break;
                }
                currentInput++;
            }
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Char2LiteralScannerMixin::Print(DebugWriter* w, const char16 * litbuf) const
    {
        Char2Mixin::Print(w, litbuf);
        w->Print(_u(" (with two character literal scanner)"));
    }
#endif

    // ----------------------------------------------------------------------
    // ScannerMixinT
    // ----------------------------------------------------------------------

    template <typename ScannerT>
    void ScannerMixinT<ScannerT>::FreeBody(ArenaAllocator* rtAllocator)
    {
        scanner.FreeBody(rtAllocator, length);
    }

    template <typename ScannerT>
    inline bool
    ScannerMixinT<ScannerT>::Match(Matcher& matcher, const char16 * const input, const CharCount inputLength, CharCount& inputOffset) const
    {
        Assert(length <= matcher.program->rep.insts.litbufLen - offset);
        return scanner.template Match<1>(
            input
            , inputLength
            , inputOffset
            , matcher.program->rep.insts.litbuf + offset
            , length
#if ENABLE_REGEX_CONFIG_OPTIONS
            , matcher.stats
#endif
            );
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template <typename ScannerT>
    void ScannerMixinT<ScannerT>::Print(DebugWriter* w, const char16* litbuf, bool isEquivClass) const
    {
        LiteralMixin::Print(w, litbuf, isEquivClass);
        w->Print(_u(" (with %s scanner)"), ScannerT::GetName());
    }
#endif

    // explicit instantiation
    template struct ScannerMixinT<TextbookBoyerMoore<char16>>;
    template struct ScannerMixinT<TextbookBoyerMooreWithLinearMap<char16>>;

    // ----------------------------------------------------------------------
    // EquivScannerMixinT
    // ----------------------------------------------------------------------

    template <uint lastPatCharEquivClassSize>
    inline bool EquivScannerMixinT<lastPatCharEquivClassSize>::Match(Matcher& matcher, const char16* const input, const CharCount inputLength, CharCount& inputOffset) const
    {
        Assert(length * CaseInsensitive::EquivClassSize <= matcher.program->rep.insts.litbufLen - offset);
        CompileAssert(lastPatCharEquivClassSize >= 1 && lastPatCharEquivClassSize <= CaseInsensitive::EquivClassSize);
        return scanner.Match<CaseInsensitive::EquivClassSize, lastPatCharEquivClassSize>(
            input
            , inputLength
            , inputOffset
            , matcher.program->rep.insts.litbuf + offset
            , length
#if ENABLE_REGEX_CONFIG_OPTIONS
            , matcher.stats
#endif
            );
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template <uint lastPatCharEquivClassSize>
    void EquivScannerMixinT<lastPatCharEquivClassSize>::Print(DebugWriter* w, const char16* litbuf) const
    {
        __super::Print(w, litbuf, true);
        w->Print(_u(" (last char equiv size:%d)"), lastPatCharEquivClassSize);
    }

    // explicit instantiation
    template struct EquivScannerMixinT<1>;
#endif

    // ----------------------------------------------------------------------
    // ScannerInfo
    // ----------------------------------------------------------------------

#if ENABLE_REGEX_CONFIG_OPTIONS
    void ScannerInfo::Print(DebugWriter* w, const char16* litbuf) const
    {
        ScannerMixin::Print(w, litbuf, isEquivClass);
    }
#endif

    ScannerInfo* ScannersMixin::Add(Recycler *recycler, Program *program, CharCount offset, CharCount length, bool isEquivClass)
    {
        Assert(numLiterals < MaxNumSyncLiterals);
        return program->AddScannerForSyncToLiterals(recycler, numLiterals++, offset, length, isEquivClass);
    }

    void ScannersMixin::FreeBody(ArenaAllocator* rtAllocator)
    {
        for (int i = 0; i < numLiterals; i++)
        {
            infos[i]->FreeBody(rtAllocator);
#if DBG
            infos[i] = nullptr;
#endif
        }
#if DBG
        numLiterals = 0;
#endif
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    void ScannersMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("literals: {"));
        for (int i = 0; i < numLiterals; i++)
        {
            if (i > 0)
            {
                w->Print(_u(", "));
            }
            infos[i]->Print(w, litbuf);
        }
        w->Print(_u("}"));
    }
#endif

    template<bool IsNegation>
    void SetMixin<IsNegation>::FreeBody(ArenaAllocator* rtAllocator)
    {
        set.FreeBody(rtAllocator);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<bool IsNegation>
    void SetMixin<IsNegation>::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("set: "));
        if (IsNegation)
        {
            w->Print(_u("not "));
        }
        set.Print(w);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void TrieMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->PrintEOL(_u(""));
        trie.Print(w);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void GroupMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("groupId: %d"), groupId);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void ChompBoundedMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("repeats: "));
        repeats.Print(w);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void JumpMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("targetLabel: L%04x"), Inst::GetPrintLabel(targetLabel));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void BodyGroupsMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("minBodyGroupId: %d, maxBodyGroupId: %d"), minBodyGroupId, maxBodyGroupId);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void BeginLoopBasicsMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("loopId: %d, repeats: "), loopId);
        repeats.Print(w);
        w->Print(_u(", hasOuterLoops: %s"), hasOuterLoops ? _u("true") : _u("false"));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void BeginLoopMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        BeginLoopBasicsMixin::Print(w, litbuf);
        w->Print(_u(", hasInnerNondet: %s, exitLabel: L%04x, "),
            hasInnerNondet ? _u("true") : _u("false"), Inst::GetPrintLabel(exitLabel));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void GreedyMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("greedy: %s"), isGreedy ? _u("true") : _u("false"));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void RepeatLoopMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("beginLabel: L%04x"), Inst::GetPrintLabel(beginLabel));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void GreedyLoopNoBacktrackMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("loopId: %d, exitLabel: L%04x"), loopId, exitLabel);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void TryMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("failLabel: L%04x"), Inst::GetPrintLabel(failLabel));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void NegationMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("isNegation: %s"), isNegation ? _u("true") : _u("false"));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void NextLabelMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("nextLabel: L%04x"), Inst::GetPrintLabel(nextLabel));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void FixedLengthMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("length: %u"), length);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void FollowFirstMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("followFirst: %c"), followFirst);
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void NoNeedToSaveMixin::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->Print(_u("noNeedToSave: %s"), noNeedToSave ? _u("true") : _u("false"));
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void SwitchCase::Print(DebugWriter* w) const
    {
        w->Print(_u("case "));
        w->PrintQuotedChar(c);
        w->PrintEOL(_u(": Jump(L%04x)"), targetLabel);
    }
#endif

    template <uint8 n>
    void SwitchMixin<n>::AddCase(char16 c, Label targetLabel)
    {
        AnalysisAssert(numCases < MaxCases);
        uint8 i;
        for (i = 0; i < numCases; i++)
        {
            Assert(cases[i].c != c);
            if (cases[i].c > c)
            {
                break;
            }
        }
        __analysis_assume(numCases < MaxCases);
        for (uint8 j = numCases; j > i; j--)
        {
            cases[j] = cases[j - 1];
        }
        cases[i].c = c;
        cases[i].targetLabel = targetLabel;
        numCases++;
    }

    void UnifiedRegexSwitchMixinForceAllInstantiations()
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
#define SWITCH_FORCE_INSTANTIATION_PRINT x.Print(0, 0)
#else
#define SWITCH_FORCE_INSTANTIATION_PRINT
#endif

#define SWITCH_FORCE_INSTANTIATION(n)                       \
        {                                                   \
            SwitchMixin<n> x;                               \
            x.AddCase(0, 0);                                \
            SWITCH_FORCE_INSTANTIATION_PRINT;               \
        }

        SWITCH_FORCE_INSTANTIATION(2);
        SWITCH_FORCE_INSTANTIATION(4);
        SWITCH_FORCE_INSTANTIATION(8);
        SWITCH_FORCE_INSTANTIATION(16);
        SWITCH_FORCE_INSTANTIATION(24);

#undef SWITCH_FORCE_INSTANTIATION_PRINT
#undef SWITCH_FORCE_INSTANTIATION
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template <uint8 n>
    void SwitchMixin<n>::Print(DebugWriter* w, const char16* litbuf) const
    {
        w->EOL();
        w->Indent();
        for (uint8 i = 0; i < numCases; i++)
        {
            cases[i].Print(w);
        }
        w->Unindent();
    }
#endif

    // ----------------------------------------------------------------------
    // NopInst
    // ----------------------------------------------------------------------

    inline bool NopInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        return false; // don't stop execution
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int NopInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("Nop");
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(NopInst);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // FailInst
    // ----------------------------------------------------------------------

    inline bool FailInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        return matcher.Fail(FAIL_PARAMETERS);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int FailInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("Fail");
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(NopInst);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SuccInst
    // ----------------------------------------------------------------------

    inline bool SuccInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        GroupInfo* info = matcher.GroupIdToGroupInfo(0);
        info->offset = matchStart;
        info->length = inputOffset - matchStart;
        return true; // STOP MATCHING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SuccInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("Succ");
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(NopInst);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // JumpInst
    // ----------------------------------------------------------------------

    inline bool JumpInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        instPointer = matcher.LabelToInstPointer(targetLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int JumpInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("Jump");
        PRINT_MIXIN(JumpMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(JumpMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // JumpIfNotCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool JumpIfNotCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == c)
        {
            instPointer += sizeof(*this);
        }
        else
        {
            instPointer = matcher.LabelToInstPointer(targetLabel);
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int JumpIfNotCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("JumpIfNotChar");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN(JumpMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(JumpMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchCharOrJumpInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool MatchCharOrJumpInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == c)
        {
            inputOffset++;
            instPointer += sizeof(*this);
        }
        else
        {
            instPointer = matcher.LabelToInstPointer(targetLabel);
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchCharOrJumpInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchCharOrJump");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN(JumpMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(JumpMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // JumpIfNotSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool JumpIfNotSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && set.Get(input[inputOffset]))
        {
            instPointer += sizeof(*this);
        }
        else
        {
            instPointer = matcher.LabelToInstPointer(targetLabel);
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int JumpIfNotSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("JumpIfNotSet");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN(JumpMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(JumpMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchSetOrJumpInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool MatchSetOrJumpInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && set.Get(input[inputOffset]))
        {
            inputOffset++;
            instPointer += sizeof(*this);
        }
        else
        {
            instPointer = matcher.LabelToInstPointer(targetLabel);
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchSetOrJumpInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchSetOrJump");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN(JumpMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(JumpMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // Switch(AndConsume)Inst (optimized instructions)
    // ----------------------------------------------------------------------

#if ENABLE_REGEX_CONFIG_OPTIONS
#define COMP_STATS matcher.CompStats()
#define SwitchAndConsumeInstPrintImpl(BaseName, n)                                              \
    int BaseName##n##Inst::Print(DebugWriter* w, Label label, const Char* litbuf) const         \
    {                                                                                           \
        PRINT_RE_BYTECODE_BEGIN("SwitchAndConsume"#n);                                          \
        PRINT_MIXIN(SwitchMixin<n>);                                                            \
        PRINT_RE_BYTECODE_MID();                                                                \
        PRINT_BYTES(SwitchMixin<n>);                                                            \
        PRINT_RE_BYTECODE_END();                                                                \
    }
#else
#define COMP_STATS
#define SwitchAndConsumeInstPrintImpl(BaseName, n)
#endif

#define SwitchAndConsumeInstImpl(BaseName, n) \
    inline bool BaseName##n##Inst::Exec(REGEX_INST_EXEC_PARAMETERS) const                       \
    {                                                                                           \
        if (inputOffset >= inputLength)                                                         \
        {                                                                                       \
            return matcher.Fail(FAIL_PARAMETERS);                                               \
        }                                                                                       \
                                                                                                \
        const uint8 localNumCases = numCases;                                                     \
        for (int i = 0; i < localNumCases; i++)                                                 \
        {                                                                                       \
            COMP_STATS;                                                                         \
            if (cases[i].c == input[inputOffset])                                               \
            {                                                                                   \
                CONSUME;                                                                        \
                instPointer = matcher.LabelToInstPointer(cases[i].targetLabel);                 \
                return false;                                                                   \
            }                                                                                   \
            else if (cases[i].c > input[inputOffset])                                           \
            {                                                                                   \
                break;                                                                          \
            }                                                                                   \
        }                                                                                       \
                                                                                                \
        instPointer += sizeof(*this);                                                           \
        return false;                                                                           \
    }                                                                                           \
    SwitchAndConsumeInstPrintImpl(BaseName, n);

#define CONSUME
    SwitchAndConsumeInstImpl(Switch, 2);
    SwitchAndConsumeInstImpl(Switch, 4);
    SwitchAndConsumeInstImpl(Switch, 8);
    SwitchAndConsumeInstImpl(Switch, 16);
    SwitchAndConsumeInstImpl(Switch, 24);
#undef CONSUME

#define CONSUME inputOffset++
    SwitchAndConsumeInstImpl(SwitchAndConsume, 2);
    SwitchAndConsumeInstImpl(SwitchAndConsume, 4);
    SwitchAndConsumeInstImpl(SwitchAndConsume, 8);
    SwitchAndConsumeInstImpl(SwitchAndConsume, 16);
    SwitchAndConsumeInstImpl(SwitchAndConsume, 24);
#undef CONSUME

#undef COMP_STATS
#undef SwitchAndConsumeInstPrintImpl
#undef SwitchAndConsumeInstImpl

    // ----------------------------------------------------------------------
    // BOITestInst
    // ----------------------------------------------------------------------

    template <>
    BOITestInst<true>::BOITestInst() : Inst(InstTag::BOIHardFailTest) {}
    template <>
    BOITestInst<false>::BOITestInst() : Inst(InstTag::BOITest) {}

    template <bool canHardFail>
    inline bool BOITestInst<canHardFail>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (inputOffset > 0)
        {
            if (canHardFail)
            {
                // Clearly trying to start from later in the input won't help, and we know backtracking can't take us earlier in the input
                return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
            }
            else
            {
                return matcher.Fail(FAIL_PARAMETERS);
            }
        }
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template <bool canHardFail>
    int BOITestInst<canHardFail>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (canHardFail)
        {
            PRINT_RE_BYTECODE_BEGIN("BOIHardFailTest");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("BOITest");
        }

        w->Print(_u("<hardFail>: %s"), canHardFail ? _u("true") : _u("false"));

        PRINT_RE_BYTECODE_MID();
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // EOITestInst
    // ----------------------------------------------------------------------

    template <>
    EOITestInst<true>::EOITestInst() : Inst(InstTag::EOIHardFailTest) {}
    template <>
    EOITestInst<false>::EOITestInst() : Inst(InstTag::EOITest) {}

    template <bool canHardFail>
    inline bool EOITestInst<canHardFail>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (inputOffset < inputLength)
        {
            if (canHardFail)
            {
                // We know backtracking can never take us later in the input, but starting from later in the input could help
                return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::LaterOnly));
            }
            else
            {
                return matcher.Fail(FAIL_PARAMETERS);
            }
        }
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template <bool canHardFail>
    int EOITestInst<canHardFail>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (canHardFail)
        {
            PRINT_RE_BYTECODE_BEGIN("EOIHardFailTest");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("EOITest");
        }

        w->Print(_u("<hardFail>: %s"), canHardFail ? _u("true") : _u("false"));

        PRINT_RE_BYTECODE_MID();
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BOLTestInst
    // ----------------------------------------------------------------------

    inline bool BOLTestInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset > 0 && !matcher.standardChars->IsNewline(input[inputOffset - 1]))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BOLTestInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BOLTest");
        PRINT_RE_BYTECODE_MID();
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // EOLTestInst
    // ----------------------------------------------------------------------

    inline bool EOLTestInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && !matcher.standardChars->IsNewline(input[inputOffset]))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int EOLTestInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("EOLTest");
        PRINT_RE_BYTECODE_MID();
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // WordBoundaryTestInst
    // ----------------------------------------------------------------------

    template <>
    WordBoundaryTestInst<true>::WordBoundaryTestInst() : Inst(InstTag::NegatedWordBoundaryTest) {}
    template <>
    WordBoundaryTestInst<false>::WordBoundaryTestInst() : Inst(InstTag::WordBoundaryTest) {}

    template <bool isNegation>
    inline bool WordBoundaryTestInst<isNegation>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        const bool prev = inputOffset > 0 && matcher.standardChars->IsWord(input[inputOffset - 1]);
        const bool curr = inputOffset < inputLength && matcher.standardChars->IsWord(input[inputOffset]);
        if (isNegation == (prev != curr))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template <bool isNegation>
    int WordBoundaryTestInst<isNegation>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (isNegation)
        {
            PRINT_RE_BYTECODE_BEGIN("NegatedWordBoundaryTest");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("WordBoundaryTest");
        }

        PRINT_RE_BYTECODE_MID();
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchCharInst
    // ----------------------------------------------------------------------

    inline bool MatchCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset >= inputLength || input[inputOffset] != c)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchChar");
        PRINT_MIXIN(CharMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchChar2Inst
    // ----------------------------------------------------------------------

    inline bool MatchChar2Inst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset >= inputLength || (input[inputOffset] != cs[0] && input[inputOffset] != cs[1]))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchChar2Inst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchChar2");
        PRINT_MIXIN(Char2Mixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char2Mixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchChar3Inst
    // ----------------------------------------------------------------------

    inline bool MatchChar3Inst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset >= inputLength || (input[inputOffset] != cs[0] && input[inputOffset] != cs[1] && input[inputOffset] != cs[2]))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchChar3Inst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchChar3");
        PRINT_MIXIN(Char3Mixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char3Mixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchChar4Inst
    // ----------------------------------------------------------------------

    inline bool MatchChar4Inst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset >= inputLength || (input[inputOffset] != cs[0] && input[inputOffset] != cs[1] && input[inputOffset] != cs[2] && input[inputOffset] != cs[3]))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchChar4Inst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchChar4");
        PRINT_MIXIN(Char4Mixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char4Mixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchSetInst
    // ----------------------------------------------------------------------

    template<bool IsNegation>
    inline bool MatchSetInst<IsNegation>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset >= inputLength || this->set.Get(input[inputOffset]) == IsNegation)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<bool IsNegation>
    int MatchSetInst<IsNegation>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (IsNegation)
        {
            PRINT_RE_BYTECODE_BEGIN("MatchNegatedSet");
            PRINT_MIXIN(SetMixin<true>);
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("MatchSet");
            PRINT_MIXIN(SetMixin<false>);
        }

        PRINT_RE_BYTECODE_MID();
        IsNegation ? PRINT_BYTES(SetMixin<true>) : PRINT_BYTES(SetMixin<false>);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchLiteralInst
    // ----------------------------------------------------------------------

    inline bool MatchLiteralInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        Assert(length <= matcher.program->rep.insts.litbufLen - offset);

        if (length > inputLength - inputOffset)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        const Char *const literalBuffer = matcher.program->rep.insts.litbuf;
        const Char * literalCurr = literalBuffer + offset;
        const Char * inputCurr = input + inputOffset;

#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (*literalCurr != *inputCurr)
        {
            inputOffset++;
            return matcher.Fail(FAIL_PARAMETERS);
        }

        const Char *const literalEnd = literalCurr + length;
        literalCurr++;
        inputCurr++;

        while (literalCurr < literalEnd)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            if (*literalCurr != *inputCurr++)
            {
                inputOffset = (CharCount)(inputCurr - input);
                return matcher.Fail(FAIL_PARAMETERS);
            }
            literalCurr++;
        }

        inputOffset = (CharCount)(inputCurr - input);
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchLiteralInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchLiteral");
        PRINT_MIXIN_ARGS(LiteralMixin, false);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(LiteralMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchLiteralEquivInst
    // ----------------------------------------------------------------------

    inline bool MatchLiteralEquivInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (length > inputLength - inputOffset)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        const Char *const literalBuffer = matcher.program->rep.insts.litbuf;
        CharCount literalOffset = offset;
        const CharCount literalEndOffset = offset + length * CaseInsensitive::EquivClassSize;

        Assert(literalEndOffset <= matcher.program->rep.insts.litbufLen);
        CompileAssert(CaseInsensitive::EquivClassSize == 4);

        do
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            if (input[inputOffset] != literalBuffer[literalOffset]
                && input[inputOffset] != literalBuffer[literalOffset + 1]
                && input[inputOffset] != literalBuffer[literalOffset + 2]
                && input[inputOffset] != literalBuffer[literalOffset + 3])
            {
                return matcher.Fail(FAIL_PARAMETERS);
            }
            inputOffset++;
            literalOffset += CaseInsensitive::EquivClassSize;
        }
        while (literalOffset < literalEndOffset);

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchLiteralEquivInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchLiteralEquiv");
        PRINT_MIXIN_ARGS(LiteralMixin, true);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(LiteralMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchTrieInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool MatchTrieInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (!trie.Match(
            input
            , inputLength
            , inputOffset
#if ENABLE_REGEX_CONFIG_OPTIONS
            , matcher.stats
#endif
        ))
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer += sizeof(*this);
        return false;
    }

    void MatchTrieInst::FreeBody(ArenaAllocator* rtAllocator)
    {
        trie.FreeBody(rtAllocator);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchTrieInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchTrie");
        PRINT_MIXIN(TrieMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(TrieMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // OptMatchCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool OptMatchCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == c)
        {
            inputOffset++;
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int OptMatchCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("OptMatchChar");
        PRINT_MIXIN(CharMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // OptMatchSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool OptMatchSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && set.Get(input[inputOffset]))
        {
            inputOffset++;
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int OptMatchSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("OptMatchSet");
        PRINT_MIXIN(SetMixin<false>);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToCharAndContinueInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool SyncToCharAndContinueInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const Char matchC = c;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputLength && input[inputOffset] != matchC)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        matchStart = inputOffset;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SyncToCharAndContinueInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("SyncToCharAndContinue");
        PRINT_MIXIN(CharMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToChar2SetAndContinueInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool SyncToChar2SetAndContinueInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const Char matchC0 = cs[0];
        const Char matchC1 = cs[1];
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputLength && input[inputOffset] != matchC0 && input[inputOffset] != matchC1)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        matchStart = inputOffset;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SyncToChar2SetAndContinueInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("SyncToChar2SetAndContinue");
        PRINT_MIXIN(Char2Mixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char2Mixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToSetAndContinueInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<bool IsNegation>
    inline bool SyncToSetAndContinueInst<IsNegation>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const RuntimeCharSet<Char>& matchSet = this->set;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif

        while (inputOffset < inputLength && matchSet.Get(input[inputOffset]) == IsNegation)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        matchStart = inputOffset;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<bool IsNegation>
    int SyncToSetAndContinueInst<IsNegation>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (IsNegation)
        {
            PRINT_RE_BYTECODE_BEGIN("SyncToNegatedSetAndContinue");
            PRINT_MIXIN(SetMixin<true>);
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("SyncToSetAndContinue");
            PRINT_MIXIN(SetMixin<false>);
        }

        PRINT_RE_BYTECODE_MID();
        IsNegation ? PRINT_BYTES(SetMixin<true>) : PRINT_BYTES(SetMixin<false>);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToLiteralAndContinueInst (optimized instruction)
    // ----------------------------------------------------------------------

    template <typename ScannerT>
    inline bool SyncToLiteralAndContinueInstT<ScannerT>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (!this->Match(matcher, input, inputLength, inputOffset))
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        matchStart = inputOffset;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    // explicit instantiation
    template struct SyncToLiteralAndContinueInstT<Char2LiteralScannerMixin>;
    template struct SyncToLiteralAndContinueInstT<ScannerMixin>;
    template struct SyncToLiteralAndContinueInstT<ScannerMixin_WithLinearCharMap>;
    template struct SyncToLiteralAndContinueInstT<EquivScannerMixin>;
    template struct SyncToLiteralAndContinueInstT<EquivTrivialLastPatCharScannerMixin>;

    // Explicitly define each of these 5 Print functions so that the output will show the actual template param mixin and
    // actual opcode name, even though the logic is basically the same in each definition. See notes below.

    template <>
    int SyncToLiteralAndContinueInstT<Char2LiteralScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndContinueInstT<Char2LiteralScannerMixin> aka SyncToChar2LiteralAndContinue");
        PRINT_MIXIN(Char2LiteralScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char2LiteralScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndContinueInstT<ScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndContinueInstT<ScannerMixin> aka SyncToLiteralAndContinue");
        PRINT_MIXIN(ScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndContinueInstT<ScannerMixin_WithLinearCharMap>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndContinueInstT<ScannerMixin_WithLinearCharMap> aka SyncToLinearLiteralAndContinue");
        PRINT_MIXIN(ScannerMixin_WithLinearCharMap); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannerMixin_WithLinearCharMap); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndContinueInstT<EquivScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndContinueInstT<EquivScannerMixin> aka SyncToLiteralEquivAndContinue");
        PRINT_MIXIN(EquivScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EquivScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndContinueInstT<EquivTrivialLastPatCharScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndContinueInstT<EquivTrivialLastPatCharScannerMixin> aka SyncToLiteralEquivTrivialLastPatCharAndContinue");
        PRINT_MIXIN(EquivTrivialLastPatCharScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EquivTrivialLastPatCharScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToCharAndConsumeInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool SyncToCharAndConsumeInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const Char matchC = c;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputLength && input[inputOffset] != matchC)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset >= inputLength)
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        matchStart = inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SyncToCharAndConsumeInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("SyncToCharAndConsume");
        PRINT_MIXIN(CharMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToChar2SetAndConsumeInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool SyncToChar2SetAndConsumeInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const Char matchC0 = cs[0];
        const Char matchC1 = cs[1];
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputLength && (input[inputOffset] != matchC0 && input[inputOffset] != matchC1))
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset >= inputLength)
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        matchStart = inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SyncToChar2SetAndConsumeInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("SyncToChar2SetAndConsume");
        PRINT_MIXIN(Char2Mixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char2Mixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToSetAndConsumeInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<bool IsNegation>
    inline bool SyncToSetAndConsumeInst<IsNegation>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const RuntimeCharSet<Char>& matchSet = this->set;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputLength && matchSet.Get(input[inputOffset]) == IsNegation)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset >= inputLength)
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        matchStart = inputOffset++;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<bool IsNegation>
    int SyncToSetAndConsumeInst<IsNegation>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (IsNegation)
        {
            PRINT_RE_BYTECODE_BEGIN("SyncToNegatedSetAndConsume");
            PRINT_MIXIN(SetMixin<true>);
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("SyncToSetAndConsume");
            PRINT_MIXIN(SetMixin<false>);
        }

        PRINT_RE_BYTECODE_MID();
        IsNegation ? PRINT_BYTES(SetMixin<true>) : PRINT_BYTES(SetMixin<false>);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToLiteralAndConsumeInst (optimized instruction)
    // ----------------------------------------------------------------------

    template <typename ScannerT>
    inline bool SyncToLiteralAndConsumeInstT<ScannerT>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (!this->Match(matcher, input, inputLength, inputOffset))
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        matchStart = inputOffset;
        inputOffset += ScannerT::GetLiteralLength();
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    // explicit instantiation
    template struct SyncToLiteralAndConsumeInstT<Char2LiteralScannerMixin>;
    template struct SyncToLiteralAndConsumeInstT<ScannerMixin>;
    template struct SyncToLiteralAndConsumeInstT<ScannerMixin_WithLinearCharMap>;
    template struct SyncToLiteralAndConsumeInstT<EquivScannerMixin>;
    template struct SyncToLiteralAndConsumeInstT<EquivTrivialLastPatCharScannerMixin>;

    // Explicitly define each of these 5 Print functions so that the output will show the actual template param mixin and
    // actual opcode name, even though the logic is basically the same in each definition. See notes below.

    template <>
    int SyncToLiteralAndConsumeInstT<Char2LiteralScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndConsumeInstT<Char2LiteralScannerMixin> aka SyncToChar2LiteralAndConsume");
        PRINT_MIXIN(Char2LiteralScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char2LiteralScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndConsumeInstT<ScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndConsumeInstT<ScannerMixin> aka SyncToLiteralAndConsume");
        PRINT_MIXIN(ScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndConsumeInstT<ScannerMixin_WithLinearCharMap>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndConsumeInstT<ScannerMixin_WithLinearCharMap> aka SyncToLinearLiteralAndConsume");
        PRINT_MIXIN(ScannerMixin_WithLinearCharMap); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannerMixin_WithLinearCharMap); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndConsumeInstT<EquivScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndConsumeInstT<EquivScannerMixin> aka SyncToLiteralEquivAndConsume");
        PRINT_MIXIN(EquivScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EquivScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndConsumeInstT<EquivTrivialLastPatCharScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndConsumeInstT<EquivTrivialLastPatCharScannerMixin> aka SyncToLiteralEquivTrivialLastPatCharAndConsume");
        PRINT_MIXIN(EquivTrivialLastPatCharScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EquivTrivialLastPatCharScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToCharAndBackupInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool SyncToCharAndBackupInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (backup.lower > inputLength - matchStart)
        {
            // Even match at very end doesn't allow for minimum backup
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        if (inputOffset < nextSyncInputOffset)
        {
            // We have not yet reached the offset in the input we last synced to before backing up, so it's unnecessary to sync
            // again since we'll sync to the same point in the input and back up to the same place we are at now
            instPointer += sizeof(*this);
            return false;
        }

        if (backup.lower > inputOffset - matchStart)
        {
            // No use looking for match until minimum backup is possible
            inputOffset = matchStart + backup.lower;
        }

        const Char matchC = c;
        while (inputOffset < inputLength && input[inputOffset] != matchC)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset >= inputLength)
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        nextSyncInputOffset = inputOffset + 1;

        if (backup.upper != CharCountFlag)
        {
            // Backup at most by backup.upper for new start
            CharCount maxBackup = inputOffset - matchStart;
            matchStart = inputOffset - min(maxBackup, (CharCount)backup.upper);
        }
        // else: leave start where it is

        // Move input to new match start
        inputOffset = matchStart;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SyncToCharAndBackupInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("SyncToCharAndBackup");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToSetAndBackupInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<bool IsNegation>
    inline bool SyncToSetAndBackupInst<IsNegation>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (backup.lower > inputLength - matchStart)
        {
            // Even match at very end doesn't allow for minimum backup
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        if (inputOffset < nextSyncInputOffset)
        {
            // We have not yet reached the offset in the input we last synced to before backing up, so it's unnecessary to sync
            // again since we'll sync to the same point in the input and back up to the same place we are at now
            instPointer += sizeof(*this);
            return false;
        }

        if (backup.lower > inputOffset - matchStart)
        {
            // No use looking for match until minimum backup is possible
            inputOffset = matchStart + backup.lower;
        }

        const RuntimeCharSet<Char>& matchSet = this->set;
        while (inputOffset < inputLength && matchSet.Get(input[inputOffset]) == IsNegation)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset >= inputLength)
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        nextSyncInputOffset = inputOffset + 1;

        if (backup.upper != CharCountFlag)
        {
            // Backup at most by backup.upper for new start
            CharCount maxBackup = inputOffset - matchStart;
            matchStart = inputOffset - min(maxBackup, (CharCount)backup.upper);
        }
        // else: leave start where it is

        // Move input to new match start
        inputOffset = matchStart;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<bool IsNegation>
    int SyncToSetAndBackupInst<IsNegation>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (IsNegation)
        {
            PRINT_RE_BYTECODE_BEGIN("SyncToNegatedSetAndBackup");
            PRINT_MIXIN_COMMA(SetMixin<true>);
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("SyncToSetAndBackup");
            PRINT_MIXIN_COMMA(SetMixin<false>);
        }

        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        IsNegation ? PRINT_BYTES(SetMixin<true>) : PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToLiteralAndBackupInst (optimized instruction)
    // ----------------------------------------------------------------------
    template <typename ScannerT>
    inline bool SyncToLiteralAndBackupInstT<ScannerT>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (backup.lower > inputLength - matchStart)
        {
            // Even match at very end doesn't allow for minimum backup
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        if(inputOffset < nextSyncInputOffset)
        {
            // We have not yet reached the offset in the input we last synced to before backing up, so it's unnecessary to sync
            // again since we'll sync to the same point in the input and back up to the same place we are at now
            instPointer += sizeof(*this);
            return false;
        }

        if (backup.lower > inputOffset - matchStart)
        {
            // No use looking for match until minimum backup is possible
            inputOffset = matchStart + backup.lower;
        }

        if (!this->Match(matcher, input, inputLength, inputOffset))
        {
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        nextSyncInputOffset = inputOffset + 1;

        if (backup.upper != CharCountFlag)
        {
            // Set new start at most backup.upper from start of literal
            CharCount maxBackup = inputOffset - matchStart;
            matchStart = inputOffset - min(maxBackup, (CharCount)backup.upper);
        }
        // else: leave start where it is

        // Move input to new match start
        inputOffset = matchStart;

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    // explicit instantiation
    template struct SyncToLiteralAndBackupInstT<Char2LiteralScannerMixin>;
    template struct SyncToLiteralAndBackupInstT<ScannerMixin>;
    template struct SyncToLiteralAndBackupInstT<ScannerMixin_WithLinearCharMap>;
    template struct SyncToLiteralAndBackupInstT<EquivScannerMixin>;
    template struct SyncToLiteralAndBackupInstT<EquivTrivialLastPatCharScannerMixin>;

    // Explicitly define each of these 5 Print functions so that the output will show the actual template param mixin and
    // actual opcode name, even though the logic is basically the same in each definition. See notes below.

    template <>
    int SyncToLiteralAndBackupInstT<Char2LiteralScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndBackupInstT<Char2LiteralScannerMixin> aka SyncToChar2LiteralAndBackup");
        PRINT_MIXIN_COMMA(Char2LiteralScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(Char2LiteralScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndBackupInstT<ScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndBackupInstT<ScannerMixin> aka SyncToLiteralAndBackup");
        PRINT_MIXIN_COMMA(ScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndBackupInstT<ScannerMixin_WithLinearCharMap>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndBackupInstT<ScannerMixin_WithLinearCharMap> aka SyncToLinearLiteralAndBackup");
        PRINT_MIXIN_COMMA(ScannerMixin_WithLinearCharMap); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannerMixin_WithLinearCharMap); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndBackupInstT<EquivScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndBackupInstT<EquivScannerMixin> aka SyncToLiteralEquivAndBackup");
        PRINT_MIXIN_COMMA(EquivScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EquivScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }

    template <>
    int SyncToLiteralAndBackupInstT<EquivTrivialLastPatCharScannerMixin>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        // NOTE: this text is unique to this instantiation
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralAndBackupInstT<EquivTrivialLastPatCharScannerMixin> aka SyncToLiteralEquivTrivialLastPatCharAndBackup");
        PRINT_MIXIN_COMMA(EquivTrivialLastPatCharScannerMixin); // NOTE: would work with template <typename ScannerT> ScannerT::Print
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EquivTrivialLastPatCharScannerMixin); // NOTE: unique because macro expansion and _u(#InstType) happen before template is evaluated (so text would be ScannerT)
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // SyncToLiteralsAndBackupInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool SyncToLiteralsAndBackupInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (backup.lower > inputLength - matchStart)
        {
            // Even match at very end doesn't allow for minimum backup
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        if (inputOffset < nextSyncInputOffset)
        {
            // We have not yet reached the offset in the input we last synced to before backing up, so it's unnecessary to sync
            // again since we'll sync to the same point in the input and back up to the same place we are at now
            instPointer += sizeof(*this);
            return false;
        }

        if (backup.lower > inputOffset - matchStart)
        {
            // No use looking for match until minimum backup is possible
            inputOffset = matchStart + backup.lower;
        }

        int besti = -1;
        CharCount bestMatchOffset = 0;

        if (matcher.literalNextSyncInputOffsets == nullptr)
        {
            Assert(numLiterals <= MaxNumSyncLiterals);
            matcher.literalNextSyncInputOffsets =
                RecyclerNewArrayLeaf(matcher.recycler, CharCount, ScannersMixin::MaxNumSyncLiterals);
        }
        CharCount* literalNextSyncInputOffsets = matcher.literalNextSyncInputOffsets;

        if (firstIteration)
        {
            for (int i = 0; i < numLiterals; i++)
            {
                literalNextSyncInputOffsets[i] = inputOffset;
            }
        }

        for (int i = 0; i < numLiterals; i++)
        {
            CharCount thisMatchOffset = literalNextSyncInputOffsets[i];
            if (inputOffset > thisMatchOffset)
            {
                thisMatchOffset = inputOffset;
            }

            if (infos[i]->isEquivClass
                ? (infos[i]->scanner.Match<CaseInsensitive::EquivClassSize>(
                    input
                    , inputLength
                    , thisMatchOffset
                    , matcher.program->rep.insts.litbuf + infos[i]->offset
                    , infos[i]->length
#if ENABLE_REGEX_CONFIG_OPTIONS
                    , matcher.stats
#endif
                    ))
                : (infos[i]->scanner.Match<1>(
                    input
                    , inputLength
                    , thisMatchOffset
                    , matcher.program->rep.insts.litbuf + infos[i]->offset
                    , infos[i]->length
#if ENABLE_REGEX_CONFIG_OPTIONS
                    , matcher.stats
#endif
                    )))
            {
                if (besti < 0 || thisMatchOffset < bestMatchOffset)
                {
                    besti = i;
                    bestMatchOffset = thisMatchOffset;
                }

                literalNextSyncInputOffsets[i] = thisMatchOffset;
            }
            else
            {
                literalNextSyncInputOffsets[i] = inputLength;
            }
        }

        if (besti < 0)
        {
            // No literals matched
            return matcher.HardFail(HARDFAIL_PARAMETERS(HardFailMode::ImmediateFail));
        }

        nextSyncInputOffset = bestMatchOffset + 1;

        if (backup.upper != CharCountFlag)
        {
            // Set new start at most backup.upper from start of literal
            CharCount maxBackup = bestMatchOffset - matchStart;
            matchStart = bestMatchOffset - min(maxBackup, (CharCount)backup.upper);
        }
        // else: leave start where it is

        // Move input to new match start
        inputOffset = matchStart;
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int SyncToLiteralsAndBackupInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("SyncToLiteralsAndBackup");
        PRINT_MIXIN_COMMA(ScannersMixin);
        PRINT_MIXIN(BackupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(ScannersMixin);
        PRINT_BYTES(BackupMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // MatchGroupInst
    // ----------------------------------------------------------------------

    inline bool MatchGroupInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        GroupInfo* const info = matcher.GroupIdToGroupInfo(groupId);
        if (!info->IsUndefined() && info->length > 0)
        {
            if (info->length > inputLength - inputOffset)
            {
                return matcher.Fail(FAIL_PARAMETERS);
            }

            CharCount groupOffset = info->offset;
            const CharCount groupEndOffset = groupOffset + info->length;

            bool isCaseInsensitiveMatch = (matcher.program->flags & IgnoreCaseRegexFlag) != 0;
            bool isCodePointList = (matcher.program->flags & UnicodeRegexFlag) != 0;

            // This is the only place in the runtime machinery we need to convert characters to their equivalence class
            if (isCaseInsensitiveMatch && isCodePointList)
            {
                auto getNextCodePoint = [=](CharCount &offset, CharCount endOffset, codepoint_t &codePoint) {
                    if (endOffset <= offset)
                    {
                        return false;
                    }

                    Char lowerPart = input[offset];
                    if (!Js::NumberUtilities::IsSurrogateLowerPart(lowerPart) || offset + 1 == endOffset)
                    {
                        codePoint = lowerPart;
                        offset += 1;
                        return true;
                    }

                    Char upperPart = input[offset + 1];
                    if (!Js::NumberUtilities::IsSurrogateUpperPart(upperPart))
                    {
                        codePoint = lowerPart;
                        offset += 1;
                    }
                    else
                    {
                        codePoint = Js::NumberUtilities::SurrogatePairAsCodePoint(lowerPart, upperPart);
                        offset += 2;
                    }

                    return true;
                };

                codepoint_t equivs[CaseInsensitive::EquivClassSize];
                while (true)
                {
                    codepoint_t groupCodePoint;
                    bool hasGroupCodePoint = getNextCodePoint(groupOffset, groupEndOffset, groupCodePoint);
                    if (!hasGroupCodePoint)
                    {
                        break;
                    }

                    // We don't need to verify that there is a valid input code point since at the beginning
                    // of the function, we make sure that the length of the input is at least as long as the
                    // length of the group.
                    codepoint_t inputCodePoint;
                    getNextCodePoint(inputOffset, inputLength, inputCodePoint);

                    bool doesMatch = false;
                    if (!Js::NumberUtilities::IsInSupplementaryPlane(groupCodePoint))
                    {
                        auto toCanonical = [&](codepoint_t c) {
                            return matcher.standardChars->ToCanonical(
                                CaseInsensitive::MappingSource::CaseFolding,
                                static_cast<char16>(c));
                        };
                        doesMatch = (toCanonical(groupCodePoint) == toCanonical(inputCodePoint));
                    }
                    else
                    {
                        uint tblidx = 0;
                        uint acth = 0;
                        CaseInsensitive::RangeToEquivClass(tblidx, groupCodePoint, groupCodePoint, acth, equivs);
                        CompileAssert(CaseInsensitive::EquivClassSize == 4);
                        doesMatch =
                            inputCodePoint == equivs[0]
                            || inputCodePoint == equivs[1]
                            || inputCodePoint == equivs[2]
                            || inputCodePoint == equivs[3];
                    }

                    if (!doesMatch)
                    {
                        return matcher.Fail(FAIL_PARAMETERS);
                    }
                }
            }
            else if (isCaseInsensitiveMatch)
            {
                do
                {
#if ENABLE_REGEX_CONFIG_OPTIONS
                    matcher.CompStats();
#endif
                    auto toCanonical = [&](CharCount &offset) {
                        return matcher.standardChars->ToCanonical(CaseInsensitive::MappingSource::UnicodeData, input[offset++]);
                    };

                    if (toCanonical(groupOffset) != toCanonical(inputOffset))
                    {
                        return matcher.Fail(FAIL_PARAMETERS);
                    }
                }
                while (groupOffset < groupEndOffset);
            }
            else
            {
                do
                {
#if ENABLE_REGEX_CONFIG_OPTIONS
                    matcher.CompStats();
#endif
                    if (input[groupOffset++] != input[inputOffset++])
                    {
                        return matcher.Fail(FAIL_PARAMETERS);
                    }
                }
                while (groupOffset < groupEndOffset);
            }
        }
        // else: trivially match empty string

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int MatchGroupInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("MatchGroup");
        PRINT_MIXIN(GroupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(GroupMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginDefineGroupInst
    // ----------------------------------------------------------------------

    inline bool BeginDefineGroupInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        GroupInfo *const groupInfo = matcher.GroupIdToGroupInfo(groupId);
        Assert(groupInfo->IsUndefined());
        groupInfo->offset = inputOffset;
        Assert(groupInfo->IsUndefined());

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginDefineGroupInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginDefineGroup");
        PRINT_MIXIN(GroupMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(GroupMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // EndDefineGroupInst
    // ----------------------------------------------------------------------

    inline bool EndDefineGroupInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (!noNeedToSave)
        {
            // UNDO ACTION: Restore group on backtrack
            PUSH(contStack, ResetGroupCont, groupId);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        GroupInfo *const groupInfo = matcher.GroupIdToGroupInfo(groupId);
        Assert(groupInfo->IsUndefined());
        Assert(inputOffset >= groupInfo->offset);
        groupInfo->length = inputOffset - groupInfo->offset;

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int EndDefineGroupInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("EndDefineGroup");
        PRINT_MIXIN_COMMA(GroupMixin);
        PRINT_MIXIN(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(GroupMixin);
        PRINT_BYTES(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // DefineGroupFixedInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool DefineGroupFixedInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (!noNeedToSave)
        {
            // UNDO ACTION: Restore group on backtrack
            PUSH(contStack, ResetGroupCont, groupId);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        GroupInfo *const groupInfo = matcher.GroupIdToGroupInfo(groupId);
        Assert(groupInfo->IsUndefined());
        groupInfo->offset = inputOffset - length;
        groupInfo->length = length;

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int DefineGroupFixedInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("DefineGroupFixed");
        PRINT_MIXIN_COMMA(GroupMixin);
        PRINT_MIXIN_COMMA(FixedLengthMixin);
        PRINT_MIXIN(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(GroupMixin);
        PRINT_BYTES(FixedLengthMixin);
        PRINT_BYTES(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginLoopInst
    // ----------------------------------------------------------------------

    inline bool BeginLoopInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

        // If loop has outer loops, the continuation stack may have choicepoints from an earlier "run" of this loop
        // which, when backtracked to, may expect the loopInfo state to be as it was at the time the choicepoint was
        // pushed.
        //  - If the loop is greedy with deterministic body, there may be Resumes into the follow of the loop, but
        //    they won't look at the loopInfo state so there's nothing to do.
        //  - If the loop is greedy, or if it is non-greedy with lower > 0, AND it has a non-deterministic body,
        //    we may have Resume entries which will resume inside the loop body, which may then run to a
        //    RepeatLoop, which will then look at the loopInfo state. However, each iteration is protected by
        //    a RestoreLoop by RepeatLoopInst below. (****)
        //  - If the loop is non-greedy there may be a RepeatLoop on the stack, so we must restore the loopInfo
        //    state before backtracking to it.
        if (!isGreedy && hasOuterLoops)
        {
            PUSH(contStack, RestoreLoopCont, loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        // The loop body must always begin with empty inner groups
        //  - if the loop is not in an outer they will be empty due to the reset when the match began
        //  - if the loop is in an outer loop, they will have been reset by the outer loop's RepeatLoop instruction
#if DBG
        for (int i = minBodyGroupId; i <= maxBodyGroupId; i++)
        {
            Assert(matcher.GroupIdToGroupInfo(i)->IsUndefined());
        }
#endif

        loopInfo->number = 0;
        loopInfo->startInputOffset = inputOffset;

        if (repeats.lower == 0)
        {
            if (isGreedy)
            {
                // CHOICEPOINT: Try one iteration of body, if backtrack continue from here with no iterations
                PUSH(contStack, ResumeCont, inputOffset, exitLabel);
                instPointer += sizeof(*this);
            }
            else
            {
                // CHOICEPOINT: Try no iterations of body, if backtrack do one iteration of body from here
                Assert(instPointer == (uint8*)this);
                PUSH(contStack, RepeatLoopCont, matcher.InstPointerToLabel(instPointer), inputOffset);
                instPointer = matcher.LabelToInstPointer(exitLabel);
            }
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }
        else
        {
            // Must match minimum iterations, so continue to loop body
            instPointer += sizeof(*this);
        }

        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginLoopInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginLoop");
        PRINT_MIXIN_COMMA(BeginLoopMixin);
        PRINT_MIXIN_COMMA(BodyGroupsMixin);
        PRINT_MIXIN(GreedyMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(BeginLoopMixin);
        PRINT_BYTES(BodyGroupsMixin);
        PRINT_BYTES(GreedyMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatLoopInst
    // ----------------------------------------------------------------------

    inline bool RepeatLoopInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        BeginLoopInst* begin = matcher.L2I(BeginLoop, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        // See comment (****) above.
        if (begin->hasInnerNondet)
        {
            PUSH(contStack, RestoreLoopCont, begin->loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        loopInfo->number++;

        if (loopInfo->number < begin->repeats.lower)
        {
            // Must match another iteration of body.
            loopInfo->startInputOffset = inputOffset;
            if(begin->hasInnerNondet)
            {
                // If it backtracks into the loop body of an earlier iteration, it must restore inner groups for that iteration.
                // Save the inner groups and reset them for the next iteration.
                matcher.SaveInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId, true, input, contStack);
            }
            else
            {
                // If it backtracks, the entire loop will fail, so no need to restore groups. Just reset the inner groups for
                // the next iteration.
                matcher.ResetInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId);
            }
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopInst));
        }
        else if (inputOffset == loopInfo->startInputOffset && loopInfo->number > begin->repeats.lower)
        {
            // The minimum number of iterations has been satisfied but the last iteration made no progress.
            //   - With greedy & deterministic body, FAIL so as to undo that iteration and restore group bindings.
            //   - With greedy & non-deterministic body, FAIL so as to try another body alternative
            //   - With non-greedy, we're trying an additional iteration because the follow failed. But
            //     since we didn't consume anything the follow will fail again, so fail
            //
            return matcher.Fail(FAIL_PARAMETERS);
        }
        else if (begin->repeats.upper != CharCountFlag && loopInfo->number >= (CharCount)begin->repeats.upper)
        {
            // Success: proceed to remainder.
            instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        }
        else if (begin->isGreedy)
        {
            // CHOICEPOINT: Try one more iteration of body, if backtrack continue from here with no more iterations
            PUSH(contStack, ResumeCont, inputOffset, begin->exitLabel);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
            loopInfo->startInputOffset = inputOffset;

            // If backtrack, we must continue with previous group bindings
            matcher.SaveInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId, true, input, contStack);
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopInst));
        }
        else
        {
            // CHOICEPOINT: Try no more iterations of body, if backtrack do one more iteration of body from here
            PUSH(contStack, RepeatLoopCont, beginLabel, inputOffset);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
            instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatLoopInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("RepeatLoop");
        PRINT_MIXIN(RepeatLoopMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(RepeatLoopMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginLoopIfCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool BeginLoopIfCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == c)
        {
            // Commit to at least one iteration of loop
            LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

            // All inner groups must begin reset
#if DBG
            for (int i = minBodyGroupId; i <= maxBodyGroupId; i++)
            {
                Assert(matcher.GroupIdToGroupInfo(i)->IsUndefined());
            }
#endif
            loopInfo->number = 0;
            instPointer += sizeof(*this);
            return false;
        }

        if (repeats.lower > 0)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer = matcher.LabelToInstPointer(exitLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginLoopIfCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginLoopIfChar");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN_COMMA(BeginLoopMixin);
        PRINT_MIXIN(BodyGroupsMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(BeginLoopMixin);
        PRINT_BYTES(BodyGroupsMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginLoopIfSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool BeginLoopIfSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && set.Get(input[inputOffset]))
        {
            // Commit to at least one iteration of loop
            LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

            // All inner groups must be begin reset
#if DBG
            for (int i = minBodyGroupId; i <= maxBodyGroupId; i++)
            {
                Assert(matcher.GroupIdToGroupInfo(i)->IsUndefined());
            }
#endif

            loopInfo->startInputOffset = inputOffset;
            loopInfo->number = 0;
            instPointer += sizeof(*this);
            return false;
        }

        if (repeats.lower > 0)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer = matcher.LabelToInstPointer(exitLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginLoopIfSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginLoopIfSet");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN_COMMA(BeginLoopMixin);
        PRINT_MIXIN(BodyGroupsMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(BeginLoopMixin);
        PRINT_BYTES(BodyGroupsMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatLoopIfCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool RepeatLoopIfCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        BeginLoopIfCharInst* begin = matcher.L2I(BeginLoopIfChar, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        if (begin->hasInnerNondet)
        {
            // May end up backtracking into loop body for iteration just completed: see above.
            PUSH(contStack, RestoreLoopCont, begin->loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        loopInfo->number++;

#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == begin->c)
        {
            if (begin->repeats.upper != CharCountFlag && loopInfo->number >= (CharCount)begin->repeats.upper)
            {
                // If the loop body's first set and the loop's follow set are disjoint, we can just fail here since
                // we know the next character in the input is in the loop body's first set.
                return matcher.Fail(FAIL_PARAMETERS);
            }

            // Commit to one more iteration
            if(begin->hasInnerNondet)
            {
                // If it backtracks into the loop body of an earlier iteration, it must restore inner groups for that iteration.
                // Save the inner groups and reset them for the next iteration.
                matcher.SaveInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId, true, input, contStack);
            }
            else
            {
                // If it backtracks, the entire loop will fail, so no need to restore groups. Just reset the inner groups for
                // the next iteration.
                matcher.ResetInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId);
            }
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopIfCharInst));
            return false;
        }

        if (loopInfo->number < begin->repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        // Proceed to exit
        instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatLoopIfCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("RepeatLoopIfChar");
        PRINT_MIXIN(RepeatLoopMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(RepeatLoopMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatLoopIfSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool RepeatLoopIfSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        BeginLoopIfSetInst* begin = matcher.L2I(BeginLoopIfSet, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        if (begin->hasInnerNondet)
        {
            // May end up backtracking into loop body for iteration just completed: see above.
            PUSH(contStack, RestoreLoopCont, begin->loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        loopInfo->number++;

#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && begin->set.Get(input[inputOffset]))
        {
            if (begin->repeats.upper != CharCountFlag && loopInfo->number >= (CharCount)begin->repeats.upper)
            {
                // If the loop body's first set and the loop's follow set are disjoint, we can just fail here since
                // we know the next character in the input is in the loop body's first set.
                return matcher.Fail(FAIL_PARAMETERS);
            }

            // Commit to one more iteration
            if (begin->hasInnerNondet)
            {
                // If it backtracks into the loop body of an earlier iteration, it must restore inner groups for that iteration.
                // Save the inner groups and reset them for the next iteration.
                matcher.SaveInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId, true, input, contStack);
            }
            else
            {
                // If it backtracks, the entire loop will fail, so no need to restore groups. Just reset the inner groups for
                // the next iteration.
                matcher.ResetInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId);
            }
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopIfSetInst));
            return false;
        }

        if (loopInfo->number < begin->repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        // Proceed to exit
        instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatLoopIfSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("RepeatLoopIfSet");
        PRINT_MIXIN(RepeatLoopMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(RepeatLoopMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginLoopFixedInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool BeginLoopFixedInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

        // If loop is contained in an outer loop, continuation stack may already have a RewindLoopFixed entry for
        // this loop. We must make sure it's state is preserved on backtrack.
        if (hasOuterLoops)
        {
            PUSH(contStack, RestoreLoopCont, loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        // startInputOffset will stay here for all iterations, and we'll use number of length to figure out
        // where in the input to rewind to
        loopInfo->number = 0;
        loopInfo->startInputOffset = inputOffset;

        if (repeats.lower == 0)
        {
            // CHOICEPOINT: Try one iteration of body. Failure of body will rewind input to here and resume with follow.
            Assert(instPointer == (uint8*)this);
            PUSH(contStack, RewindLoopFixedCont, matcher.InstPointerToLabel(instPointer), true);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }
        // else: Must match minimum iterations, so continue to loop body. Failure of body signals failure of entire loop.

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginLoopFixedInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginLoopFixed");
        PRINT_MIXIN_COMMA(BeginLoopMixin);
        PRINT_MIXIN(FixedLengthMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(BeginLoopMixin);
        PRINT_BYTES(FixedLengthMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatLoopFixedInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool RepeatLoopFixedInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        BeginLoopFixedInst* begin = matcher.L2I(BeginLoopFixed, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        loopInfo->number++;

        if (loopInfo->number < begin->repeats.lower)
        {
            // Must match another iteration of body. Failure of body signals failure of the entire loop.
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopFixedInst));
        }
        else if (begin->repeats.upper != CharCountFlag && loopInfo->number >= (CharCount)begin->repeats.upper)
        {
            // Matched maximum number of iterations. Continue with follow.
            if (begin->repeats.lower < begin->repeats.upper)
            {
                // Failure of follow will try one fewer iterations (subject to repeats.lower).
                // Since loop body is non-deterministic and does not define groups the rewind continuation must be on top of the stack.
                Cont *top = contStack.Top();
                Assert(top != 0);
                Assert(top->tag == Cont::ContTag::RewindLoopFixed);
                RewindLoopFixedCont* rewind = (RewindLoopFixedCont*)top;
                rewind->tryingBody = false;
            }
            // else: we never pushed a rewind continuation
            instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        }
        else
        {
            // CHOICEPOINT: Try one more iteration of body. Failure of body will rewind input to here and
            // try follow.
            if (loopInfo->number == begin->repeats.lower)
            {
                // i.e. begin->repeats.lower > 0, so continuation won't have been pushed in BeginLoopFixed
                PUSH(contStack, RewindLoopFixedCont, beginLabel, true);
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.PushStats(contStack, input);
#endif
            }
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopFixedInst));
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatLoopFixedInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("RepeatLoopFixed");
        PRINT_MIXIN(RepeatLoopMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(RepeatLoopMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // LoopSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool LoopSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

        // If loop is contained in an outer loop, continuation stack may already have a RewindLoopFixed entry for
        // this loop. We must make sure it's state is preserved on backtrack.
        if (hasOuterLoops)
        {
            PUSH(contStack, RestoreLoopCont, loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        // startInputOffset will stay here for all iterations, and we'll use number of length to figure out
        // where in the input to rewind to
        loopInfo->startInputOffset = inputOffset;

        // Consume as many elements of set as possible
        const RuntimeCharSet<Char>& matchSet = this->set;
        const CharCount loopMatchStart = inputOffset;
        const CharCountOrFlag repeatsUpper = repeats.upper;
        const CharCount inputEndOffset =
            static_cast<CharCount>(repeatsUpper) >= inputLength - inputOffset
                ? inputLength
                : inputOffset + static_cast<CharCount>(repeatsUpper);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputEndOffset && matchSet.Get(input[inputOffset]))
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        loopInfo->number = inputOffset - loopMatchStart;
        if (loopInfo->number < repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }
        else if (loopInfo->number > repeats.lower)
        {
            // CHOICEPOINT: If follow fails, try consuming one fewer characters
            Assert(instPointer == (uint8*)this);
            PUSH(contStack, RewindLoopSetCont, matcher.InstPointerToLabel(instPointer));
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }
        // else: failure of follow signals failure of entire loop

        // Continue with follow
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int LoopSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("LoopSetInst");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN(BeginLoopBasicsMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(BeginLoopBasicsMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    inline bool LoopSetWithFollowFirstInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

        // If loop is contained in an outer loop, continuation stack may already have a RewindLoopFixed entry for
        // this loop. We must make sure it's state is preserved on backtrack.
        if (hasOuterLoops)
        {
            PUSH(contStack, RestoreLoopCont, loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        if (loopInfo->offsetsOfFollowFirst)
        {
            loopInfo->offsetsOfFollowFirst->Clear();
        }
        // startInputOffset will stay here for all iterations, and we'll use number of length to figure out
        // where in the input to rewind to
        loopInfo->startInputOffset = inputOffset;

        // Consume as many elements of set as possible
        const RuntimeCharSet<Char>& matchSet = this->set;
        const CharCount loopMatchStart = inputOffset;
        const CharCountOrFlag repeatsUpper = repeats.upper;
        const CharCount inputEndOffset =
            static_cast<CharCount>(repeatsUpper) >= inputLength - inputOffset
            ? inputLength
            : inputOffset + static_cast<CharCount>(repeatsUpper);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputEndOffset && matchSet.Get(input[inputOffset]))
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            if (input[inputOffset] == this->followFirst)
            {
                loopInfo->EnsureOffsetsOfFollowFirst(matcher);
                loopInfo->offsetsOfFollowFirst->Add(inputOffset - loopInfo->startInputOffset);
            }
            inputOffset++;
        }

        loopInfo->number = inputOffset - loopMatchStart;
        if (loopInfo->number < repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }
        else if (loopInfo->number > repeats.lower)
        {
            // CHOICEPOINT: If follow fails, try consuming one fewer characters
            Assert(instPointer == (uint8*)this);
            PUSH(contStack, RewindLoopSetWithFollowFirstCont, matcher.InstPointerToLabel(instPointer));
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }
        // else: failure of follow signals failure of entire loop

        // Continue with follow
        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int LoopSetWithFollowFirstInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("LoopSetWithFollowFirstInst");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN_COMMA(BeginLoopBasicsMixin);
        PRINT_MIXIN(FollowFirstMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(BeginLoopBasicsMixin);
        PRINT_MIXIN(FollowFirstMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginLoopFixedGroupLastIterationInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool BeginLoopFixedGroupLastIterationInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        Assert(matcher.GroupIdToGroupInfo(groupId)->IsUndefined());

        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

        // If loop is contained in an outer loop, continuation stack may already have a RewindLoopFixedGroupLastIteration entry
        // for this loop. We must make sure it's state is preserved on backtrack.
        if (hasOuterLoops)
        {
            PUSH(contStack, RestoreLoopCont, loopId, *loopInfo, matcher);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        // If loop is contained in an outer loop or assertion, we must reset the group binding if we backtrack all the way out
        if (!noNeedToSave)
        {
            PUSH(contStack, ResetGroupCont, groupId);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }

        // startInputOffset will stay here for all iterations, and we'll use number of length to figure out
        // where in the input to rewind to
        loopInfo->number = 0;
        loopInfo->startInputOffset = inputOffset;

        if (repeats.lower == 0)
        {
            // CHOICEPOINT: Try one iteration of body. Failure of body will rewind input to here and resume with follow.
            Assert(instPointer == (uint8*)this);
            PUSH(contStack, RewindLoopFixedGroupLastIterationCont, matcher.InstPointerToLabel(instPointer), true);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
        }
        // else: Must match minimum iterations, so continue to loop body. Failure of body signals failure of entire loop.

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginLoopFixedGroupLastIterationInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginLoopFixedGroupLastIteration");
        PRINT_MIXIN_COMMA(BeginLoopMixin);
        PRINT_MIXIN_COMMA(FixedLengthMixin);
        PRINT_MIXIN_COMMA(GroupMixin);
        PRINT_MIXIN(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(BeginLoopMixin);
        PRINT_BYTES(FixedLengthMixin);
        PRINT_BYTES(GroupMixin);
        PRINT_BYTES(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatLoopFixedGroupLastIterationInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool RepeatLoopFixedGroupLastIterationInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        BeginLoopFixedGroupLastIterationInst* begin = matcher.L2I(BeginLoopFixedGroupLastIteration, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        loopInfo->number++;

        if (loopInfo->number < begin->repeats.lower)
        {
            // Must match another iteration of body. Failure of body signals failure of the entire loop.
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopFixedGroupLastIterationInst));
        }
        else if (begin->repeats.upper != CharCountFlag && loopInfo->number >= (CharCount)begin->repeats.upper)
        {
            // Matched maximum number of iterations. Continue with follow.
            if (begin->repeats.lower < begin->repeats.upper)
            {
                // Failure of follow will try one fewer iterations (subject to repeats.lower).
                // Since loop body is non-deterministic and does not define groups the rewind continuation must be on top of the stack.
                Cont *top = contStack.Top();
                Assert(top != 0);
                Assert(top->tag == Cont::ContTag::RewindLoopFixedGroupLastIteration);
                RewindLoopFixedGroupLastIterationCont* rewind = (RewindLoopFixedGroupLastIterationCont*)top;
                rewind->tryingBody = false;
            }
            // else: we never pushed a rewind continuation

            // Bind group
            GroupInfo* groupInfo = matcher.GroupIdToGroupInfo(begin->groupId);
            groupInfo->offset = inputOffset - begin->length;
            groupInfo->length = begin->length;

            instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        }
        else
        {
            // CHOICEPOINT: Try one more iteration of body. Failure of body will rewind input to here and
            // try follow.
            if (loopInfo->number == begin->repeats.lower)
            {
                // i.e. begin->repeats.lower > 0, so continuation won't have been pushed in BeginLoopFixed
                PUSH(contStack, RewindLoopFixedGroupLastIterationCont, beginLabel, true);
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.PushStats(contStack, input);
#endif
            }
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopFixedGroupLastIterationInst));
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatLoopFixedGroupLastIterationInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("RepeatLoopFixedGroupLastIteration");
        PRINT_MIXIN(RepeatLoopMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(RepeatLoopMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginGreedyLoopNoBacktrackInst
    // ----------------------------------------------------------------------

    inline bool BeginGreedyLoopNoBacktrackInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(loopId);

        loopInfo->number = 0;
        loopInfo->startInputOffset = inputOffset;

        // CHOICEPOINT: Try one iteration of body, if backtrack continue from here with no iterations
        PUSH(contStack, ResumeCont, inputOffset, exitLabel);
        instPointer += sizeof(*this);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.PushStats(contStack, input);
#endif

        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginGreedyLoopNoBacktrackInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginGreedyLoopNoBacktrack");
        PRINT_MIXIN(GreedyLoopNoBacktrackMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(GreedyLoopNoBacktrackMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatGreedyLoopNoBacktrackInst
    // ----------------------------------------------------------------------

    inline bool RepeatGreedyLoopNoBacktrackInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        BeginGreedyLoopNoBacktrackInst* begin = matcher.L2I(BeginGreedyLoopNoBacktrack, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        loopInfo->number++;

        if (inputOffset == loopInfo->startInputOffset)
        {
            // No progress
            return matcher.Fail(FAIL_PARAMETERS);
        }
        else
        {
            // CHOICEPOINT: Try one more iteration of body, if backtrack, continue from here with no more iterations.
            // Since the loop body is deterministic and group free, it wouldn't have left any continuation records.
            // Therefore we can simply update the Resume continuation still on the top of the stack with the current
            // input pointer.
            Cont* top = contStack.Top();
            Assert(top != 0 && top->tag == Cont::ContTag::Resume);
            ResumeCont* resume = (ResumeCont*)top;
            resume->origInputOffset = inputOffset;

            loopInfo->startInputOffset = inputOffset;
            instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginGreedyLoopNoBacktrackInst));
        }
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatGreedyLoopNoBacktrackInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("RepeatGreedyLoopNoBacktrack");
        PRINT_MIXIN(RepeatLoopMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(RepeatLoopMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<ChompMode Mode>
    inline bool ChompCharInst<Mode>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const Char matchC = c;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (Mode == ChompMode::Star || (inputOffset < inputLength && input[inputOffset] == matchC))
        {
            while (true)
            {
                if (Mode != ChompMode::Star)
                {
                    ++inputOffset;
                }
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.CompStats();
#endif
                if (inputOffset < inputLength && input[inputOffset] == matchC)
                {
                    if (Mode == ChompMode::Star)
                    {
                        ++inputOffset;
                    }
                    continue;
                }
                break;
            }

            instPointer += sizeof(*this);
            return false;
        }

        return matcher.Fail(FAIL_PARAMETERS);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<ChompMode Mode>
    int ChompCharInst<Mode>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (Mode == ChompMode::Star)
        {
            PRINT_RE_BYTECODE_BEGIN("ChompChar<Star>");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("ChompChar<Plus>");
        }

        PRINT_MIXIN(CharMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<ChompMode Mode>
    inline bool ChompSetInst<Mode>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const RuntimeCharSet<Char>& matchSet = this->set;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if(Mode == ChompMode::Star || (inputOffset < inputLength && matchSet.Get(input[inputOffset])))
        {
            while(true)
            {
                if (Mode != ChompMode::Star)
                {
                    ++inputOffset;
                }
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.CompStats();
#endif
                if (inputOffset < inputLength && matchSet.Get(input[inputOffset]))
                {
                    if (Mode == ChompMode::Star)
                    {
                        ++inputOffset;
                    }
                    continue;
                }
                break;
            }

            instPointer += sizeof(*this);
            return false;
        }

        return matcher.Fail(FAIL_PARAMETERS);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<ChompMode Mode>
    int ChompSetInst<Mode>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (Mode == ChompMode::Star)
        {
            PRINT_RE_BYTECODE_BEGIN("ChompSet<Star>");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("ChompSet<Plus>");
        }

        PRINT_MIXIN(SetMixin<false>);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompCharGroupInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<ChompMode Mode>
    inline bool ChompCharGroupInst<Mode>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        Assert(matcher.GroupIdToGroupInfo(groupId)->IsUndefined());

        const CharCount inputStartOffset = inputOffset;
        const Char matchC = c;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if(Mode == ChompMode::Star || (inputOffset < inputLength && input[inputOffset] == matchC))
        {
            while (true)
            {
                if (Mode != ChompMode::Star)
                {
                    ++inputOffset;
                }
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.CompStats();
#endif
                if (inputOffset < inputLength && input[inputOffset] == matchC)
                {
                    if (Mode == ChompMode::Star)
                    {
                        ++inputOffset;
                    }
                    continue;
                }
                break;
            }

            if (!noNeedToSave)
            {
                // UNDO ACTION: Restore group on backtrack
                PUSH(contStack, ResetGroupCont, groupId);
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.PushStats(contStack, input);
#endif
            }

            GroupInfo *const groupInfo = matcher.GroupIdToGroupInfo(groupId);
            groupInfo->offset = inputStartOffset;
            groupInfo->length = inputOffset - inputStartOffset;

            instPointer += sizeof(*this);
            return false;
        }

        return matcher.Fail(FAIL_PARAMETERS);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<ChompMode Mode>
    int ChompCharGroupInst<Mode>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (Mode == ChompMode::Star)
        {
            PRINT_RE_BYTECODE_BEGIN("ChompCharGroup<Star>");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("ChompCharGroup<Plus>");
        }

        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN_COMMA(GroupMixin);
        PRINT_MIXIN(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(GroupMixin);
        PRINT_BYTES(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompSetGroupInst (optimized instruction)
    // ----------------------------------------------------------------------

    template<ChompMode Mode>
    inline bool ChompSetGroupInst<Mode>::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        Assert(matcher.GroupIdToGroupInfo(groupId)->IsUndefined());

        const CharCount inputStartOffset = inputOffset;
        const RuntimeCharSet<Char>& matchSet = this->set;
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (Mode == ChompMode::Star || (inputOffset < inputLength && matchSet.Get(input[inputOffset])))
        {
            while (true)
            {
                if (Mode != ChompMode::Star)
                {
                    ++inputOffset;
                }
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.CompStats();
#endif
                if (inputOffset < inputLength && matchSet.Get(input[inputOffset]))
                {
                    if (Mode == ChompMode::Star)
                    {
                        ++inputOffset;
                    }
                    continue;
                }
                break;
            }

            if (!noNeedToSave)
            {
                // UNDO ACTION: Restore group on backtrack
                PUSH(contStack, ResetGroupCont, groupId);
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.PushStats(contStack, input);
#endif
            }

            GroupInfo *const groupInfo = matcher.GroupIdToGroupInfo(groupId);
            groupInfo->offset = inputStartOffset;
            groupInfo->length = inputOffset - inputStartOffset;

            instPointer += sizeof(*this);
            return false;
        }

        return matcher.Fail(FAIL_PARAMETERS);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    template<ChompMode Mode>
    int ChompSetGroupInst<Mode>::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        if (Mode == ChompMode::Star)
        {
            PRINT_RE_BYTECODE_BEGIN("ChompSetGroup<Star>");
        }
        else
        {
            PRINT_RE_BYTECODE_BEGIN("ChompSetGroup<Plus>");
        }

        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN_COMMA(GroupMixin);
        PRINT_MIXIN(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(GroupMixin);
        PRINT_BYTES(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompCharBoundedInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool ChompCharBoundedInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const Char matchC = c;
        const CharCount loopMatchStart = inputOffset;
        const CharCountOrFlag repeatsUpper = repeats.upper;
        const CharCount inputEndOffset =
            static_cast<CharCount>(repeatsUpper) >= inputLength - inputOffset
                ? inputLength
                : inputOffset + static_cast<CharCount>(repeatsUpper);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputEndOffset && input[inputOffset] == matchC)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset - loopMatchStart < repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int ChompCharBoundedInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("ChompCharBounded");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN(ChompBoundedMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(ChompBoundedMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompSetBoundedInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool ChompSetBoundedInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        const RuntimeCharSet<Char>& matchSet = this->set;
        const CharCount loopMatchStart = inputOffset;
        const CharCountOrFlag repeatsUpper = repeats.upper;
        const CharCount inputEndOffset =
            static_cast<CharCount>(repeatsUpper) >= inputLength - inputOffset
                ? inputLength
                : inputOffset + static_cast<CharCount>(repeatsUpper);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputEndOffset && matchSet.Get(input[inputOffset]))
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset - loopMatchStart < repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int ChompSetBoundedInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("ChompSetBounded");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN(ChompBoundedMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(ChompBoundedMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // ChompSetBoundedGroupLastCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool ChompSetBoundedGroupLastCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        Assert(matcher.GroupIdToGroupInfo(groupId)->IsUndefined());

        const RuntimeCharSet<Char>& matchSet = this->set;
        const CharCount loopMatchStart = inputOffset;
        const CharCountOrFlag repeatsUpper = repeats.upper;
        const CharCount inputEndOffset =
            static_cast<CharCount>(repeatsUpper) >= inputLength - inputOffset
                ? inputLength
                : inputOffset + static_cast<CharCount>(repeatsUpper);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        while (inputOffset < inputEndOffset && matchSet.Get(input[inputOffset]))
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.CompStats();
#endif
            inputOffset++;
        }

        if (inputOffset - loopMatchStart < repeats.lower)
        {
            return matcher.Fail(FAIL_PARAMETERS);
        }

        if (inputOffset > loopMatchStart)
        {
            if (!noNeedToSave)
            {
                PUSH(contStack, ResetGroupCont, groupId);
#if ENABLE_REGEX_CONFIG_OPTIONS
                matcher.PushStats(contStack, input);
#endif
            }

            GroupInfo *const groupInfo = matcher.GroupIdToGroupInfo(groupId);
            groupInfo->offset  = inputOffset - 1;
            groupInfo->length = 1;
        }

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int ChompSetBoundedGroupLastCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("ChompSetBoundedGroupLastChar");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN_COMMA(ChompBoundedMixin);
        PRINT_MIXIN_COMMA(GroupMixin);
        PRINT_MIXIN(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(ChompBoundedMixin);
        PRINT_BYTES(GroupMixin);
        PRINT_BYTES(NoNeedToSaveMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // TryInst
    // ----------------------------------------------------------------------

    inline bool TryInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        // CHOICEPOINT: Resume at fail label on backtrack
        PUSH(contStack, ResumeCont, inputOffset, failLabel);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.PushStats(contStack, input);
#endif

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int TryInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("Try");
        PRINT_MIXIN(TryMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(TryMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // TryIfCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool TryIfCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == c)
        {
            // CHOICEPOINT: Resume at fail label on backtrack
            PUSH(contStack, ResumeCont, inputOffset, failLabel);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
            instPointer += sizeof(*this);
            return false;
        }

        // Proceed directly to exit
        instPointer = matcher.LabelToInstPointer(failLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int TryIfCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("TryIfChar");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN(TryMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(TryMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // TryMatchCharInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool TryMatchCharInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && input[inputOffset] == c)
        {
            // CHOICEPOINT: Resume at fail label on backtrack
            PUSH(contStack, ResumeCont, inputOffset, failLabel);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
            inputOffset++;
            instPointer += sizeof(*this);
            return false;
        }

        // Proceed directly to exit
        instPointer = matcher.LabelToInstPointer(failLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int TryMatchCharInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("TryMatchChar");
        PRINT_MIXIN_COMMA(CharMixin);
        PRINT_MIXIN(TryMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(CharMixin);
        PRINT_BYTES(TryMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // TryIfSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool TryIfSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && set.Get(input[inputOffset]))
        {
            // CHOICEPOINT: Resume at fail label on backtrack
            PUSH(contStack, ResumeCont, inputOffset, failLabel);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
            instPointer += sizeof(*this);
            return false;
        }

        // Proceed directly to exit
        instPointer = matcher.LabelToInstPointer(failLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int TryIfSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("TryIfSet");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN(TryMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(TryMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // TryMatchSetInst (optimized instruction)
    // ----------------------------------------------------------------------

    inline bool TryMatchSetInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.CompStats();
#endif
        if (inputOffset < inputLength && set.Get(input[inputOffset]))
        {
            // CHOICEPOINT: Resume at fail label on backtrack
            PUSH(contStack, ResumeCont, inputOffset, failLabel);
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.PushStats(contStack, input);
#endif
            inputOffset++;
            instPointer += sizeof(*this);
            return false;
        }

        // Proceed directly to exit
        instPointer = matcher.LabelToInstPointer(failLabel);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int TryMatchSetInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("TryMatchSet");
        PRINT_MIXIN_COMMA(SetMixin<false>);
        PRINT_MIXIN(TryMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(SetMixin<false>);
        PRINT_BYTES(TryMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // BeginAssertionInst
    // ----------------------------------------------------------------------

    inline bool BeginAssertionInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        Assert(instPointer == (uint8*)this);

        if (!isNegation)
        {
            // If the positive assertion binds some groups then on success any RestoreGroup continuations pushed
            // in the assertion body will be cut. Hence if the entire assertion is backtracked over we must restore
            // the current inner group bindings.
            matcher.SaveInnerGroups(minBodyGroupId, maxBodyGroupId, false, input, contStack);
        }

        PUSHA(assertionStack, AssertionInfo, matcher.InstPointerToLabel(instPointer), inputOffset, contStack.Position());
        PUSH(contStack, PopAssertionCont);
#if ENABLE_REGEX_CONFIG_OPTIONS
        matcher.PushStats(contStack, input);
#endif

        instPointer += sizeof(*this);
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int BeginAssertionInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("BeginAssertion");
        PRINT_MIXIN_COMMA(BodyGroupsMixin);
        PRINT_MIXIN_COMMA(NegationMixin);
        PRINT_MIXIN(NextLabelMixin);
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(BodyGroupsMixin);
        PRINT_BYTES(NegationMixin);
        PRINT_BYTES(NextLabelMixin);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // EndAssertionInst
    // ----------------------------------------------------------------------

    inline bool EndAssertionInst::Exec(REGEX_INST_EXEC_PARAMETERS) const
    {
        if (!matcher.PopAssertion(inputOffset, instPointer, contStack, assertionStack, true))
        {
            // Body of negative assertion succeeded, so backtrack
            return matcher.Fail(FAIL_PARAMETERS);
        }

        // else: body of positive assertion succeeded, instruction pointer already at next instruction
        return false;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int EndAssertionInst::Print(DebugWriter* w, Label label, const Char* litbuf) const
    {
        PRINT_RE_BYTECODE_BEGIN("EndAssertion");
        PRINT_RE_BYTECODE_MID();
        PRINT_BYTES(EndAssertionInst);
        PRINT_RE_BYTECODE_END();
    }
#endif

    // ----------------------------------------------------------------------
    // Matcher state
    // ----------------------------------------------------------------------

#if ENABLE_REGEX_CONFIG_OPTIONS
    void LoopInfo::Print(DebugWriter* w) const
    {
        w->Print(_u("number: %u, startInputOffset: %u"), number, startInputOffset);
    }
#endif

    void LoopInfo::EnsureOffsetsOfFollowFirst(Matcher& matcher)
    {
        if (this->offsetsOfFollowFirst == nullptr)
        {
            this->offsetsOfFollowFirst = JsUtil::List<CharCount, ArenaAllocator>::New(matcher.pattern->library->GetScriptContext()->RegexAllocator());
        }
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    void GroupInfo::Print(DebugWriter* w, const Char* const input) const
    {
        if (IsUndefined())
        {
            w->Print(_u("<undefined> (%u)"), offset);
        }
        else
        {
            w->PrintQuotedString(input + offset, (CharCount)length);
            w->Print(_u(" (%u+%u)"), offset, (CharCount)length);
        }
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void AssertionInfo::Print(DebugWriter* w) const
    {
        w->PrintEOL(_u("beginLabel: L%04x, startInputOffset: %u, contStackPosition: $llu"), beginLabel, startInputOffset, static_cast<unsigned long long>(contStackPosition));
    }
#endif

    // ----------------------------------------------------------------------
    // ResumeCont
    // ----------------------------------------------------------------------

    inline bool ResumeCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        inputOffset = origInputOffset;
        instPointer = matcher.LabelToInstPointer(origInstLabel);
        return true; // STOP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int ResumeCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("Resume(origInputOffset: %u, origInstLabel: L%04x)"), origInputOffset, origInstLabel);
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RestoreLoopCont
    // ----------------------------------------------------------------------

    inline RestoreLoopCont::RestoreLoopCont(int loopId, LoopInfo& origLoopInfo, Matcher& matcher) : Cont(ContTag::RestoreLoop), loopId(loopId)
    {
        this->origLoopInfo.number = origLoopInfo.number;
        this->origLoopInfo.startInputOffset = origLoopInfo.startInputOffset;
        this->origLoopInfo.offsetsOfFollowFirst = nullptr;
        if (origLoopInfo.offsetsOfFollowFirst != nullptr)
        {
            this->origLoopInfo.offsetsOfFollowFirst = JsUtil::List<CharCount, ArenaAllocator>::New(matcher.pattern->library->GetScriptContext()->RegexAllocator());
            this->origLoopInfo.offsetsOfFollowFirst->Copy(origLoopInfo.offsetsOfFollowFirst);
        }
    }

    inline bool RestoreLoopCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.QueryContinue(qcTicks);

        *matcher.LoopIdToLoopInfo(loopId) = origLoopInfo;
        return false; // KEEP BACKTRACKING
    }


#if ENABLE_REGEX_CONFIG_OPTIONS
    int RestoreLoopCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->Print(_u("RestoreLoop(loopId: %d, "), loopId);
        origLoopInfo.Print(w);
        w->PrintEOL(_u(")"));
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RestoreGroupCont
    // ----------------------------------------------------------------------

    inline bool RestoreGroupCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        *matcher.GroupIdToGroupInfo(groupId) = origGroupInfo;
        return false; // KEEP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RestoreGroupCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->Print(_u("RestoreGroup(groupId: %d, "), groupId);
        origGroupInfo.Print(w, input);
        w->PrintEOL(_u(")"));
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // ResetGroupCont
    // ----------------------------------------------------------------------

    inline bool ResetGroupCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.ResetGroup(groupId);
        return false; // KEEP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int ResetGroupCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("ResetGroup(groupId: %d)"), groupId);
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // ResetGroupRangeCont
    // ----------------------------------------------------------------------

    inline bool ResetGroupRangeCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.ResetInnerGroups(fromGroupId, toGroupId);
        return false; // KEEP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int ResetGroupRangeCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("ResetGroupRange(fromGroupId: %d, toGroupId: %d)"), fromGroupId, toGroupId);
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RepeatLoopCont
    // ----------------------------------------------------------------------

    inline bool RepeatLoopCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.QueryContinue(qcTicks);

        // Try one more iteration of a non-greedy loop
        BeginLoopInst* begin = matcher.L2I(BeginLoop, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);
        loopInfo->startInputOffset = inputOffset = origInputOffset;
        instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(BeginLoopInst));
        if(begin->hasInnerNondet)
        {
            // If it backtracks into the loop body of an earlier iteration, it must restore inner groups for that iteration.
            // Save the inner groups and reset them for the next iteration.
            matcher.SaveInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId, true, input, contStack);
        }
        else
        {
            // If it backtracks, the entire loop will fail, so no need to restore groups. Just reset the inner groups for
            // the next iteration.
            matcher.ResetInnerGroups(begin->minBodyGroupId, begin->maxBodyGroupId);
        }
        return true; // STOP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RepeatLoopCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("RepeatLoop(beginLabel: L%04x, origInputOffset: %u)"), beginLabel, origInputOffset);
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // PopAssertionCont
    // ----------------------------------------------------------------------

    inline bool PopAssertionCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        Assert(!assertionStack.IsEmpty());
        if (matcher.PopAssertion(inputOffset, instPointer, contStack, assertionStack, false))
        {
            // Body of negative assertion failed
            return true; // STOP BACKTRACKING
        }
        else
        {
            // Body of positive assertion failed
            return false; // CONTINUE BACKTRACKING
        }
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int PopAssertionCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("PopAssertion()"));
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RewindLoopFixedCont
    // ----------------------------------------------------------------------

    inline bool RewindLoopFixedCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.QueryContinue(qcTicks);

        BeginLoopFixedInst* begin = matcher.L2I(BeginLoopFixed, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        if (tryingBody)
        {
            tryingBody = false;
            // loopInfo->number is the number of iterations completed before trying body
            Assert(loopInfo->number >= begin->repeats.lower);
        }
        else
        {
            // loopInfo->number is the number of iterations completed before trying follow
            Assert(loopInfo->number > begin->repeats.lower);
            // Try follow with one fewer iteration
            loopInfo->number--;
        }

        // Rewind input
        inputOffset = loopInfo->startInputOffset + loopInfo->number * begin->length;

        if (loopInfo->number > begin->repeats.lower)
        {
            // Un-pop the continuation ready for next time
            contStack.UnPop<RewindLoopFixedCont>();
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.UnPopStats(contStack, input);
#endif
        }
        // else: Can't try any fewer iterations if follow fails, so leave continuation as popped and let failure propagate

        instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        return true; // STOP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RewindLoopFixedCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("RewindLoopFixed(beginLabel: L%04x, tryingBody: %s)"), beginLabel, tryingBody ? _u("true") : _u("false"));
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RewindLoopSetCont
    // ----------------------------------------------------------------------

    inline bool RewindLoopSetCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.QueryContinue(qcTicks);

        LoopSetInst* begin = matcher.L2I(LoopSet, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        // loopInfo->number is the number of iterations completed before trying follow
        Assert(loopInfo->number > begin->repeats.lower);
        // Try follow with fewer iterations
        loopInfo->number--;

        // Rewind input
        inputOffset = loopInfo->startInputOffset + loopInfo->number;

        if (loopInfo->number > begin->repeats.lower)
        {
            // Un-pop the continuation ready for next time
            contStack.UnPop<RewindLoopSetCont>();
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.UnPopStats(contStack, input);
#endif
        }
        // else: Can't try any fewer iterations if follow fails, so leave continuation as popped and let failure propagate

        instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(LoopSetInst));
        return true; // STOP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RewindLoopSetCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("RewindLoopSet(beginLabel: L%04x)"), beginLabel);
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RewindLoopSetWithFollowFirstCont
    // ----------------------------------------------------------------------

    inline bool RewindLoopSetWithFollowFirstCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.QueryContinue(qcTicks);

        LoopSetWithFollowFirstInst* begin = matcher.L2I(LoopSetWithFollowFirst, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);

        // loopInfo->number is the number of iterations completed before trying follow
        Assert(loopInfo->number > begin->repeats.lower);
        // Try follow with fewer iterations

        if (loopInfo->offsetsOfFollowFirst == nullptr)
        {
            if (begin->followFirst != MaxUChar)
            {
                // We determined the first character in the follow set at compile time,
                // but didn't find a single match for it in the last iteration of the loop.
                // So, there is no benefit in backtracking.
                loopInfo->number = begin->repeats.lower; // stop backtracking
            }
            else
            {
                // We couldn't determine the first character in the follow set at compile time;
                // fall back to backtracking by one character at a time.
                loopInfo->number--;
            }
        }
        else
        {
            if (loopInfo->offsetsOfFollowFirst->Empty())
            {
                // We have already backtracked to the first offset where we matched the LoopSet's followFirst;
                // no point in backtracking more.
                loopInfo->number = begin->repeats.lower; // stop backtracking
            }
            else
            {
                // Backtrack to the previous offset where we matched the LoopSet's followFirst
                // We will be doing one unnecessary match. But, if we wanted to avoid it, we'd have
                // to propagate to the next Inst, that the first character is already matched.
                // Seems like an overkill to avoid one match.
                loopInfo->number = loopInfo->offsetsOfFollowFirst->RemoveAtEnd();
            }
        }

        // If loopInfo->number now is less than begins->repeats.lower, the loop
        // shouldn't match anything. In that case, stop backtracking.
        loopInfo->number = max(loopInfo->number, begin->repeats.lower);
        // Rewind input
        inputOffset = loopInfo->startInputOffset + loopInfo->number;

        if (loopInfo->number > begin->repeats.lower)
        {
            // Un-pop the continuation ready for next time
            contStack.UnPop<RewindLoopSetWithFollowFirstCont>();
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.UnPopStats(contStack, input);
#endif
        }
        // else: Can't try any fewer iterations if follow fails, so leave continuation as popped and let failure propagate

        instPointer = matcher.LabelToInstPointer(beginLabel + sizeof(LoopSetWithFollowFirstInst));
        return true; // STOP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RewindLoopSetWithFollowFirstCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("RewindLoopSetWithFollowFirst(beginLabel: L%04x)"), beginLabel);
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // RewindLoopFixedGroupLastIterationCont
    // ----------------------------------------------------------------------

    inline bool RewindLoopFixedGroupLastIterationCont::Exec(REGEX_CONT_EXEC_PARAMETERS)
    {
        matcher.QueryContinue(qcTicks);

        BeginLoopFixedGroupLastIterationInst* begin = matcher.L2I(BeginLoopFixedGroupLastIteration, beginLabel);
        LoopInfo* loopInfo = matcher.LoopIdToLoopInfo(begin->loopId);
        GroupInfo* groupInfo = matcher.GroupIdToGroupInfo(begin->groupId);

        if (tryingBody)
        {
            tryingBody = false;
            // loopInfo->number is the number of iterations completed before current attempt of body
            Assert(loopInfo->number >= begin->repeats.lower);
        }
        else
        {
            // loopInfo->number is the number of iterations completed before trying follow
            Assert(loopInfo->number > begin->repeats.lower);
            // Try follow with one fewer iteration
            loopInfo->number--;
        }

        // Rewind input
        inputOffset = loopInfo->startInputOffset + loopInfo->number * begin->length;

        if (loopInfo->number > 0)
        {
            // Bind previous iteration's body
            groupInfo->offset = inputOffset - begin->length;
            groupInfo->length = begin->length;
        }
        else
        {
            groupInfo->Reset();
        }

        if (loopInfo->number > begin->repeats.lower)
        {
            // Un-pop the continuation ready for next time
            contStack.UnPop<RewindLoopFixedGroupLastIterationCont>();
#if ENABLE_REGEX_CONFIG_OPTIONS
            matcher.UnPopStats(contStack, input);
#endif
        }
        // else: Can't try any fewer iterations if follow fails, so leave continuation as popped and let failure propagate

        instPointer = matcher.LabelToInstPointer(begin->exitLabel);
        return true; // STOP BACKTRACKING
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    int RewindLoopFixedGroupLastIterationCont::Print(DebugWriter* w, const Char* const input) const
    {
        w->PrintEOL(_u("RewindLoopFixedGroupLastIteration(beginLabel: L%04x, tryingBody: %s)"), beginLabel, tryingBody ? _u("true") : _u("false"));
        return sizeof(*this);
    }
#endif

    // ----------------------------------------------------------------------
    // Matcher
    // ----------------------------------------------------------------------

#if ENABLE_REGEX_CONFIG_OPTIONS
    void ContStack::Print(DebugWriter* w, const Char* const input) const
    {
        for (Iterator it(*this); it; ++it)
        {
            w->Print(_u("%4llu: "), static_cast<unsigned long long>(it.Position()));
            it->Print(w, input);
        }
    }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
    void AssertionStack::Print(DebugWriter* w, const Matcher* matcher) const
    {
        for (Iterator it(*this); it; ++it)
        {
            it->Print(w);
        }
    }
#endif

    Matcher::Matcher(Js::ScriptContext* scriptContext, RegexPattern* pattern)
        : pattern(pattern)
        , standardChars(scriptContext->GetThreadContext()->GetStandardChars((char16*)0))
        , program(pattern->rep.unified.program)
        , groupInfos(nullptr)
        , loopInfos(nullptr)
        , literalNextSyncInputOffsets(nullptr)
        , recycler(scriptContext->GetRecycler())
        , previousQcTime(0)
#if ENABLE_REGEX_CONFIG_OPTIONS
        , stats(0)
        , w(0)
#endif
    {
        // Don't need to zero out - the constructor for GroupInfo should take care of it
        groupInfos = RecyclerNewArrayLeaf(recycler, GroupInfo, program->numGroups);

        if (program->numLoops > 0)
        {
            loopInfos = RecyclerNewArrayLeafZ(recycler, LoopInfo, program->numLoops);
        }
    }

    Matcher *Matcher::New(Js::ScriptContext* scriptContext, RegexPattern* pattern)
    {
        return RecyclerNew(scriptContext->GetRecycler(), Matcher, scriptContext, pattern);
    }

    Matcher *Matcher::CloneToScriptContext(Js::ScriptContext *scriptContext, RegexPattern *pattern)
    {
        Matcher *result = New(scriptContext, pattern);
        if (groupInfos)
        {
            size_t size = program->numGroups * sizeof(GroupInfo);
            js_memcpy_s(result->groupInfos, size, groupInfos, size);
        }
        if (loopInfos)
        {
            size_t size = program->numLoops * sizeof(LoopInfo);
            js_memcpy_s(result->loopInfos, size, loopInfos, size);
        }

        return result;
    }

#if DBG
    const Cont::ContTag contTags[] = {
#define M(O) Cont::ContTag::O,
#include "RegexContcodes.h"
#undef M
    };

    const Cont::ContTag minContTag = contTags[0];
    const Cont::ContTag maxContTag = contTags[(sizeof(contTags) / sizeof(Cont::ContTag)) - 1];
#endif

    void Matcher::DoQueryContinue(const uint qcTicks)
    {
        // See definition of TimePerQc for description of regex QC heuristics

        const uint before = previousQcTime;
        const uint now = GetTickCount();
        if ((!before || now - before < TimePerQc) && qcTicks & TicksPerQc - 1)
        {
            return;
        }

        previousQcTime = now;
        TraceQueryContinue(now);

        // Query-continue can be reentrant and run the same regex again. To prevent the matcher and other persistent objects
        // from being reused reentrantly, save and restore them around the QC call.
        class AutoCleanup
        {
        private:
            RegexPattern *const pattern;
            Matcher *const matcher;
            RegexStacks * regexStacks;

        public:
            AutoCleanup(RegexPattern *const pattern, Matcher *const matcher) : pattern(pattern), matcher(matcher)
            {
                Assert(pattern);
                Assert(matcher);
                Assert(pattern->rep.unified.matcher == matcher);

                pattern->rep.unified.matcher = nullptr;

                const auto scriptContext = pattern->GetScriptContext();
                regexStacks = scriptContext->SaveRegexStacks();
            }

            ~AutoCleanup()
            {
                pattern->rep.unified.matcher = matcher;

                const auto scriptContext = pattern->GetScriptContext();
                scriptContext->RestoreRegexStacks(regexStacks);
            }
        } autoCleanup(pattern, this);

        pattern->GetScriptContext()->GetThreadContext()->CheckScriptInterrupt();
    }

    void Matcher::TraceQueryContinue(const uint now)
    {
        if (!PHASE_TRACE1(Js::RegexQcPhase))
        {
            return;
        }

        Output::Print(_u("Regex QC"));

        static uint n = 0;
        static uint firstQcTime = 0;

        ++n;
        if (firstQcTime)
        {
            Output::Print(_u(" - frequency: %0.1f"), static_cast<double>(n * 1000) / (now - firstQcTime));
        }
        else
        {
            firstQcTime = now;
        }

        Output::Print(_u("\n"));
        Output::Flush();
    }

    bool Matcher::Fail(const Char* const input, CharCount &inputOffset, const uint8 *&instPointer, ContStack &contStack, AssertionStack &assertionStack, uint &qcTicks)
    {
        if (!contStack.IsEmpty())
        {
            if (!RunContStack(input, inputOffset, instPointer, contStack, assertionStack, qcTicks))
            {
                return false;
            }
        }

        Assert(assertionStack.IsEmpty());
        groupInfos[0].Reset();
        return true; // STOP EXECUTION
    }

    inline bool Matcher::RunContStack(const Char* const input, CharCount &inputOffset, const uint8 *&instPointer, ContStack &contStack, AssertionStack &assertionStack, uint &qcTicks)
    {
        while (true)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            PopStats(contStack, input);
#endif
            Cont* cont = contStack.Pop();
            if (cont == 0)
            {
                break;
            }

            Assert(cont->tag >= minContTag && cont->tag <= maxContTag);
            // All these cases RESUME EXECUTION if backtracking finds a stop point
            const Cont::ContTag tag = cont->tag;
            switch (tag)
            {
#define M(O) case Cont::ContTag::O: if (((O##Cont*)cont)->Exec(*this, input, inputOffset, instPointer, contStack, assertionStack, qcTicks)) return false; break;
#include "RegexContcodes.h"
#undef M
            default:
                Assert(false); // should never be reached
                return false;  // however, can't use complier optimization if we wnat to return false here
            }
        }
        return true;
    }

#if DBG
    const Inst::InstTag instTags[] = {
#define M(TagName) Inst::InstTag::TagName,
#define MTemplate(TagName, ...) M(TagName)
#include "RegexOpCodes.h"
#undef M
#undef MTemplate
    };

    const Inst::InstTag minInstTag = instTags[0];
    const Inst::InstTag maxInstTag = instTags[(sizeof(instTags) / sizeof(Inst::InstTag)) - 1];
#endif

    inline void Matcher::Run(const Char* const input, const CharCount inputLength, CharCount &matchStart, CharCount &nextSyncInputOffset, ContStack &contStack, AssertionStack &assertionStack, uint &qcTicks, bool firstIteration)
    {
        CharCount inputOffset = matchStart;
        const uint8 *instPointer = program->rep.insts.insts;
        Assert(instPointer != 0);

        while (true)
        {
            Assert(inputOffset >= matchStart && inputOffset <= inputLength);
            Assert(instPointer >= program->rep.insts.insts && instPointer < program->rep.insts.insts + program->rep.insts.instsLen);
            Assert(((Inst*)instPointer)->tag >= minInstTag && ((Inst*)instPointer)->tag <= maxInstTag);
#if ENABLE_REGEX_CONFIG_OPTIONS
            if (w != 0)
            {
                Print(w, input, inputLength, inputOffset, instPointer, contStack, assertionStack);
            }
            InstStats();
#endif
            const Inst *inst = (const Inst*)instPointer;
            const Inst::InstTag tag = inst->tag;
            switch (tag)
            {
#define MBase(TagName, ClassName) \
                case Inst::InstTag::TagName: \
                    if (((const ClassName *)inst)->Exec(*this, input, inputLength, matchStart, inputOffset, nextSyncInputOffset, instPointer, contStack, assertionStack, qcTicks, firstIteration)) { return; } \
                    break;
#define M(TagName) MBase(TagName, TagName##Inst)
#define MTemplate(TagName, TemplateDeclaration, GenericClassName, SpecializedClassName) MBase(TagName, SpecializedClassName)
#include "RegexOpCodes.h"
#undef MBase
#undef M
#undef MTemplate
            default:
                Assert(false);
                __assume(false);
            }
        }
    }

#if DBG
    void Matcher::ResetLoopInfos()
    {
        for (int i = 0; i < program->numLoops; i++)
        {
            loopInfos[i].Reset();
        }
    }
#endif

    inline bool Matcher::MatchHere(const Char* const input, const CharCount inputLength, CharCount &matchStart, CharCount &nextSyncInputOffset, ContStack &contStack, AssertionStack &assertionStack, uint &qcTicks, bool firstIteration)
    {
        // Reset the continuation and assertion stacks ready for fresh run
        // NOTE: We used to do this after the Run, but it's safer to do it here in case unusual control flow exits
        //       the matcher without executing the clears.
        contStack.Clear();
        // assertionStack may be non-empty since we can hard fail directly out of matcher without popping assertion
        assertionStack.Clear();

        Assert(contStack.IsEmpty());
        Assert(assertionStack.IsEmpty());

        ResetInnerGroups(0, program->numGroups - 1);
#if DBG
        ResetLoopInfos();
#endif

        Run(input, inputLength, matchStart, nextSyncInputOffset, contStack, assertionStack, qcTicks, firstIteration);
        // Leave the continuation and assertion stack memory in place so we don't have to alloc next time

        return WasLastMatchSuccessful();
    }

    inline bool Matcher::MatchSingleCharCaseInsensitive(const Char* const input, const CharCount inputLength, CharCount offset, const Char c)
    {
        CaseInsensitive::MappingSource mappingSource = program->GetCaseMappingSource();

        // If sticky flag is present, break since the 1st character didn't match the pattern character
        if ((program->flags & StickyRegexFlag) != 0)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            CompStats();
#endif
            if (MatchSingleCharCaseInsensitiveHere(mappingSource, input, offset, c))
            {
                GroupInfo* const info = GroupIdToGroupInfo(0);
                info->offset = offset;
                info->length = 1;
                return true;
            }
            else
            {
                ResetGroup(0);
                return false;
            }
        }

        while (offset < inputLength)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            CompStats();
#endif
            if (MatchSingleCharCaseInsensitiveHere(mappingSource, input, offset, c))
            {
                GroupInfo* const info = GroupIdToGroupInfo(0);
                info->offset = offset;
                info->length = 1;
                return true;
            }
            offset++;
        }

        ResetGroup(0);
        return false;
    }

    inline bool Matcher::MatchSingleCharCaseInsensitiveHere(
        CaseInsensitive::MappingSource mappingSource,
        const Char* const input,
        const CharCount offset,
        const Char c)
    {
        return (standardChars->ToCanonical(mappingSource, input[offset]) == standardChars->ToCanonical(mappingSource, c));
    }

    inline bool Matcher::MatchSingleCharCaseSensitive(const Char* const input, const CharCount inputLength, CharCount offset, const Char c)
    {
        // If sticky flag is present, break since the 1st character didn't match the pattern character
        if ((program->flags & StickyRegexFlag) != 0)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            CompStats();
#endif
            if (input[offset] == c)
            {
                GroupInfo* const info = GroupIdToGroupInfo(0);
                info->offset = offset;
                info->length = 1;
                return true;
            }
            else
            {
                ResetGroup(0);
                return false;
            }
        }

        while (offset < inputLength)
        {
#if ENABLE_REGEX_CONFIG_OPTIONS
            CompStats();
#endif
            if (input[offset] == c)
            {
                GroupInfo* const info = GroupIdToGroupInfo(0);
                info->offset = offset;
                info->length = 1;
                return true;
            }
            offset++;
        }

        ResetGroup(0);
        return false;
    }

    inline bool Matcher::MatchBoundedWord(const Char* const input, const CharCount inputLength, CharCount offset)
    {
        const StandardChars<Char>& stdchrs = *standardChars;

        if (offset >= inputLength)
        {
            ResetGroup(0);
            return false;
        }

#if ENABLE_REGEX_CONFIG_OPTIONS
        CompStats();
#endif

        if ((offset == 0 && stdchrs.IsWord(input[0])) ||
            (offset > 0 && (!stdchrs.IsWord(input[offset - 1]) && stdchrs.IsWord(input[offset]))))
        {
            // Already at start of word
        }
        // If sticky flag is present, return false since we are not at the beginning of the word yet
        else if ((program->flags & StickyRegexFlag) == StickyRegexFlag)
        {
            ResetGroup(0);
            return false;
        }
        else
        {
            if (stdchrs.IsWord(input[offset]))
            {
                // Scan for end of current word
                while (true)
                {
                    offset++;
                    if (offset >= inputLength)
                    {
                        ResetGroup(0);
                        return false;
                    }
#if ENABLE_REGEX_CONFIG_OPTIONS
                    CompStats();
#endif
                    if (!stdchrs.IsWord(input[offset]))
                    {
                        break;
                    }
                }
            }

            // Scan for start of next word
            while (true)
            {
                offset++;
                if (offset >= inputLength)
                {
                    ResetGroup(0);
                    return false;
                }
#if ENABLE_REGEX_CONFIG_OPTIONS
                CompStats();
#endif
                if (stdchrs.IsWord(input[offset]))
                {
                    break;
                }
            }
        }

        GroupInfo* const info = GroupIdToGroupInfo(0);
        info->offset = offset;

        // Scan for end of word
        do
        {
            offset++;
#if ENABLE_REGEX_CONFIG_OPTIONS
            CompStats();
#endif
        }
        while (offset < inputLength && stdchrs.IsWord(input[offset]));

        info->length = offset - info->offset;
        return true;
    }

    inline bool Matcher::MatchLeadingTrailingSpaces(const Char* const input, const CharCount inputLength, CharCount offset)
    {
        GroupInfo* const info = GroupIdToGroupInfo(0);
        Assert(offset <= inputLength);
        Assert((program->flags & MultilineRegexFlag) == 0);

        if (offset >= inputLength)
        {
            Assert(offset == inputLength);
            if (program->rep.leadingTrailingSpaces.endMinMatch == 0 ||
                (offset == 0 && program->rep.leadingTrailingSpaces.beginMinMatch == 0))
            {
                info->offset = offset;
                info->length = 0;
                return true;
            }
            info->Reset();
            return false;
        }

        const StandardChars<Char> &stdchrs = *standardChars;
        if (offset == 0)
        {
            while (offset < inputLength && stdchrs.IsWhitespaceOrNewline(input[offset]))
            {
                offset++;
#if ENABLE_REGEX_CONFIG_OPTIONS
                CompStats();
#endif
            }
            if (offset >= program->rep.leadingTrailingSpaces.beginMinMatch)
            {
                info->offset = 0;
                info->length = offset;
                return true;
            }
        }

        Assert(inputLength > 0);
        const CharCount initOffset = offset;
        offset = inputLength - 1;
        while (offset >= initOffset && stdchrs.IsWhitespaceOrNewline(input[offset]))
        {
            // This can never underflow since initOffset > 0
            Assert(offset > 0);
            offset--;
#if ENABLE_REGEX_CONFIG_OPTIONS
            CompStats();
#endif
        }
        offset++;
        CharCount length = inputLength - offset;
        if (length >= program->rep.leadingTrailingSpaces.endMinMatch)
        {
            info->offset = offset;
            info->length = length;
            return true;
        }
        info->Reset();
        return false;
    }

    inline bool Matcher::MatchOctoquad(const Char* const input, const CharCount inputLength, CharCount offset, OctoquadMatcher* matcher)
    {
        if (matcher->Match
            ( input
            , inputLength
            , offset
#if ENABLE_REGEX_CONFIG_OPTIONS
            , stats
#endif
            ))
        {
            GroupInfo* const info = GroupIdToGroupInfo(0);
            info->offset = offset;
            info->length = TrigramInfo::PatternLength;
            return true;
        }
        else
        {
            ResetGroup(0);
            return false;
        }
    }

    inline bool Matcher::MatchBOILiteral2(const Char* const input, const CharCount inputLength, CharCount offset, DWORD literal2)
    {
        if (offset == 0 && inputLength >= 2)
        {
            CompileAssert(sizeof(Char) == 2);
            const Program * program = this->program;
            if (program->rep.boiLiteral2.literal == *(DWORD *)input)
            {
                GroupInfo* const info = GroupIdToGroupInfo(0);
                info->offset = 0;
                info->length = 2;
                return true;
            }
        }
        ResetGroup(0);
        return false;
    }

    bool Matcher::Match
        ( const Char* const input
        , const CharCount inputLength
        , CharCount offset
        , Js::ScriptContext * scriptContext
#if ENABLE_REGEX_CONFIG_OPTIONS
        , RegexStats* stats
        , DebugWriter* w
#endif
        )
    {
#if ENABLE_REGEX_CONFIG_OPTIONS
        this->stats = stats;
        this->w = w;
#endif

        Assert(offset <= inputLength);
        bool res;
        bool loopMatchHere = true;
        Program const *prog = this->program;
        bool isStickyPresent = this->pattern->IsSticky();
        switch (prog->tag)
        {
        case Program::ProgramTag::BOIInstructionsTag:
            if (offset != 0)
            {
                groupInfos[0].Reset();
                res = false;
                break;
            }

            // fall through

        case Program::ProgramTag::BOIInstructionsForStickyFlagTag:
            AssertMsg(prog->tag == Program::ProgramTag::BOIInstructionsTag || isStickyPresent, "prog->tag should be BOIInstructionsForStickyFlagTag if sticky = true.");

            loopMatchHere = false;

            // fall through

        case Program::ProgramTag::InstructionsTag:
            {
                previousQcTime = 0;
                uint qcTicks = 0;

                // This is the next offset in the input from where we will try to sync. For sync instructions that back up, this
                // is used to avoid trying to sync when we have not yet reached the offset in the input we last synced to before
                // backing up.
                CharCount nextSyncInputOffset = offset;

                RegexStacks * regexStacks = scriptContext->RegexStacks();

                // Need to continue matching even if matchStart == inputLim since some patterns may match an empty string at the end
                // of the input. For instance: /a*$/.exec("b")
                bool firstIteration = true;
                do
                {
                    // Let there be only one call to MatchHere(), as that call expands the interpreter loop in-place. Having
                    // multiple calls to MatchHere() would bloat the code.
                    res = MatchHere(input, inputLength, offset, nextSyncInputOffset, regexStacks->contStack, regexStacks->assertionStack, qcTicks, firstIteration);
                    firstIteration = false;
                } while(!res && loopMatchHere && ++offset <= inputLength);

                break;
            }

        case Program::ProgramTag::SingleCharTag:
            if (this->pattern->IsIgnoreCase())
            {
                res = MatchSingleCharCaseInsensitive(input, inputLength, offset, prog->rep.singleChar.c);
            }
            else
            {
                res = MatchSingleCharCaseSensitive(input, inputLength, offset, prog->rep.singleChar.c);
            }

            break;

        case Program::ProgramTag::BoundedWordTag:
            res = MatchBoundedWord(input, inputLength, offset);
            break;

        case Program::ProgramTag::LeadingTrailingSpacesTag:
            res = MatchLeadingTrailingSpaces(input, inputLength, offset);
            break;

        case Program::ProgramTag::OctoquadTag:
            res = MatchOctoquad(input, inputLength, offset, prog->rep.octoquad.matcher);
            break;

        case Program::ProgramTag::BOILiteral2Tag:
            res = MatchBOILiteral2(input, inputLength, offset, prog->rep.boiLiteral2.literal);
            break;

        default:
            Assert(false);
            __assume(false);
        }

#if ENABLE_REGEX_CONFIG_OPTIONS
        this->stats = 0;
        this->w = 0;
#endif

        return res;
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Matcher::Print(DebugWriter* w, const Char* const input, const CharCount inputLength, CharCount inputOffset, const uint8* instPointer, ContStack &contStack, AssertionStack &assertionStack) const
    {
        w->PrintEOL(_u("Matcher {"));
        w->Indent();
        w->Print(_u("program:      "));
        w->PrintQuotedString(program->source, program->sourceLen);
        w->EOL();
        w->Print(_u("inputPointer: "));
        if (inputLength == 0)
        {
            w->PrintEOL(_u("<empty input>"));
        }
        else if (inputLength > 1024)
        {
            w->PrintEOL(_u("<string too large>"));
        }
        else
        {
            w->PrintEscapedString(input, inputOffset);
            if (inputOffset >= inputLength)
            {
                w->Print(_u("<<<>>>"));
            }
            else
            {
                w->Print(_u("<<<"));
                w->PrintEscapedChar(input[inputOffset]);
                w->Print(_u(">>>"));
                w->PrintEscapedString(input + inputOffset + 1, inputLength - inputOffset - 1);
            }
            w->EOL();
        }
        if (program->tag == Program::ProgramTag::BOIInstructionsTag || program->tag == Program::ProgramTag::InstructionsTag)
        {
            w->Print(_u("instPointer: "));

            const Inst* inst = (const Inst*)instPointer;
            switch (inst->tag)
            {
#define MBase(TagName, ClassName) \
            case Inst::InstTag::TagName: \
            { \
                const ClassName *actualInst = static_cast<const ClassName *>(inst); \
                actualInst->Print(w, InstPointerToLabel(instPointer), program->rep.insts.litbuf); \
                break; \
            }
#define M(TagName) MBase(TagName, TagName##Inst)
#define MTemplate(TagName, TemplateDeclaration, GenericClassName, SpecializedClassName) MBase(TagName, SpecializedClassName)
#include "RegexOpCodes.h"
#undef MBase
#undef M
#undef MTemplate
            default:
                Assert(false);
                __assume(false);
            }

            w->PrintEOL(_u("groups:"));
            w->Indent();
            for (int i = 0; i < program->numGroups; i++)
            {
                w->Print(_u("%d: "), i);
                groupInfos[i].Print(w, input);
                w->EOL();
            }
            w->Unindent();
            w->PrintEOL(_u("loops:"));
            w->Indent();
            for (int i = 0; i < program->numLoops; i++)
            {
                w->Print(_u("%d: "), i);
                loopInfos[i].Print(w);
                w->EOL();
            }
            w->Unindent();
            w->PrintEOL(_u("contStack: (top to bottom)"));
            w->Indent();
            contStack.Print(w, input);
            w->Unindent();
            w->PrintEOL(_u("assertionStack: (top to bottom)"));
            w->Indent();
            assertionStack.Print(w, this);
            w->Unindent();
        }
        w->Unindent();
        w->PrintEOL(_u("}"));
        w->Flush();
    }
#endif

    // ----------------------------------------------------------------------
    // Program
    // ----------------------------------------------------------------------

    Program::Program(RegexFlags flags)
        : source(nullptr)
        , sourceLen(0)
        , flags(flags)
        , numGroups(0)
        , numLoops(0)
    {
        tag = ProgramTag::InstructionsTag;
        rep.insts.insts = nullptr;
        rep.insts.instsLen = 0;
        rep.insts.litbuf = nullptr;
        rep.insts.litbufLen = 0;
        rep.insts.scannersForSyncToLiterals = nullptr;
    }

    Program *Program::New(Recycler *recycler, RegexFlags flags)
    {
        return RecyclerNew(recycler, Program, flags);
    }

    Field(ScannerInfo *)*Program::CreateScannerArrayForSyncToLiterals(Recycler *const recycler)
    {
        Assert(tag == ProgramTag::InstructionsTag);
        Assert(!rep.insts.scannersForSyncToLiterals);
        Assert(recycler);

        return
            rep.insts.scannersForSyncToLiterals =
                RecyclerNewArrayZ(recycler, Field(ScannerInfo *), ScannersMixin::MaxNumSyncLiterals);
    }

    ScannerInfo *Program::AddScannerForSyncToLiterals(
        Recycler *const recycler,
        const int scannerIndex,
        const CharCount offset,
        const CharCount length,
        const bool isEquivClass)
    {
        Assert(tag == ProgramTag::InstructionsTag);
        Assert(rep.insts.scannersForSyncToLiterals);
        Assert(recycler);
        Assert(scannerIndex >= 0);
        Assert(scannerIndex < ScannersMixin::MaxNumSyncLiterals);
        Assert(!rep.insts.scannersForSyncToLiterals[scannerIndex]);

        return
            rep.insts.scannersForSyncToLiterals[scannerIndex] =
                RecyclerNewLeaf(recycler, ScannerInfo, offset, length, isEquivClass);
    }

    void Program::FreeBody(ArenaAllocator* rtAllocator)
    {
        if (tag != ProgramTag::InstructionsTag || !rep.insts.insts)
        {
            return;
        }

        Inst *inst = reinterpret_cast<Inst *>(PointerValue(rep.insts.insts));
        const auto instEnd = reinterpret_cast<Inst *>(reinterpret_cast<uint8 *>(inst) + rep.insts.instsLen);
        Assert(inst < instEnd);
        do
        {
            switch(inst->tag)
            {
#define MBase(TagName, ClassName) \
                case Inst::InstTag::TagName: \
                { \
                    const auto actualInst = static_cast<ClassName *>(inst); \
                    actualInst->FreeBody(rtAllocator); \
                    inst = actualInst + 1; \
                    break; \
                }
#define M(TagName) MBase(TagName, TagName##Inst)
#define MTemplate(TagName, TemplateDeclaration, GenericClassName, SpecializedClassName) MBase(TagName, SpecializedClassName)
#include "RegexOpCodes.h"
#undef MBase
#undef M
#undef MTemplate
                default:
                    Assert(false);
                    __assume(false);
            }
        } while(inst < instEnd);
        Assert(inst == instEnd);

#if DBG
        rep.insts.insts = nullptr;
        rep.insts.instsLen = 0;
#endif
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    void Program::Print(DebugWriter* w)
    {
        const bool isBaselineMode = Js::Configuration::Global.flags.BaselineMode;
        w->PrintEOL(_u("Program {"));
        w->Indent();
        w->PrintEOL(_u("source:       %s"), PointerValue(source));

        w->Print(_u("litbuf:       "));
        const char16 *litbuf = this->rep.insts.litbuf;
        size_t litbufLen = 0;
        if (litbuf == nullptr)
        {
            w->PrintEOL(_u("<NONE>"));
        }
        else
        {
            litbufLen = this->rep.insts.litbufLen;
            for (size_t i = 0; i < litbufLen; ++i)
            {
                const char16 c = (char16)litbuf[i];
                w->PrintEscapedChar(c);
            }
            w->PrintEOL(_u(""));
        }
        w->PrintEOL(_u("litbufLen:    %u"), litbufLen);

        w->Print(_u("flags:        "));
        if ((flags & GlobalRegexFlag) != 0) w->Print(_u("global "));
        if ((flags & MultilineRegexFlag) != 0) w->Print(_u("multiline "));
        if ((flags & IgnoreCaseRegexFlag) != 0) w->Print(_u("ignorecase "));
        if ((flags & DotAllRegexFlag) != 0) w->Print(_u("dotAll "));
        if ((flags & UnicodeRegexFlag) != 0) w->Print(_u("unicode "));
        if ((flags & StickyRegexFlag) != 0) w->Print(_u("sticky "));
        w->EOL();
        w->PrintEOL(_u("numGroups:    %d"), numGroups);
        w->PrintEOL(_u("numLoops:     %d"), numLoops);
        switch (tag)
        {
        case ProgramTag::BOIInstructionsTag:
        case ProgramTag::InstructionsTag:
            {
                w->PrintEOL(_u("instructions: {"));
                w->Indent();
                if (tag == ProgramTag::BOIInstructionsTag)
                {
                    w->PrintEOL(_u("       BOITest(hardFail: true)"));
                }
                uint8* instsLim = rep.insts.insts + rep.insts.instsLen;
                uint8* curr = rep.insts.insts;
                int i = 0;
                while (curr != instsLim)
                {
                    const Inst *inst = (const Inst*)curr;
                    switch (inst->tag)
                    {
#define MBase(TagName, ClassName) \
                    case Inst::InstTag::TagName: \
                    { \
                        const ClassName *actualInst = static_cast<const ClassName *>(inst); \
                        curr += actualInst->Print(w, (Label)(isBaselineMode ? i++ : curr - rep.insts.insts), rep.insts.litbuf); \
                        break; \
                    }
#define M(TagName) MBase(TagName, TagName##Inst)
#define MTemplate(TagName, TemplateDeclaration, GenericClassName, SpecializedClassName) MBase(TagName, SpecializedClassName)
#include "RegexOpCodes.h"
#undef MBase
#undef M
#undef MTemplate
                    default:
                        Assert(false);
                        __assume(false);
                    }
                }
                w->Unindent();
                w->PrintEOL(_u("}"));
            }
            break;
        case ProgramTag::SingleCharTag:
            w->Print(_u("special form: <match single char "));
            w->PrintQuotedChar(rep.singleChar.c);
            w->PrintEOL(_u(">"));
            break;
        case ProgramTag::BoundedWordTag:
            w->PrintEOL(_u("special form: <match bounded word>"));
            break;
        case ProgramTag::LeadingTrailingSpacesTag:
            w->PrintEOL(_u("special form: <match leading/trailing spaces: minBegin=%d minEnd=%d>"),
                rep.leadingTrailingSpaces.beginMinMatch, rep.leadingTrailingSpaces.endMinMatch);
            break;
        case ProgramTag::OctoquadTag:
            w->Print(_u("special form: <octoquad "));
            rep.octoquad.matcher->Print(w);
            w->PrintEOL(_u(">"));
            break;
        }
        w->Unindent();
        w->PrintEOL(_u("}"));
    }
#endif

    // Template parameter here is the max number of cases
    template void UnifiedRegex::SwitchMixin<2>::AddCase(char16, Label);
    template void UnifiedRegex::SwitchMixin<4>::AddCase(char16, Label);
    template void UnifiedRegex::SwitchMixin<8>::AddCase(char16, Label);
    template void UnifiedRegex::SwitchMixin<16>::AddCase(char16, Label);
    template void UnifiedRegex::SwitchMixin<24>::AddCase(char16, Label);

#define M(...)
#define MTemplate(TagName, TemplateDeclaration, GenericClassName, SpecializedClassName) template struct SpecializedClassName;
#include "RegexOpCodes.h"
#undef M
#undef MTemplate
}
