//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    struct RegexMatchState;

    class RegexHelper
    {
        static const int MinTrigramInputLength=250000;

        //
        // Dynamic compilation
        //

        static bool GetFlags(ScriptContext* scriptContext, __in_ecount(strLen) const char16* str, CharCount strLen, UnifiedRegex::RegexFlags &flags);
    public:
        static UnifiedRegex::RegexPattern* CompileDynamic(ScriptContext *scriptContext, const char16* psz, CharCount csz, const char16* pszOpts, CharCount cszOpts, bool isLiteralSource);
        static UnifiedRegex::RegexPattern* CompileDynamic(ScriptContext *scriptContext, const char16* psz, CharCount csz, UnifiedRegex::RegexFlags flags, bool isLiteralSource);
    private:
        static UnifiedRegex::RegexPattern* PrimCompileDynamic(ScriptContext *scriptContext, const char16* psz, CharCount csz, const char16* pszOpts, CharCount cszOpts, bool isLiteralSource);

        //
        // Primitives
        //

    public:
        static UnifiedRegex::GroupInfo SimpleMatch(ScriptContext * scriptContext, UnifiedRegex::RegexPattern * pattern, const char16 * inputStr,  CharCount inputLength, CharCount offset);
        static Var NonMatchValue(ScriptContext* scriptContext, bool isGlobalCtor);
        static Var GetString(ScriptContext* scriptContext, JavascriptString* input, Var nonMatchValue, UnifiedRegex::GroupInfo group);
        static Var GetGroup(ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern, JavascriptString* input, Var nonMatchValue, int groupId);
    private:
        static void PropagateLastMatch
            ( ScriptContext* scriptContext
            , bool isGlobal
            , bool isSticky
            , JavascriptRegExp* regularExpression
            , JavascriptString* lastInput
            , UnifiedRegex::GroupInfo lastSuccessfulMatch
            , UnifiedRegex::GroupInfo lastActualMatch
            , bool updateRegex
            , bool updateCtor
            , bool useSplitPattern = false );
        static void PropagateLastMatchToRegex
            ( ScriptContext* scriptContext
            , bool isGlobal
            , bool isSticky
            , JavascriptRegExp* regularExpression
            , UnifiedRegex::GroupInfo lastSuccessfulMatch
            , UnifiedRegex::GroupInfo lastActualMatch );
        static void PropagateLastMatchToCtor
            ( ScriptContext* scriptContext
            , JavascriptRegExp* regularExpression
            , JavascriptString* lastInput
            , UnifiedRegex::GroupInfo lastSuccessfulMatch
            , bool useSplitPattern );

        static bool GetInitialOffset(bool isGlobal, bool isSticky, JavascriptRegExp* regularExpression, CharCount inputLength, CharCount& offset);
        static JavascriptArray* CreateMatchResult(void *const stackAllocationPointer, ScriptContext* scriptContext, bool isGlobal, int numGroups, JavascriptString* input);
        static void FinalizeMatchResult(ScriptContext* scriptContext, bool isGlobal, JavascriptArray* arr, UnifiedRegex::GroupInfo match);
        static JavascriptArray* CreateExecResult(void *const stackAllocationPointer, ScriptContext* scriptContext, int numGroups, JavascriptString* input, UnifiedRegex::GroupInfo match);
        template<typename T> static T CheckCrossContextAndMarshalResult(T value, ScriptContext* targetContext);

        //
        // Regex entry points
        //

    public:
        static Var RegexMatchResultUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var RegexMatchResultUsedAndMayBeTemp(void *const stackAllocationPointer, ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var RegexMatchResultNotUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var RegexMatch(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, bool noResult, void *const stackAllocationPointer = nullptr);
        static Var RegexMatchNoHistory(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, bool noResult);
        static Var RegexExecResultUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var RegexExecResultUsedAndMayBeTemp(void *const stackAllocationPointer, ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var RegexExecResultNotUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var RegexExec(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, bool noResult, void *const stackAllocationPointer = nullptr);
        static Var RegexTest(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input);
        template<bool mustMatchEntireInput> static BOOL RegexTest_NonScript(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, const char16 *const input, const CharCount inputLength);

    private:
        static void PrimBeginMatch(RegexMatchState& state, ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern, const char16* input, CharCount inputLength, bool alwaysNeedAlloc);
        static UnifiedRegex::GroupInfo PrimMatch(RegexMatchState& state, ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern, CharCount inputLength, CharCount offset);
        static void PrimEndMatch(RegexMatchState& state, ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern);

        template<typename GroupFn>
        static void ReplaceFormatString
            ( ScriptContext* scriptContext
            , int numGroups
            , GroupFn getGroup
            , JavascriptString* input
            , const char16* matchedString
            , UnifiedRegex::GroupInfo match
            , JavascriptString* replace
            , int substitutions
            , __in_ecount(substitutions) CharCount* substitutionOffsets
            , CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)>& concatenated );

    public:
        static Var RegexReplaceResultUsed(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace);
        static Var RegexReplaceResultNotUsed(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace);
        static Var RegexReplace(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptString* replace, bool noResult);
        static Var RegexReplaceFunction(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptFunction* replacefn);
        static Var StringReplace(JavascriptString* regularExpression, JavascriptString* input, JavascriptString* replace);
        static Var StringReplace(ScriptContext* scriptContext, JavascriptString* regularExpression, JavascriptString* input, JavascriptFunction* replacefn);
        static Var RegexSplitResultUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit);
        static Var RegexSplitResultUsedAndMayBeTemp(void *const stackAllocationPointer, ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit);
        static Var RegexSplitResultNotUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit);
        static Var RegexSplit(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer = nullptr);
        static Var RegexSearch(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static Var StringSplit(JavascriptString* regularExpression, JavascriptString* input, CharCount limit);
        static bool IsResultNotUsed(CallFlags flags);

    private:
        static void AppendSubString(ScriptContext* scriptContext, JavascriptArray* ary, JavascriptString* input, CharCount startInclusive, CharCount endExclusive);
        template <bool updateHistory>
        static Var RegexMatchImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, bool noResult, void *const stackAllocationPointer = nullptr);
        static bool IsRegexSymbolMatchObservable(RecyclableObject* instance, ScriptContext* scriptContext);
        static Var RegexEs6MatchImpl(ScriptContext* scriptContext, RecyclableObject *thisObj, JavascriptString *input, bool noResult, void *const stackAllocationPointer);
        template <bool updateHistory>
        static Var RegexEs5MatchImpl(ScriptContext* scriptContext, JavascriptRegExp *regularExpression, JavascriptString *input, bool noResult, void *const stackAllocationPointer = nullptr);
        static Var RegexExecImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, bool noResult, void *const stackAllocationPointer = nullptr);
        static Var RegexEs5Replace(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace, bool noResult);
        static Var RegexReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptString* replace, bool noResult);
        static bool IsRegexSymbolReplaceObservable(RecyclableObject* instance, ScriptContext* scriptContext);
        static Var RegexEs6ReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptString* replace, bool noResult);
        static Var RegexEs6ReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptFunction* replaceFn);
        template<typename ReplacementFn>
        static Var RegexEs6ReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, ReplacementFn appendReplacement, bool noResult);
        static Var RegexEs5ReplaceImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace, bool noResult);
        static Var RegexReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptFunction* replacefn);
        static Var RegexEs5ReplaceImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptFunction* replacefn);
        static Var RegexSearchImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        inline static UnifiedRegex::RegexPattern *GetSplitPattern(ScriptContext* scriptContext, JavascriptRegExp *regularExpression);
        static bool IsRegexSymbolSplitObservable(RecyclableObject* instance, ScriptContext* scriptContext);
        static Var RegexSplitImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer = nullptr);
        static Var RegexEs6SplitImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer = nullptr);
        static JavascriptString* AppendStickyToFlagsIfNeeded(JavascriptString* flags, ScriptContext* scriptContext);
        static Var RegexEs5SplitImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer = nullptr);
        static bool IsRegexTestObservable(RecyclableObject* instance, ScriptContext* scriptContext);
        static Var RegexEs6TestImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input);
        static Var RegexEs5TestImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input);
        static int GetReplaceSubstitutions(const char16 * const replaceStr, CharCount const replaceLength, ArenaAllocator * const tempAllocator, CharCount** const substitutionOffsetsOut);
        static RecyclableObject* ExecResultToRecyclableObject(Var result);
        static JavascriptString* GetMatchStrFromResult(RecyclableObject* result, ScriptContext* scriptContext);
        static void AdvanceLastIndex(RecyclableObject* instance, JavascriptString* input, JavascriptString* matchStr, bool unicode, ScriptContext* scriptContext);
        static charcount_t AdvanceStringIndex(JavascriptString* string, charcount_t index, bool isUnicode);
    };
}
