//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

// Parser Includes
#include "DebugWriter.h"
#include "RegexStats.h"
#include "OctoquadIdentifier.h"
#include "RegexCompileTime.h"
#include "RegexParser.h"
#include "RegexPattern.h"

namespace Js
{
    // ----------------------------------------------------------------------
    // Dynamic compilation
    // ----------------------------------------------------------------------

    // See also:
    //    UnifiedRegex::Parser::Options(...)
    bool RegexHelper::GetFlags(Js::ScriptContext* scriptContext, __in_ecount(strLen) const char16* str, CharCount strLen, UnifiedRegex::RegexFlags &flags)
    {
        for (CharCount i = 0; i < strLen; i++)
        {
            switch (str[i])
            {
            case 'i':
                if ((flags & UnifiedRegex::IgnoreCaseRegexFlag) != 0)
                    return false;
                flags = (UnifiedRegex::RegexFlags)(flags | UnifiedRegex::IgnoreCaseRegexFlag);
                break;
            case 'g':
                if ((flags & UnifiedRegex::GlobalRegexFlag) != 0)
                    return false;
                flags = (UnifiedRegex::RegexFlags)(flags | UnifiedRegex::GlobalRegexFlag);
                break;
            case 'm':
                if ((flags & UnifiedRegex::MultilineRegexFlag) != 0)
                    return false;
                flags = (UnifiedRegex::RegexFlags)(flags | UnifiedRegex::MultilineRegexFlag);
                break;
            case 'u':
                if (scriptContext->GetConfig()->IsES6UnicodeExtensionsEnabled())
                {
                    if((flags & UnifiedRegex::UnicodeRegexFlag) != 0)
                        return false;
                    flags = (UnifiedRegex::RegexFlags)(flags | UnifiedRegex::UnicodeRegexFlag);
                    break;
                }
                return false;
            case 'y':
                if (scriptContext->GetConfig()->IsES6RegExStickyEnabled())
                {
                    if ((flags & UnifiedRegex::StickyRegexFlag) != 0)
                        return false;
                    flags = (UnifiedRegex::RegexFlags)(flags | UnifiedRegex::StickyRegexFlag);
                    break;
                }
                return false;
            default:
                return false;
            }
        }

        return true;
    }

    UnifiedRegex::RegexPattern* RegexHelper::CompileDynamic(ScriptContext *scriptContext, const char16* psz, CharCount csz, const char16* pszOpts, CharCount cszOpts, bool isLiteralSource)
    {
        Assert(psz != 0 && psz[csz] == 0);
        Assert(pszOpts != 0 || cszOpts == 0);
        Assert(pszOpts == 0 || pszOpts[cszOpts] == 0);

        UnifiedRegex::RegexFlags flags = UnifiedRegex::NoRegexFlags;

        if (pszOpts != NULL)
        {
            if (!GetFlags(scriptContext, pszOpts, cszOpts, flags))
            {
                // Compile in order to throw appropriate error for ill-formed flags
                PrimCompileDynamic(scriptContext, psz, csz, pszOpts, cszOpts, isLiteralSource);
                Assert(false);
            }
        }

        if(isLiteralSource)
        {
            // The source is from a literal regex, so we're cloning a literal regex. Don't use the dynamic regex MRU map since
            // these literal regex patterns' lifetimes are tied with the function body.
            return PrimCompileDynamic(scriptContext, psz, csz, pszOpts, cszOpts, isLiteralSource);
        }

        UnifiedRegex::RegexKey lookupKey(psz, csz, flags);
        UnifiedRegex::RegexPattern* pattern = nullptr;
        RegexPatternMruMap* dynamicRegexMap = scriptContext->GetDynamicRegexMap();
        if (!dynamicRegexMap->TryGetValue(lookupKey, &pattern))
        {
            pattern = PrimCompileDynamic(scriptContext, psz, csz, pszOpts, cszOpts, isLiteralSource);

            // WARNING: Must calculate key again so that dictionary has copy of source associated with the pattern
            const auto source = pattern->GetSource();
            UnifiedRegex::RegexKey finalKey(source.GetBuffer(), source.GetLength(), flags);
            dynamicRegexMap->Add(finalKey, pattern);
        }
        return pattern;
    }

    UnifiedRegex::RegexPattern* RegexHelper::CompileDynamic(
        ScriptContext *scriptContext, const char16* psz, CharCount csz, UnifiedRegex::RegexFlags flags, bool isLiteralSource)
    {
        //
        // Regex compilations are mostly string parsing based. To avoid duplicating validation rules,
        // generate a trivial options string right here on the stack and delegate to the string parsing
        // based implementation.
        //
        const CharCount OPT_BUF_SIZE = 6;
        char16 opts[OPT_BUF_SIZE];

        CharCount i = 0;
        if (flags & UnifiedRegex::IgnoreCaseRegexFlag)
        {
            opts[i++] = _u('i');
        }
        if (flags & UnifiedRegex::GlobalRegexFlag)
        {
            opts[i++] = _u('g');
        }
        if (flags & UnifiedRegex::MultilineRegexFlag)
        {
            opts[i++] = _u('m');
        }
        if (flags & UnifiedRegex::UnicodeRegexFlag)
        {
            Assert(scriptContext->GetConfig()->IsES6UnicodeExtensionsEnabled());
            opts[i++] = _u('u');
        }
        if (flags & UnifiedRegex::StickyRegexFlag)
        {
            Assert(scriptContext->GetConfig()->IsES6RegExStickyEnabled());
            opts[i++] = _u('y');
        }
        Assert(i < OPT_BUF_SIZE);
        opts[i] = NULL;

        return CompileDynamic(scriptContext, psz, csz, opts, i, isLiteralSource);
    }

    UnifiedRegex::RegexPattern* RegexHelper::PrimCompileDynamic(ScriptContext *scriptContext, const char16* psz, CharCount csz, const char16* pszOpts, CharCount cszOpts, bool isLiteralSource)
    {
        PROBE_STACK_NO_DISPOSE(scriptContext, Js::Constants::MinStackRegex);

        // SEE ALSO: Scanner<EncodingPolicy>::ScanRegExpConstant()
#ifdef PROFILE_EXEC
        scriptContext->ProfileBegin(Js::RegexCompilePhase);
#endif
        ArenaAllocator* rtAllocator = scriptContext->RegexAllocator();
#if ENABLE_REGEX_CONFIG_OPTIONS
        UnifiedRegex::DebugWriter *dw = 0;
        if (REGEX_CONFIG_FLAG(RegexDebug))
            dw = scriptContext->GetRegexDebugWriter();
        UnifiedRegex::RegexStats* stats = 0;
#endif
        UnifiedRegex::RegexFlags flags = UnifiedRegex::NoRegexFlags;

        if(csz == 0 && cszOpts == 0)
        {
            // Fast path for compiling the empty regex with empty flags, for the RegExp constructor object and other cases.
            // These empty regexes are dynamic regexes and so this fast path only exists for dynamic regex compilation. The
            // standard chars in particular, do not need to be initialized to compile this regex.
            UnifiedRegex::Program* program = UnifiedRegex::Program::New(scriptContext->GetRecycler(), flags);
            UnifiedRegex::Parser<NullTerminatedUnicodeEncodingPolicy, false>::CaptureEmptySourceAndNoGroups(program);
            UnifiedRegex::RegexPattern* pattern = UnifiedRegex::RegexPattern::New(scriptContext, program, false);
            UnifiedRegex::Compiler::CompileEmptyRegex
                ( program
                , pattern
#if ENABLE_REGEX_CONFIG_OPTIONS
                , dw
                , stats
#endif
                );
#ifdef PROFILE_EXEC
            scriptContext->ProfileEnd(Js::RegexCompilePhase);
#endif
            return pattern;
        }

#if ENABLE_REGEX_CONFIG_OPTIONS
        if (REGEX_CONFIG_FLAG(RegexProfile))
            scriptContext->GetRegexStatsDatabase()->BeginProfile();
#endif
        BEGIN_TEMP_ALLOCATOR(ctAllocator, scriptContext, _u("UnifiedRegexParseAndCompile"));
        UnifiedRegex::StandardChars<char16>* standardChars = scriptContext->GetThreadContext()->GetStandardChars((char16*)0);
        UnifiedRegex::Node* root = 0;
        UnifiedRegex::Parser<NullTerminatedUnicodeEncodingPolicy, false> parser
            ( scriptContext
            , ctAllocator
            , standardChars
            , standardChars
            , false
#if ENABLE_REGEX_CONFIG_OPTIONS
            , dw
#endif
            );
        try
        {
            root = parser.ParseDynamic(psz, psz + csz, pszOpts, pszOpts + cszOpts, flags);
        }
        catch (UnifiedRegex::ParseError e)
        {
            END_TEMP_ALLOCATOR(ctAllocator, scriptContext);
#ifdef PROFILE_EXEC
            scriptContext->ProfileEnd(Js::RegexCompilePhase);
#endif
            Js::JavascriptError::ThrowSyntaxError(scriptContext, e.error);
            // never reached
        }

        const auto recycler = scriptContext->GetRecycler();
        UnifiedRegex::Program* program = UnifiedRegex::Program::New(recycler, flags);
        parser.CaptureSourceAndGroups(recycler, program, psz, csz, csz);

        UnifiedRegex::RegexPattern* pattern = UnifiedRegex::RegexPattern::New(scriptContext, program, isLiteralSource);

#if ENABLE_REGEX_CONFIG_OPTIONS
        if (REGEX_CONFIG_FLAG(RegexProfile))
        {
            stats = scriptContext->GetRegexStatsDatabase()->GetRegexStats(pattern);
            scriptContext->GetRegexStatsDatabase()->EndProfile(stats, UnifiedRegex::RegexStats::Parse);
        }
        if (REGEX_CONFIG_FLAG(RegexTracing))
        {
            UnifiedRegex::DebugWriter* tw = scriptContext->GetRegexDebugWriter();
            tw->Print(_u("// REGEX COMPILE "));
            pattern->Print(tw);
            tw->EOL();
        }
        if (REGEX_CONFIG_FLAG(RegexProfile))
            scriptContext->GetRegexStatsDatabase()->BeginProfile();
#endif

        UnifiedRegex::Compiler::Compile
            ( scriptContext
            , ctAllocator
            , rtAllocator
            , standardChars
            , program
            , root
            , parser.GetLitbuf()
            , pattern
#if ENABLE_REGEX_CONFIG_OPTIONS
            , dw
            , stats
#endif
            );

#if ENABLE_REGEX_CONFIG_OPTIONS
        if (REGEX_CONFIG_FLAG(RegexProfile))
            scriptContext->GetRegexStatsDatabase()->EndProfile(stats, UnifiedRegex::RegexStats::Compile);
#endif

        END_TEMP_ALLOCATOR(ctAllocator, scriptContext);
#ifdef PROFILE_EXEC
        scriptContext->ProfileEnd(Js::RegexCompilePhase);
#endif

        return pattern;

    }

    // ----------------------------------------------------------------------
    // Primitives
    // ----------------------------------------------------------------------
#if ENABLE_REGEX_CONFIG_OPTIONS


    static void RegexHelperTrace(
        ScriptContext* scriptContext,
        UnifiedRegex::RegexStats::Use use,
        JavascriptRegExp* regExp,
        const char16 *const input,
        const CharCount inputLength,
        const char16 *const replace = 0,
        const CharCount replaceLength = 0)
    {
        Assert(regExp);
        Assert(input);

        if (REGEX_CONFIG_FLAG(RegexProfile))
        {
            UnifiedRegex::RegexStats* stats =
                scriptContext->GetRegexStatsDatabase()->GetRegexStats(regExp->GetPattern());
            stats->useCounts[use]++;
            stats->inputLength += inputLength;
        }
        if (REGEX_CONFIG_FLAG(RegexTracing))
        {
            UnifiedRegex::DebugWriter* w = scriptContext->GetRegexDebugWriter();
            w->Print(_u("%s("), UnifiedRegex::RegexStats::UseNames[use]);
            regExp->GetPattern()->Print(w);
            w->Print(_u(", "));
            if (!CONFIG_FLAG(Verbose) && inputLength > 1024)
                w->Print(_u("\"<string too large>\""));
            else
                w->PrintQuotedString(input, inputLength);
            if (replace != 0)
            {
                Assert(use == UnifiedRegex::RegexStats::Replace);
                w->Print(_u(", "));
                if (!CONFIG_FLAG(Verbose) && replaceLength > 1024)
                    w->Print(_u("\"<string too large>\""));
                else
                    w->PrintQuotedString(replace, replaceLength);
            }
            w->PrintEOL(_u(");"));
            w->Flush();
        }
    }

    static void RegexHelperTrace(ScriptContext* scriptContext, UnifiedRegex::RegexStats::Use use, JavascriptRegExp* regExp, JavascriptString* input)
    {
        Assert(regExp);
        Assert(input);

        RegexHelperTrace(scriptContext, use, regExp, input->GetString(), input->GetLength());
    }

    static void RegexHelperTrace(ScriptContext* scriptContext, UnifiedRegex::RegexStats::Use use, JavascriptRegExp* regExp, JavascriptString* input, JavascriptString* replace)
    {
        Assert(regExp);
        Assert(input);
        Assert(replace);

        RegexHelperTrace(scriptContext, use, regExp, input->GetString(), input->GetLength(), replace->GetString(), replace->GetLength());
    }
#endif

    // ----------------------------------------------------------------------
    // Regex entry points
    // ----------------------------------------------------------------------

    struct RegexMatchState
    {
        const char16* input;
        TempArenaAllocatorObject* tempAllocatorObj;
        UnifiedRegex::Matcher* matcher;
    };

    template <bool updateHistory>
    Var RegexHelper::RegexMatchImpl(ScriptContext* scriptContext, RecyclableObject *thisObj, JavascriptString *input, bool noResult, void *const stackAllocationPointer)
    {
        ScriptConfiguration const * scriptConfig = scriptContext->GetConfig();

        // Normally, this check would be done in JavascriptRegExp::EntrySymbolMatch. However,
        // since the lowerer inlines String.prototype.match and directly calls the helper,
        // the check then would be bypassed. That's the reason we do the check here.
        if (scriptConfig->IsES6RegExSymbolsEnabled()
            && IsRegexSymbolMatchObservable(thisObj, scriptContext))
        {
            // We don't need to pass "updateHistory" here since the call to "exec" will handle it.
            return RegexEs6MatchImpl(scriptContext, thisObj, input, noResult, stackAllocationPointer);
        }
        else
        {
            PCWSTR varName = scriptConfig->IsES6RegExSymbolsEnabled()
                ? _u("RegExp.prototype[Symbol.match]")
                : _u("String.prototype.match");
            JavascriptRegExp* regularExpression = JavascriptRegExp::ToRegExp(thisObj, varName, scriptContext);
            return RegexEs5MatchImpl<updateHistory>(scriptContext, regularExpression, input, noResult, stackAllocationPointer);
        }
    }

    bool RegexHelper::IsRegexSymbolMatchObservable(RecyclableObject* instance, ScriptContext* scriptContext)
    {
        DynamicObject* regexPrototype = scriptContext->GetLibrary()->GetRegExpPrototype();
        return !JavascriptRegExp::HasOriginalRegExType(instance)
            || JavascriptRegExp::HasObservableExec(regexPrototype)
            || JavascriptRegExp::HasObservableGlobalFlag(regexPrototype)
            || JavascriptRegExp::HasObservableUnicodeFlag(regexPrototype);
    }

    Var RegexHelper::RegexEs6MatchImpl(ScriptContext* scriptContext, RecyclableObject *thisObj, JavascriptString *input, bool noResult, void *const stackAllocationPointer)
    {
        PCWSTR const varName = _u("RegExp.prototype[Symbol.match]");

        if (!JavascriptRegExp::GetGlobalProperty(thisObj, scriptContext))
        {
            return JavascriptRegExp::CallExec(thisObj, input, varName, scriptContext);
        }
        else
        {
            bool unicode = JavascriptRegExp::GetUnicodeProperty(thisObj, scriptContext);

            JavascriptRegExp::SetLastIndexProperty(thisObj, TaggedInt::ToVarUnchecked(0), scriptContext);

            JavascriptArray* arrayResult = nullptr;

            do
            {
                Var result = JavascriptRegExp::CallExec(thisObj, input, varName, scriptContext);
                if (JavascriptOperators::IsNull(result))
                {
                    break;
                }

                RecyclableObject* resultObj = ExecResultToRecyclableObject(result);
                JavascriptString* matchStr = GetMatchStrFromResult(resultObj, scriptContext);

                if (arrayResult == nullptr)
                {
                    arrayResult = scriptContext->GetLibrary()->CreateArray();
                }

                arrayResult->DirectAppendItem(matchStr);

                AdvanceLastIndex(thisObj, input, matchStr, unicode, scriptContext);
            }
            while (true);

            return arrayResult != nullptr
                ? arrayResult
                : scriptContext->GetLibrary()->GetNull();
        }
    }

    // String.prototype.match (ES5 15.5.4.10)
    template <bool updateHistory>
    Var RegexHelper::RegexEs5MatchImpl(ScriptContext* scriptContext, JavascriptRegExp *regularExpression, JavascriptString *input, bool noResult, void *const stackAllocationPointer)
    {
        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
        const char16* inputStr = input->GetString();
        CharCount inputLength = input->GetLength();

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Match, regularExpression, input);
#endif

        UnifiedRegex::GroupInfo lastSuccessfulMatch; // initially undefined
        UnifiedRegex::GroupInfo lastActualMatch; // initially undefined

#ifdef REGEX_TRIGRAMS
        UnifiedRegex::TrigramAlphabet* trigramAlphabet = scriptContext->GetTrigramAlphabet();
        UnifiedRegex::TrigramInfo* trigramInfo= pattern->rep.unified.trigramInfo;
        if (trigramAlphabet!=NULL && inputLength>=MinTrigramInputLength && trigramInfo!=NULL)
        {
            if (trigramAlphabet->input==NULL)
                trigramAlphabet->MegaMatch((char16*)inputStr,inputLength);

            if (trigramInfo->isTrigramPattern)
            {
                if (trigramInfo->resultCount > 0)
                {
                    lastSuccessfulMatch.offset=trigramInfo->offsets[trigramInfo->resultCount-1];
                    lastSuccessfulMatch.length=UnifiedRegex::TrigramInfo::PatternLength;
                }
                // else: leave lastMatch undefined

                // Make sure a matcher is allocated and holds valid last match in case the RegExp constructor
                // needs to fill-in details from the last match via JavascriptRegExpConstructor::EnsureValues
                Assert(pattern->rep.unified.program != 0);
                if (pattern->rep.unified.matcher == 0)
                    pattern->rep.unified.matcher = UnifiedRegex::Matcher::New(scriptContext, pattern);
                *pattern->rep.unified.matcher->GroupIdToGroupInfo(0) = lastSuccessfulMatch;

                Assert(pattern->IsGlobal());

                JavascriptArray* arrayResult = CreateMatchResult(stackAllocationPointer, scriptContext, /* isGlobal */ true, pattern->NumGroups(), input);
                FinalizeMatchResult(scriptContext, /* isGlobal */ true, arrayResult, lastSuccessfulMatch);

                if (trigramInfo->resultCount > 0)
                {
                    if (trigramInfo->hasCachedResultString)
                    {
                        for (int k = 0; k < trigramInfo->resultCount; k++)
                        {
                            arrayResult->DirectSetItemAt(k,
                                static_cast<Js::JavascriptString*>(trigramInfo->cachedResult[k]));
                        }
                    }
                    else
                    {
                        for (int k = 0;  k < trigramInfo->resultCount; k++)
                        {
                            JavascriptString * str = SubString::New(input, trigramInfo->offsets[k], UnifiedRegex::TrigramInfo::PatternLength);
                            trigramInfo->cachedResult[k] = str;
                            arrayResult->DirectSetItemAt(k, str);
                        }
                        trigramInfo->hasCachedResultString = true;
                    }
                } // otherwise, there are no results and null will be returned

                if (updateHistory)
                {
                    PropagateLastMatch(scriptContext, /* isGlobal */ true, pattern->IsSticky(), regularExpression, input, lastSuccessfulMatch, lastActualMatch, true, true);
                }

                return lastSuccessfulMatch.IsUndefined() ? scriptContext->GetLibrary()->GetNull() : arrayResult;
            }
        }
#endif

        // If global regex, result array holds substrings for each match, and group bindings are ignored
        // If non-global regex, result array holds overall substring and each group binding substring

        const bool isGlobal = pattern->IsGlobal();
        const bool isSticky = pattern->IsSticky();
        JavascriptArray* arrayResult = 0;
        RegexMatchState state;

        // If global = false and sticky = true, set offset = lastIndex, else set offset = 0
        CharCount offset = 0;
        if (!isGlobal && isSticky)
        {
            offset = regularExpression->GetLastIndex();
        }

        uint32 globalIndex = 0;
        PrimBeginMatch(state, scriptContext, pattern, inputStr, inputLength, false);

        do
        {
            if (offset > inputLength)
            {
                lastActualMatch.Reset();
                break;
            }
            lastActualMatch = PrimMatch(state, scriptContext, pattern, inputLength, offset);

            if (lastActualMatch.IsUndefined())
                break;
            lastSuccessfulMatch = lastActualMatch;
            if (!noResult)
            {
                if (arrayResult == 0)
                    arrayResult = CreateMatchResult(stackAllocationPointer, scriptContext, isGlobal, pattern->NumGroups(), input);
                JavascriptString *const matchedString = SubString::New(input, lastActualMatch.offset, lastActualMatch.length);
                if(isGlobal)
                    arrayResult->DirectSetItemAt(globalIndex, matchedString);
                else
                {
                    // The array's head segment up to length - 1 may not be filled. Write to the head segment element directly
                    // instead of calling a helper that expects the segment to be pre-filled.
                    Assert(globalIndex < arrayResult->GetHead()->length);
                    static_cast<SparseArraySegment<Var> *>(arrayResult->GetHead())->elements[globalIndex] = matchedString;
                }
                globalIndex++;
            }
            offset = lastActualMatch.offset + max(lastActualMatch.length, static_cast<CharCountOrFlag>(1));
        } while (isGlobal);
        PrimEndMatch(state, scriptContext, pattern);
        if(updateHistory)
        {
            PropagateLastMatch(scriptContext, isGlobal, isSticky, regularExpression, input, lastSuccessfulMatch, lastActualMatch, true, true);
        }

        if (arrayResult == 0)
        {
            return scriptContext->GetLibrary()->GetNull();
        }

        const int numGroups = pattern->NumGroups();
        if (!isGlobal)
        {
            if (numGroups > 1)
            {
                // Overall match already captured in index 0 by above, so just grab the groups
                Var nonMatchValue = NonMatchValue(scriptContext, false);
                Field(Var) *elements = ((SparseArraySegment<Var>*)arrayResult->GetHead())->elements;
                for (uint groupId = 1; groupId < (uint)numGroups; groupId++)
                {
                    Assert(groupId < arrayResult->GetHead()->left + arrayResult->GetHead()->length);
                    elements[groupId] = GetGroup(scriptContext, pattern, input, nonMatchValue, groupId);
                }
            }
            FinalizeMatchResult(scriptContext, /* isGlobal */ false, arrayResult, lastSuccessfulMatch);
        }
        else
        {
            FinalizeMatchResult(scriptContext, /* isGlobal */ true, arrayResult, lastSuccessfulMatch);
        }

        return arrayResult;
    }

    // RegExp.prototype.exec (ES5 15.10.6.2)
    Var RegexHelper::RegexExecImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, bool noResult, void *const stackAllocationPointer)
    {
        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Exec, regularExpression, input);
#endif

        const bool isGlobal = pattern->IsGlobal();
        const bool isSticky = pattern->IsSticky();
        CharCount offset;
        CharCount inputLength = input->GetLength();
        if (!GetInitialOffset(isGlobal, isSticky, regularExpression, inputLength, offset))
        {
            return scriptContext->GetLibrary()->GetNull();
        }

        UnifiedRegex::GroupInfo match; // initially undefined
        if (offset <= inputLength)
        {
            const char16* inputStr = input->GetString();
            match = SimpleMatch(scriptContext, pattern, inputStr, inputLength, offset);
        }

        // else: match remains undefined
        PropagateLastMatch(scriptContext, isGlobal, isSticky, regularExpression, input, match, match, true, true);

        if (noResult || match.IsUndefined())
        {
            return scriptContext->GetLibrary()->GetNull();
        }

        const int numGroups = pattern->NumGroups();
        Assert(numGroups >= 0);
        JavascriptArray* result = CreateExecResult(stackAllocationPointer, scriptContext, numGroups, input, match);
        Var nonMatchValue = NonMatchValue(scriptContext, false);
        Field(Var) *elements = ((SparseArraySegment<Var>*)result->GetHead())->elements;
        for (uint groupId = 0; groupId < (uint)numGroups; groupId++)
        {
            Assert(groupId < result->GetHead()->left + result->GetHead()->length);
            elements[groupId] = GetGroup(scriptContext, pattern, input, nonMatchValue, groupId);
        }
        return result;
    }

    Var RegexHelper::RegexTest(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString *input)
    {
        if (scriptContext->GetConfig()->IsES6RegExSymbolsEnabled()
            && IsRegexTestObservable(thisObj, scriptContext))
        {
            return RegexEs6TestImpl(scriptContext, thisObj, input);
        }
        else
        {
            JavascriptRegExp* regularExpression =
                JavascriptRegExp::ToRegExp(thisObj, _u("RegExp.prototype.test"), scriptContext);
            return RegexEs5TestImpl(scriptContext, regularExpression, input);
        }
    }

    bool RegexHelper::IsRegexTestObservable(RecyclableObject* instance, ScriptContext* scriptContext)
    {
        DynamicObject* regexPrototype = scriptContext->GetLibrary()->GetRegExpPrototype();
        return !JavascriptRegExp::HasOriginalRegExType(instance)
            || JavascriptRegExp::HasObservableExec(regexPrototype);
    }

    Var RegexHelper::RegexEs6TestImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString *input)
    {
        Var match = JavascriptRegExp::CallExec(thisObj, input, _u("RegExp.prototype.test"), scriptContext);
        return JavascriptBoolean::ToVar(!JavascriptOperators::IsNull(match), scriptContext);
    }

    // RegExp.prototype.test (ES5 15.10.6.3)
    Var RegexHelper::RegexEs5TestImpl(ScriptContext* scriptContext, JavascriptRegExp *regularExpression, JavascriptString *input)
    {
        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
        const char16* inputStr = input->GetString();
        CharCount inputLength = input->GetLength();
        UnifiedRegex::GroupInfo match; // initially undefined

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Test, regularExpression, input);
#endif
        const bool isGlobal = pattern->IsGlobal();
        const bool isSticky = pattern->IsSticky();
        CharCount offset;
        if (!GetInitialOffset(isGlobal, isSticky, regularExpression, inputLength, offset))
            return scriptContext->GetLibrary()->GetFalse();

        if (offset <= inputLength)
        {
            match = SimpleMatch(scriptContext, pattern, inputStr, inputLength, offset);
        }

        // else: match remains undefined
        PropagateLastMatch(scriptContext, isGlobal, isSticky, regularExpression, input, match, match, true, true);

        return JavascriptBoolean::ToVar(!match.IsUndefined(), scriptContext);
    }

    template<typename GroupFn>
    void RegexHelper::ReplaceFormatString
        ( ScriptContext* scriptContext
        , int numGroups
        , GroupFn getGroup
        , JavascriptString* input
        , const char16* matchedString
        , UnifiedRegex::GroupInfo match
        , JavascriptString* replace
        , int substitutions
        , __in_ecount(substitutions) CharCount* substitutionOffsets
        , CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)>& concatenated )
    {
        Var nonMatchValue = NonMatchValue(scriptContext, false);
        const CharCount inputLength = input->GetLength();
        const char16* replaceStr = replace->GetString();
        const CharCount replaceLength = replace->GetLength();

        CharCount offset = 0;
        for (int i = 0; i < substitutions; i++)
        {
            CharCount substitutionOffset = substitutionOffsets[i];
            concatenated.Append(replace, offset, substitutionOffset - offset);
            char16 currentChar = replaceStr[substitutionOffset + 1];
            if (currentChar >= _u('0') && currentChar <= _u('9'))
            {
                int captureIndex = (int)(currentChar - _u('0'));
                offset = substitutionOffset + 2;

                if (offset < replaceLength)
                {
                    currentChar = replaceStr[substitutionOffset + 2];
                    if (currentChar >= _u('0') && currentChar <= _u('9'))
                    {
                        int tempCaptureIndex = (10 * captureIndex) + (int)(currentChar - _u('0'));
                        if (tempCaptureIndex < numGroups)
                        {
                            captureIndex = tempCaptureIndex;
                            offset = substitutionOffset + 3;
                        }
                    }
                }

                if (captureIndex < numGroups && (captureIndex != 0))
                {
                    Var group = getGroup(captureIndex, nonMatchValue);
                    if (JavascriptString::Is(group))
                        concatenated.Append(JavascriptString::FromVar(group));
                    else if (group != nonMatchValue)
                        concatenated.Append(replace, substitutionOffset, offset - substitutionOffset);
                }
                else
                    concatenated.Append(replace, substitutionOffset, offset - substitutionOffset);
            }
            else
            {
                switch (currentChar)
                {
                case _u('$'): // literal '$' character
                    concatenated.Append(_u('$'));
                    offset = substitutionOffset + 2;
                    break;
                case _u('&'): // matched string
                    concatenated.Append(matchedString, match.length);
                    offset = substitutionOffset + 2;
                    break;
                case _u('`'): // left context
                    concatenated.Append(input, 0, match.offset);
                    offset = substitutionOffset + 2;
                    break;
                case _u('\''): // right context
                    if (match.EndOffset() < inputLength)
                    {
                        concatenated.Append(input, match.EndOffset(), inputLength - match.EndOffset());
                    }
                    offset = substitutionOffset + 2;
                    break;
                default:
                    concatenated.Append(_u('$'));
                    offset = substitutionOffset + 1;
                    break;
                }
            }
        }
        concatenated.Append(replace, offset, replaceLength - offset);
    }

    int RegexHelper::GetReplaceSubstitutions(const char16 * const replaceStr, CharCount const replaceLength,
        ArenaAllocator * const tempAllocator, CharCount** const substitutionOffsetsOut)
    {
        int substitutions = 0;

        for (CharCount i = 0; i < replaceLength; i++)
        {
            if (replaceStr[i] == _u('$'))
            {
                if (++i < replaceLength)
                {
                    substitutions++;
                }
            }
        }

        if (substitutions > 0)
        {
            CharCount* substitutionOffsets = AnewArray(tempAllocator, CharCount, substitutions);
            substitutions = 0;
            for (CharCount i = 0; i < replaceLength; i++)
            {
                if (replaceStr[i] == _u('$'))
                {
                    if (i < (replaceLength - 1))
                    {
#pragma prefast(suppress:26000, "index doesn't overflow the buffer")
                        substitutionOffsets[substitutions] = i;
                        i++;
                        substitutions++;
                    }
                }
            }
            *substitutionOffsetsOut = substitutionOffsets;
        }

        return substitutions;
    }

    Var RegexHelper::RegexReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptString* replace, bool noResult)
    {
        ScriptConfiguration const * scriptConfig = scriptContext->GetConfig();

        if (scriptConfig->IsES6RegExSymbolsEnabled() && IsRegexSymbolReplaceObservable(thisObj, scriptContext))
        {
            return RegexEs6ReplaceImpl(scriptContext, thisObj, input, replace, noResult);
        }
        else
        {
            PCWSTR varName = scriptConfig->IsES6RegExSymbolsEnabled()
                ? _u("RegExp.prototype[Symbol.replace]")
                : _u("String.prototype.replace");
            JavascriptRegExp* regularExpression = JavascriptRegExp::ToRegExp(thisObj, varName, scriptContext);
            return RegexEs5ReplaceImpl(scriptContext, regularExpression, input, replace, noResult);
        }
    }

    bool RegexHelper::IsRegexSymbolReplaceObservable(RecyclableObject* instance, ScriptContext* scriptContext)
    {
        DynamicObject* regexPrototype = scriptContext->GetLibrary()->GetRegExpPrototype();
        return !JavascriptRegExp::HasOriginalRegExType(instance)
            || JavascriptRegExp::HasObservableUnicodeFlag(regexPrototype)
            || JavascriptRegExp::HasObservableExec(regexPrototype)
            || JavascriptRegExp::HasObservableGlobalFlag(regexPrototype);
    }

    Var RegexHelper::RegexEs6ReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptString* replace, bool noResult)
    {
        auto appendReplacement = [&](
            CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)>& resultBuilder,
            ArenaAllocator* tempAlloc,
            JavascriptString* matchStr,
            int numberOfCaptures,
            Var* captures,
            CharCount position)
        {
            CharCount* substitutionOffsets = nullptr;
            int substitutions = GetReplaceSubstitutions(
                replace->GetString(),
                replace->GetLength(),
                tempAlloc,
                &substitutionOffsets);
            auto getGroup = [&](int captureIndex, Var nonMatchValue) {
                return captureIndex <= numberOfCaptures ? captures[captureIndex] : nonMatchValue;
            };
            UnifiedRegex::GroupInfo match(position, matchStr->GetLength());
            int numGroups = numberOfCaptures + 1; // Take group 0 into account.
            ReplaceFormatString(
                scriptContext,
                numGroups,
                getGroup,
                input,
                matchStr->GetString(),
                match,
                replace,
                substitutions,
                substitutionOffsets,
                resultBuilder);
        };
        return RegexEs6ReplaceImpl(scriptContext, thisObj, input, appendReplacement, noResult);
    }

    Var RegexHelper::RegexEs6ReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptFunction* replaceFn)
    {
        auto appendReplacement = [&](
            CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)>& resultBuilder,
            ArenaAllocator* tempAlloc,
            JavascriptString* matchStr,
            int numberOfCaptures,
            Var* captures,
            CharCount position)
        {
            // replaceFn Arguments:
            //
            // 0: this
            // 1: matched
            // 2: capture1
            // ...
            // N + 1: capture N
            // N + 2: position
            // N + 3: input

            // Number of captures can be at most 99, so we won't overflow.
            ushort argCount = (ushort) numberOfCaptures + 4;

            PROBE_STACK_NO_DISPOSE(scriptContext, argCount * sizeof(Var));
            Var* args = (Var*) _alloca(argCount * sizeof(Var));

            args[0] = scriptContext->GetLibrary()->GetUndefined();
#pragma prefast(suppress:6386, "The write is within the bounds")
            args[1] = matchStr;
            for (int i = 1; i <= numberOfCaptures; ++i)
            {
                args[i + 1] = captures[i];
            }
            args[numberOfCaptures + 2] = JavascriptNumber::ToVar(position, scriptContext);
            args[numberOfCaptures + 3] = input;

            JavascriptString* replace = JavascriptConversion::ToString(
                replaceFn->CallFunction(Arguments(CallInfo(argCount), args)),
                scriptContext);

            resultBuilder.Append(replace);
        };
        return RegexEs6ReplaceImpl(scriptContext, thisObj, input, appendReplacement, /* noResult */ false);
    }

    template<typename ReplacementFn>
    Var RegexHelper::RegexEs6ReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, ReplacementFn appendReplacement, bool noResult)
    {
        bool global = JavascriptRegExp::GetGlobalProperty(thisObj, scriptContext);
        bool unicode = false; // Dummy value. It isn't used below unless "global" is "true".
        if (global)
        {
            unicode = JavascriptRegExp::GetUnicodeProperty(thisObj, scriptContext);
            JavascriptRegExp::SetLastIndexProperty(thisObj, TaggedInt::ToVarUnchecked(0), scriptContext);
        }

        JavascriptString* accumulatedResult = nullptr;

        Recycler* recycler = scriptContext->GetRecycler();

        JsUtil::List<RecyclableObject*>* results = RecyclerNew(recycler, JsUtil::List<RecyclableObject*>, recycler);

        while (true)
        {
            PCWSTR varName = _u("RegExp.prototype[Symbol.replace]");
            Var result = JavascriptRegExp::CallExec(thisObj, input, varName, scriptContext);
            if (JavascriptOperators::IsNull(result))
            {
                break;
            }

            RecyclableObject* resultObj = ExecResultToRecyclableObject(result);

            results->Add(resultObj);

            if (!global)
            {
                break;
            }

            JavascriptString* matchStr = GetMatchStrFromResult(resultObj, scriptContext);
            AdvanceLastIndex(thisObj, input, matchStr, unicode, scriptContext);
        }

        CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)> accumulatedResultBuilder(scriptContext);
        CharCount inputLength = input->GetLength();
        CharCount nextSourcePosition = 0;

        size_t previousNumberOfCapturesToKeep = 0;
        Field(Var)* captures = nullptr;

        BEGIN_TEMP_ALLOCATOR(tempAlloc, scriptContext, _u("RegexHelper"))
        {
            results->Map([&](int resultIndex, RecyclableObject* resultObj) {
                int64 length = JavascriptConversion::ToLength(
                    JavascriptOperators::GetProperty(resultObj, PropertyIds::length, scriptContext),
                    scriptContext);
                uint64 numberOfCaptures = (uint64) max(length - 1, (int64) 0);

                JavascriptString* matchStr = GetMatchStrFromResult(resultObj, scriptContext);

                int64 index = JavascriptConversion::ToLength(
                    JavascriptOperators::GetProperty(resultObj, PropertyIds::index, scriptContext),
                    scriptContext);
                CharCount position = max(
                    min(JavascriptRegExp::GetIndexOrMax(index), inputLength),
                    (CharCount) 0);

                // Capture groups can be referenced using at most two digits.
                const uint64 maxNumberOfCaptures = 99;
                size_t numberOfCapturesToKeep = (size_t) min(numberOfCaptures, maxNumberOfCaptures);
                if (captures == nullptr)
                {
                    captures = RecyclerNewArray(recycler, Field(Var), numberOfCapturesToKeep + 1);
                }
                else if (numberOfCapturesToKeep != previousNumberOfCapturesToKeep)
                {
                    size_t existingBytes = (previousNumberOfCapturesToKeep + 1) * sizeof(Var*);
                    size_t requestedBytes = (numberOfCapturesToKeep + 1) * sizeof(Var*);
                    captures = (Field(Var)*) recycler->Realloc(captures, existingBytes, requestedBytes);
                }
                previousNumberOfCapturesToKeep = numberOfCapturesToKeep;

                for (uint64 i = 1; i <= numberOfCaptures; ++i)
                {
                    Var nextCapture = JavascriptOperators::GetItem(resultObj, i, scriptContext);
                    if (!JavascriptOperators::IsUndefined(nextCapture))
                    {
                        nextCapture = JavascriptConversion::ToString(nextCapture, scriptContext);
                    }

                    if (i <= numberOfCapturesToKeep)
                    {
                        captures[i] = nextCapture;
                    }
                }

                if (position >= nextSourcePosition)
                {
                    CharCount substringLength = position - nextSourcePosition;
                    accumulatedResultBuilder.Append(input, nextSourcePosition, substringLength);

                    appendReplacement(accumulatedResultBuilder, tempAlloc, matchStr, (int) numberOfCapturesToKeep, (Var*)captures, position);

                    nextSourcePosition = JavascriptRegExp::AddIndex(position, matchStr->GetLength());
                }
            });
        }
        END_TEMP_ALLOCATOR(tempAlloc, scriptContext);

        if (nextSourcePosition < inputLength)
        {
            CharCount substringLength = inputLength - nextSourcePosition;
            accumulatedResultBuilder.Append(input, nextSourcePosition, substringLength);
        }

        accumulatedResult =  accumulatedResultBuilder.ToString();

        Assert(accumulatedResult != nullptr);
        return accumulatedResult;
    }

    // String.prototype.replace, replace value has been converted to a string (ES5 15.5.4.11)
    Var RegexHelper::RegexEs5ReplaceImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace, bool noResult)
    {
        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
        const char16* replaceStr = replace->GetString();
        CharCount replaceLength = replace->GetLength();
        const char16* inputStr = input->GetString();
        CharCount inputLength = input->GetLength();

        JavascriptString* newString = nullptr;

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Replace, regularExpression, input, replace);
#endif

        RegexMatchState state;
        PrimBeginMatch(state, scriptContext, pattern, inputStr, inputLength, true);

        UnifiedRegex::GroupInfo lastActualMatch;
        UnifiedRegex::GroupInfo lastSuccessfulMatch;
        const bool isGlobal = pattern->IsGlobal();
        const bool isSticky = pattern->IsSticky();

        // If global = false and sticky = true, set offset = lastIndex, else set offset = 0
        CharCount offset = 0;
        if (!isGlobal && isSticky)
        {
            offset = regularExpression->GetLastIndex();
        }

        if (!noResult)
        {
            CharCount* substitutionOffsets = nullptr;
            int substitutions = GetReplaceSubstitutions(replaceStr, replaceLength,
                 state.tempAllocatorObj->GetAllocator(), &substitutionOffsets);

            // Use to see if we already have partial result populated in concatenated
            CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)> concatenated(scriptContext);

            // If lastIndex > 0, append input[0..offset] characters to the result
            if (offset > 0)
            {
                concatenated.Append(input, 0, min(offset, inputLength));
            }

            do
            {
                if (offset > inputLength)
                {
                    lastActualMatch.Reset();
                    break;
                }

                lastActualMatch = PrimMatch(state, scriptContext, pattern, inputLength, offset);

                if (lastActualMatch.IsUndefined())
                    break;

                lastSuccessfulMatch = lastActualMatch;
                concatenated.Append(input, offset, lastActualMatch.offset - offset);
                if (substitutionOffsets != 0)
                {
                    auto getGroup = [&](int captureIndex, Var nonMatchValue) {
                        return GetGroup(scriptContext, pattern, input, nonMatchValue, captureIndex);
                    };
                    const char16* matchedString = inputStr + lastActualMatch.offset;
                    ReplaceFormatString(scriptContext, pattern->NumGroups(), getGroup, input, matchedString, lastActualMatch, replace, substitutions, substitutionOffsets, concatenated);
                }
                else
                {
                    concatenated.Append(replace);
                }
                if (lastActualMatch.length == 0)
                {
                    if (lastActualMatch.offset < inputLength)
                    {
                        concatenated.Append(inputStr[lastActualMatch.offset]);
                    }
                    offset = lastActualMatch.offset + 1;
                }
                else
                {
                    offset = lastActualMatch.EndOffset();
                }
            }
            while (isGlobal);

            if (offset == 0)
            {
                // There was no successful match so the result is the input string.
                newString = input;
            }
            else
            {
                if (offset < inputLength)
                {
                    concatenated.Append(input, offset, inputLength - offset);
                }
                newString = concatenated.ToString();
            }
            substitutionOffsets = 0;
        }
        else
        {
            do
            {
                if (offset > inputLength)
                {
                    lastActualMatch.Reset();
                    break;
                }
                lastActualMatch = PrimMatch(state, scriptContext, pattern, inputLength, offset);

                if (lastActualMatch.IsUndefined())
                    break;

                lastSuccessfulMatch = lastActualMatch;
                offset = lastActualMatch.length == 0? lastActualMatch.offset + 1 : lastActualMatch.EndOffset();
            }
            while (isGlobal);
            newString = scriptContext->GetLibrary()->GetEmptyString();
        }

        PrimEndMatch(state, scriptContext, pattern);
        PropagateLastMatch(scriptContext, isGlobal, isSticky, regularExpression, input, lastSuccessfulMatch, lastActualMatch, true, true);
        return newString;
    }

    Var RegexHelper::RegexReplaceImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptFunction* replacefn)
    {
        ScriptConfiguration const * scriptConfig = scriptContext->GetConfig();

        if (scriptConfig->IsES6RegExSymbolsEnabled() && IsRegexSymbolReplaceObservable(thisObj, scriptContext))
        {
            return RegexEs6ReplaceImpl(scriptContext, thisObj, input, replacefn);
        }
        else
        {
            PCWSTR varName = scriptConfig->IsES6RegExSymbolsEnabled()
                ? _u("RegExp.prototype[Symbol.replace]")
                : _u("String.prototype.replace");
            JavascriptRegExp* regularExpression = JavascriptRegExp::ToRegExp(thisObj, varName, scriptContext);
            return RegexEs5ReplaceImpl(scriptContext, regularExpression, input, replacefn);
        }
    }

    // String.prototype.replace, replace value is a function (ES5 15.5.4.11)
    Var RegexHelper::RegexEs5ReplaceImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptFunction* replacefn)
    {
        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
        const char16* inputStr = input->GetString();
        CharCount inputLength = input->GetLength();
        JavascriptString* newString = nullptr;
        const int numGroups = pattern->NumGroups();
        Var nonMatchValue = NonMatchValue(scriptContext, false);
        UnifiedRegex::GroupInfo lastMatch; // initially undefined

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Replace, regularExpression, input, scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("<replace function>")));
#endif

        RegexMatchState state;
        PrimBeginMatch(state, scriptContext, pattern, inputStr, inputLength, false);

        // NOTE: These must be kept out of the scope of the try below!
        const bool isGlobal = pattern->IsGlobal();
        const bool isSticky = pattern->IsSticky();

        // If global = true, set lastIndex to 0 in case it is used in replacefn
        if (isGlobal)
        {
            regularExpression->SetLastIndex(0);
        }

        // If global = false and sticky = true, set offset = lastIndex, else set offset = 0
        CharCount offset = 0;
        if (!isGlobal && isSticky)
        {
            offset = regularExpression->GetLastIndex();
        }

        CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)> concatenated(scriptContext);
        UnifiedRegex::GroupInfo lastActualMatch;
        UnifiedRegex::GroupInfo lastSuccessfulMatch;

        // Replace function must be called with arguments (<function's this>, group0, ..., groupn, offset, input)
        // The garbage collector must know about this array since it is being passed back into script land
        Var* replaceArgs;
        PROBE_STACK_NO_DISPOSE(scriptContext, (numGroups + 3) * sizeof(Var));
        replaceArgs = (Var*)_alloca((numGroups + 3) * sizeof(Var));
        replaceArgs[0] = scriptContext->GetLibrary()->GetUndefined();
        replaceArgs[numGroups + 2] = input;

        if (offset > 0)
        {
            concatenated.Append(input, 0, min(offset, inputLength));
        }

        do
        {
            if (offset > inputLength)
            {
                lastActualMatch.Reset();
                break;
            }

            lastActualMatch = PrimMatch(state, scriptContext, pattern, inputLength, offset);

            if (lastActualMatch.IsUndefined())
                break;

            lastSuccessfulMatch = lastActualMatch;
            for (int groupId = 0;  groupId < numGroups; groupId++)
                replaceArgs[groupId + 1] = GetGroup(scriptContext, pattern, input, nonMatchValue, groupId);
#pragma prefast(suppress:6386, "The write index numGroups + 1 is in the bound")
            replaceArgs[numGroups + 1] = JavascriptNumber::ToVar(lastActualMatch.offset, scriptContext);

            // The called function must see the global state updated by the current match
            // (Should the function reach into a RegExp field, the pattern will still be valid, thus there's no
            //  danger of the primitive regex matcher being re-entered)

            // WARNING: We go off into script land here, which way in turn invoke a regex operation, even on the
            //          same regex.
            JavascriptString* replace = JavascriptConversion::ToString(replacefn->CallFunction(Arguments(CallInfo((ushort)(numGroups + 3)), replaceArgs)), scriptContext);
            concatenated.Append(input, offset, lastActualMatch.offset - offset);
            concatenated.Append(replace);
            if (lastActualMatch.length == 0)
            {
                if (lastActualMatch.offset < inputLength)
                {
                    concatenated.Append(inputStr[lastActualMatch.offset]);
                }
                offset = lastActualMatch.offset + 1;
            }
            else
            {
                offset = lastActualMatch.EndOffset();
            }
        }
        while (isGlobal);

        PrimEndMatch(state, scriptContext, pattern);

        if (offset == 0)
        {
            // There was no successful match so the result is the input string.
            newString = input;
        }
        else
        {
            if (offset < inputLength)
            {
                concatenated.Append(input, offset, inputLength - offset);
            }
            newString = concatenated.ToString();
        }

        PropagateLastMatch(scriptContext, isGlobal, isSticky, regularExpression, input, lastSuccessfulMatch, lastActualMatch, true, true);
        return newString;
    }

    Var RegexHelper::StringReplace(JavascriptString* match, JavascriptString* input, JavascriptString* replace)
    {
        CharCount matchedIndex = JavascriptString::strstr(input, match, true);
        if (matchedIndex == CharCountFlag)
        {
            return input;
        }

        const char16 *const replaceStr = replace->GetString();

        // Unfortunately, due to the possibility of there being $ escapes, we can't just wmemcpy the replace string. Check if we
        // have a small replace string that we can quickly scan for '$', to see if we can just wmemcpy.
        bool definitelyNoEscapes = replace->GetLength() == 0;
        if(!definitelyNoEscapes && replace->GetLength() <= 8)
        {
            CharCount i = 0;
            for(; i < replace->GetLength() && replaceStr[i] != _u('$'); ++i);
            definitelyNoEscapes = i >= replace->GetLength();
        }

        if(definitelyNoEscapes)
        {
            const char16* inputStr = input->GetString();
            const char16* prefixStr = inputStr;
            CharCount prefixLength = (CharCount)matchedIndex;
            const char16* postfixStr = inputStr + prefixLength + match->GetLength();
            CharCount postfixLength = input->GetLength() - prefixLength - match->GetLength();
            CharCount newLength = prefixLength + postfixLength + replace->GetLength();
            BufferStringBuilder bufferString(newLength, match->GetScriptContext());
            bufferString.SetContent(prefixStr, prefixLength,
                                    replaceStr, replace->GetLength(),
                                    postfixStr, postfixLength);
            return bufferString.ToString();
        }

        CompoundString::Builder<64 * sizeof(void *) / sizeof(char16)> concatenated(input->GetScriptContext());

        // Copy portion of input string that precedes the matched substring
        concatenated.Append(input, 0, matchedIndex);

        // Copy the replace string with substitutions
        CharCount i = 0, j = 0;
        for(; j < replace->GetLength(); ++j)
        {
            if(replaceStr[j] == _u('$') && j + 1 < replace->GetLength())
            {
                switch(replaceStr[j + 1])
                {
                    case _u('$'): // literal '$'
                        ++j;
                        concatenated.Append(replace, i, j - i);
                        i = j + 1;
                        break;

                    case _u('&'): // matched substring
                        concatenated.Append(replace, i, j - i);
                        concatenated.Append(match);
                        ++j;
                        i = j + 1;
                        break;

                    case _u('`'): // portion of input string that precedes the matched substring
                        concatenated.Append(replace, i, j - i);
                        concatenated.Append(input, 0, matchedIndex);
                        ++j;
                        i = j + 1;
                        break;

                    case _u('\''): // portion of input string that follows the matched substring
                        concatenated.Append(replace, i, j - i);
                        concatenated.Append(
                            input,
                            matchedIndex + match->GetLength(),
                            input->GetLength() - matchedIndex - match->GetLength());
                        ++j;
                        i = j + 1;
                        break;

                    default: // take both the initial '$' and the following character literally
                        ++j;
                }
            }
        }
        Assert(i <= j);
        concatenated.Append(replace, i, j - i);

        // Copy portion of input string that follows the matched substring
        concatenated.Append(input, matchedIndex + match->GetLength(), input->GetLength() - matchedIndex - match->GetLength());

        return concatenated.ToString();
    }

    Var RegexHelper::StringReplace(ScriptContext* scriptContext, JavascriptString* match, JavascriptString* input, JavascriptFunction* replacefn)
    {
        CharCount indexMatched = JavascriptString::strstr(input, match, true);
        Assert(match->GetScriptContext() == scriptContext);
        Assert(input->GetScriptContext() == scriptContext);

        if (indexMatched != CharCountFlag)
        {
            Var pThis = scriptContext->GetLibrary()->GetUndefined();
            Var replaceVar = CALL_FUNCTION(scriptContext->GetThreadContext(), replacefn, CallInfo(4), pThis, match, JavascriptNumber::ToVar((int)indexMatched, scriptContext), input);
            JavascriptString* replace = JavascriptConversion::ToString(replaceVar, scriptContext);
            const char16* inputStr = input->GetString();
            const char16* prefixStr = inputStr;
            CharCount prefixLength = indexMatched;
            const char16* postfixStr = inputStr + prefixLength + match->GetLength();
            CharCount postfixLength = input->GetLength() - prefixLength - match->GetLength();
            CharCount newLength = prefixLength + postfixLength + replace->GetLength();
            BufferStringBuilder bufferString(newLength, match->GetScriptContext());
            bufferString.SetContent(prefixStr, prefixLength,
                                    replace->GetString(), replace->GetLength(),
                                    postfixStr, postfixLength);
            return bufferString.ToString();
        }

        return input;
    }

    void RegexHelper::AppendSubString(ScriptContext* scriptContext, JavascriptArray* ary, JavascriptString* input, CharCount startInclusive, CharCount endExclusive)
    {
        Assert(endExclusive >= startInclusive);
        Assert(endExclusive <= input->GetLength());
        CharCount length = endExclusive - startInclusive;
        JavascriptString* subString;
        if (length == 0)
        {
            subString = scriptContext->GetLibrary()->GetEmptyString();
        }
        else if (length == 1)
        {
            subString = scriptContext->GetLibrary()->GetCharStringCache().GetStringForChar(input->GetString()[startInclusive]);
        }
        else
        {
            subString = SubString::New(input, startInclusive, length);
        }
        ary->DirectAppendItem(subString);
    }

    inline UnifiedRegex::RegexPattern *RegexHelper::GetSplitPattern(ScriptContext* scriptContext, JavascriptRegExp *regularExpression)
    {
        UnifiedRegex::RegexPattern* splitPattern = regularExpression->GetSplitPattern();
        if (!splitPattern)
        {
            UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
            bool isSticky = (pattern->GetFlags() & UnifiedRegex::StickyRegexFlag) != 0;
            if (!isSticky)
            {
                splitPattern = pattern;
            }
            else
            {
                // When the sticky flag is present, the pattern will match the input only at
                // the beginning since "lastIndex" is set to 0 before the first iteration.
                // However, for split(), we need to look for the pattern anywhere in the input.
                //
                // One way to handle this is to use the original pattern with the sticky flag and
                // when it fails, move to the next character and retry.
                //
                // Another way, which is implemented here, is to create another pattern without the
                // sticky flag and have it automatically look for itself anywhere in the input. This
                // way, we can also take advantage of the optimizations for the global search (e.g.,
                // the Boyer-Moore string search).

                InternalString source = pattern->GetSource();
                UnifiedRegex::RegexFlags nonStickyFlags =
                    static_cast<UnifiedRegex::RegexFlags>(pattern->GetFlags() & ~UnifiedRegex::StickyRegexFlag);
                splitPattern = CompileDynamic(
                    scriptContext,
                    source.GetBuffer(),
                    source.GetLength(),
                    nonStickyFlags,
                    pattern->IsLiteral());
            }
            regularExpression->SetSplitPattern(splitPattern);
        }

        return splitPattern;
    }

    Var RegexHelper::RegexSplitImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer)
    {
        ScriptConfiguration const * scriptConfig = scriptContext->GetConfig();

        if (scriptConfig->IsES6RegExSymbolsEnabled()
            && IsRegexSymbolSplitObservable(thisObj, scriptContext))
        {
            return RegexEs6SplitImpl(scriptContext, thisObj, input, limit, noResult, stackAllocationPointer);
        }
        else
        {
            PCWSTR varName = scriptContext->GetConfig()->IsES6RegExSymbolsEnabled()
                ? _u("RegExp.prototype[Symbol.split]")
                : _u("String.prototype.split");
            JavascriptRegExp* regularExpression = JavascriptRegExp::ToRegExp(thisObj, varName, scriptContext);
            return RegexEs5SplitImpl(scriptContext, regularExpression, input, limit, noResult, stackAllocationPointer);
        }
    }

    bool RegexHelper::IsRegexSymbolSplitObservable(RecyclableObject* instance, ScriptContext* scriptContext)
    {
        DynamicObject* regexPrototype = scriptContext->GetLibrary()->GetRegExpPrototype();
        return !JavascriptRegExp::HasOriginalRegExType(instance)
            || JavascriptRegExp::HasObservableConstructor(regexPrototype)
            || JavascriptRegExp::HasObservableFlags(regexPrototype)
            || JavascriptRegExp::HasObservableExec(regexPrototype);
    }

    Var RegexHelper::RegexEs6SplitImpl(ScriptContext* scriptContext, RecyclableObject* thisObj, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer)
    {
        PCWSTR const varName = _u("RegExp.prototype[Symbol.split]");

        Var speciesConstructor = JavascriptOperators::SpeciesConstructor(
            thisObj,
            scriptContext->GetLibrary()->GetRegExpConstructor(),
            scriptContext);

        JavascriptString* flags = JavascriptConversion::ToString(
            JavascriptOperators::GetProperty(thisObj, PropertyIds::flags, scriptContext),
            scriptContext);
        bool unicode = wcsstr(flags->GetString(), _u("u")) != nullptr;
        flags = AppendStickyToFlagsIfNeeded(flags, scriptContext);

        Js::Var args[] = { speciesConstructor, thisObj, flags };
        Js::CallInfo callInfo(Js::CallFlags_New, _countof(args));
        Var regEx = JavascriptOperators::NewScObject(
            speciesConstructor,
            Js::Arguments(callInfo, args),
            scriptContext);
        RecyclableObject* splitter = RecyclableObject::FromVar(regEx);

        JavascriptArray* arrayResult = scriptContext->GetLibrary()->CreateArray();

        if (limit == 0)
        {
            return arrayResult;
        }

        CharCount inputLength = input->GetLength();
        if (inputLength == 0)
        {
            Var result = JavascriptRegExp::CallExec(splitter, input, varName, scriptContext);
            if (!JavascriptOperators::IsNull(result))
            {
                return arrayResult;
            }

            arrayResult->DirectAppendItem(input);
            return arrayResult;
        }

        CharCount substringStartIndex = 0; // 'p' in spec
        CharCount substringEndIndex = substringStartIndex; // 'q' in spec
        do // inputLength > 0
        {
            JavascriptRegExp::SetLastIndexProperty(splitter, substringEndIndex, scriptContext);
            Var result = JavascriptRegExp::CallExec(splitter, input, varName, scriptContext); // 'z' in spec
            if (JavascriptOperators::IsNull(result))
            {
                substringEndIndex = AdvanceStringIndex(input, substringEndIndex, unicode);
            }
            else
            {
                CharCount endIndex = JavascriptRegExp::GetLastIndexProperty(splitter, scriptContext); // 'e' in spec
                endIndex = min(endIndex, inputLength);
                if (endIndex == substringStartIndex)
                {
                    substringEndIndex = AdvanceStringIndex(input, substringEndIndex, unicode);
                }
                else
                {
                    AppendSubString(scriptContext, arrayResult, input, substringStartIndex, substringEndIndex);
                    if (arrayResult->GetLength() == limit)
                    {
                        return arrayResult;
                    }

                    substringStartIndex = endIndex;

                    RecyclableObject* resultObject = ExecResultToRecyclableObject(result);

                    int64 length = JavascriptConversion::ToLength(
                        JavascriptOperators::GetProperty(resultObject, PropertyIds::length, scriptContext),
                        scriptContext);
                    uint64 numberOfCaptures = max(length - 1, (int64) 0);
                    for (uint64 i = 1; i <= numberOfCaptures; ++i)
                    {
                        Var nextCapture = JavascriptOperators::GetItem(resultObject, i, scriptContext);
                        arrayResult->DirectAppendItem(nextCapture);

                        if (arrayResult->GetLength() == limit)
                        {
                            return arrayResult;
                        }
                    }

                    substringEndIndex = substringStartIndex;
                }
            }
        }
        while (substringEndIndex < inputLength);

        AppendSubString(scriptContext, arrayResult, input, substringStartIndex, substringEndIndex);

        return arrayResult;
    }

    JavascriptString* RegexHelper::AppendStickyToFlagsIfNeeded(JavascriptString* flags, ScriptContext* scriptContext)
    {
        const char16* flagsString = flags->GetString();
        if (wcsstr(flagsString, _u("y")) == nullptr)
        {
            BEGIN_TEMP_ALLOCATOR(tempAlloc, scriptContext, _u("RegexHelper"))
            {
                StringBuilder<ArenaAllocator> bs(tempAlloc, flags->GetLength() + 1);
                bs.Append(flagsString, flags->GetLength());
                bs.Append(_u('y'));
                flags = Js::JavascriptString::NewCopyBuffer(bs.Detach(), bs.Count(), scriptContext);
            }
            END_TEMP_ALLOCATOR(tempAlloc, scriptContext);
        }

        return flags;
    }

    // String.prototype.split (ES5 15.5.4.14)
    Var RegexHelper::RegexEs5SplitImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer)
    {
        if (noResult && scriptContext->GetConfig()->SkipSplitOnNoResult())
        {
            // TODO: Fix this so that the side effect for PropagateLastMatch is done
            return scriptContext->GetLibrary()->GetNull();
        }

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Split, regularExpression, input);
#endif

        JavascriptArray* ary = scriptContext->GetLibrary()->CreateArrayOnStack(stackAllocationPointer);

        if (limit == 0)
        {
            // SPECIAL CASE: Zero limit
            return ary;
        }

        UnifiedRegex::RegexPattern *splitPattern = GetSplitPattern(scriptContext, regularExpression);

        const char16* inputStr = input->GetString();
        CharCount inputLength = input->GetLength(); // s in spec
        const int numGroups = splitPattern->NumGroups();
        Var nonMatchValue = NonMatchValue(scriptContext, false);
        UnifiedRegex::GroupInfo lastSuccessfulMatch; // initially undefined

        RegexMatchState state;
        PrimBeginMatch(state, scriptContext, splitPattern, inputStr, inputLength, false);

        if (inputLength == 0)
        {
            // SPECIAL CASE: Empty string
            UnifiedRegex::GroupInfo match = PrimMatch(state, scriptContext, splitPattern, inputLength, 0);
            if (match.IsUndefined())
                ary->DirectAppendItem(input);
            else
                lastSuccessfulMatch = match;
        }
        else
        {
            CharCount copyOffset = 0;  // p in spec
            CharCount startOffset = 0; // q in spec

            CharCount inputLimit = inputLength;

            while (startOffset < inputLimit)
            {
                UnifiedRegex::GroupInfo match = PrimMatch(state, scriptContext, splitPattern, inputLength, startOffset);

                if (match.IsUndefined())
                    break;

                lastSuccessfulMatch = match;

                if (match.offset >= inputLimit)
                    break;

                startOffset = match.offset;
                CharCount endOffset = match.EndOffset(); // e in spec

                if (endOffset == copyOffset)
                    startOffset++;
                else
                {
                    AppendSubString(scriptContext, ary, input, copyOffset, startOffset);
                    if (ary->GetLength() >= limit)
                        break;

                    startOffset = copyOffset = endOffset;

                    for (int groupId = 1; groupId < numGroups; groupId++)
                    {
                        ary->DirectAppendItem(GetGroup(scriptContext, splitPattern, input, nonMatchValue, groupId));
                        if (ary->GetLength() >= limit)
                            break;
                    }
                }
            }

            if (ary->GetLength() < limit)
                AppendSubString(scriptContext, ary, input, copyOffset, inputLength);
        }

        PrimEndMatch(state, scriptContext, splitPattern);
        Assert(!splitPattern->IsSticky());
        PropagateLastMatch
            ( scriptContext
            , splitPattern->IsGlobal()
            , /* isSticky */ false
            , regularExpression
            , input
            , lastSuccessfulMatch
            , UnifiedRegex::GroupInfo()
            , /* updateRegex */ true
            , /* updateCtor */ true
            , /* useSplitPattern */ true );

        return ary;
    }

    UnifiedRegex::GroupInfo
    RegexHelper::SimpleMatch(ScriptContext * scriptContext, UnifiedRegex::RegexPattern * pattern, const char16 * input,  CharCount inputLength, CharCount offset)
    {
        RegexMatchState state;
        PrimBeginMatch(state, scriptContext, pattern, input, inputLength, false);
        UnifiedRegex::GroupInfo match = PrimMatch(state, scriptContext, pattern, inputLength, offset);
        PrimEndMatch(state, scriptContext, pattern);
        return match;
    }

    // String.prototype.search (ES5 15.5.4.12)
    Var RegexHelper::RegexSearchImpl(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
        const char16* inputStr = input->GetString();
        CharCount inputLength = input->GetLength();

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Search, regularExpression, input);
#endif
        UnifiedRegex::GroupInfo match = RegexHelper::SimpleMatch(scriptContext, pattern, inputStr, inputLength, 0);

        PropagateLastMatch(scriptContext, pattern->IsGlobal(), pattern->IsSticky(), regularExpression, input, match, match, false, true);

        return JavascriptNumber::ToVar(match.IsUndefined() ? -1 : (int32)match.offset, scriptContext);
    }

    // String.prototype.split (ES5 15.5.4.14)
    Var RegexHelper::StringSplit(JavascriptString* match, JavascriptString* input, CharCount limit)
    {
        ScriptContext* scriptContext = match->GetScriptContext();
        JavascriptArray* ary;
        CharCount matchLen = match->GetLength();
        if (matchLen == 0)
        {
            CharCount count = min(input->GetLength(), limit);
            ary = scriptContext->GetLibrary()->CreateArray(count);
            const char16 * charString = input->GetString();
            for (CharCount i = 0; i < count; i++)
            {
                ary->DirectSetItemAt(i, scriptContext->GetLibrary()->GetCharStringCache().GetStringForChar(charString[i]));
            }
        }
        else
        {
            CharCount i = 0;
            CharCount offset = 0;
            ary = scriptContext->GetLibrary()->CreateArray(0);
            while (i < limit)
            {
                CharCount prevOffset = offset;
                offset = JavascriptString::strstr(input, match, false, prevOffset);
                if (offset != CharCountFlag)
                {
                    ary->DirectSetItemAt(i++, SubString::New(input, prevOffset, offset-prevOffset));
                    offset += max(matchLen, static_cast<CharCount>(1));
                    if (offset > input->GetLength())
                        break;
                }
                else
                {
                    ary->DirectSetItemAt(i++, SubString::New(input, prevOffset, input->GetLength() - prevOffset));
                    break;
                }
            }
        }
        return ary;
    }

    bool RegexHelper::IsResultNotUsed(CallFlags flags)
    {
        return !PHASE_OFF1(Js::RegexResultNotUsedPhase) && ((flags & CallFlags_NotUsed) != 0);
    }

    // ----------------------------------------------------------------------
    // Primitives
    // ----------------------------------------------------------------------
    void RegexHelper::PrimBeginMatch(RegexMatchState& state, ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern, const char16* input, CharCount inputLength, bool alwaysNeedAlloc)
    {
        state.input = input;
        if (pattern->rep.unified.matcher == 0)
            pattern->rep.unified.matcher = UnifiedRegex::Matcher::New(scriptContext, pattern);
        if (alwaysNeedAlloc)
            state.tempAllocatorObj = scriptContext->GetTemporaryAllocator(_u("RegexUnifiedExecTemp"));
        else
            state.tempAllocatorObj = 0;
    }

    UnifiedRegex::GroupInfo
    RegexHelper::PrimMatch(RegexMatchState& state, ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern, CharCount inputLength, CharCount offset)
    {
        Assert(pattern->rep.unified.program != 0);
        Assert(pattern->rep.unified.matcher != 0);
#if ENABLE_REGEX_CONFIG_OPTIONS
        UnifiedRegex::RegexStats* stats = 0;
        if (REGEX_CONFIG_FLAG(RegexProfile))
        {
            stats = scriptContext->GetRegexStatsDatabase()->GetRegexStats(pattern);
            scriptContext->GetRegexStatsDatabase()->BeginProfile();
        }
        UnifiedRegex::DebugWriter* w = 0;
        if (REGEX_CONFIG_FLAG(RegexTracing) && CONFIG_FLAG(Verbose))
            w = scriptContext->GetRegexDebugWriter();
#endif

        pattern->rep.unified.matcher->Match
            (state.input
                , inputLength
                , offset
                , scriptContext
#if ENABLE_REGEX_CONFIG_OPTIONS
                , stats
                , w
#endif
                );

#if ENABLE_REGEX_CONFIG_OPTIONS
        if (REGEX_CONFIG_FLAG(RegexProfile))
            scriptContext->GetRegexStatsDatabase()->EndProfile(stats, UnifiedRegex::RegexStats::Execute);
#endif
        return pattern->GetGroup(0);
    }

    void RegexHelper::PrimEndMatch(RegexMatchState& state, ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern)
    {
        if (state.tempAllocatorObj != 0)
            scriptContext->ReleaseTemporaryAllocator(state.tempAllocatorObj);
    }

    Var RegexHelper::NonMatchValue(ScriptContext* scriptContext, bool isGlobalCtor)
    {
        // SPEC DEVIATION: The $n properties of the RegExp ctor use empty strings rather than undefined to represent
        //                 the non-match value, even in ES5 mode.
        if (isGlobalCtor)
            return scriptContext->GetLibrary()->GetEmptyString();
        else
            return scriptContext->GetLibrary()->GetUndefined();
    }

    Var RegexHelper::GetString(ScriptContext* scriptContext, JavascriptString* input, Var nonMatchValue, UnifiedRegex::GroupInfo group)
    {
        if (group.IsUndefined())
            return nonMatchValue;
        switch (group.length)
        {
        case 0:
            return scriptContext->GetLibrary()->GetEmptyString();
        case 1:
        {
            const char16* inputStr = input->GetString();
            return scriptContext->GetLibrary()->GetCharStringCache().GetStringForChar(inputStr[group.offset]);
        }
        case 2:
        {
            const char16* inputStr = input->GetString();
            PropertyString* propString = scriptContext->GetPropertyString2(inputStr[group.offset], inputStr[group.offset + 1]);
            if (propString != 0)
                return propString;
            // fall-through for default
        }
        default:
            return SubString::New(input, group.offset, group.length);
        }
    }

    Var RegexHelper::GetGroup(ScriptContext* scriptContext, UnifiedRegex::RegexPattern* pattern, JavascriptString* input, Var nonMatchValue, int groupId)
    {
        return GetString(scriptContext, input, nonMatchValue, pattern->GetGroup(groupId));
    }

    // ======================================================================
    // Match results propagate into three places:
    //  - The match result array. Generally the array has string entries for the overall match substring,
    //    followed by final bindings for each group, plus the fields:
    //     - 'input': string used in match
    //     - 'index': index of first character of match in input
    //     - 'lastIndex' (IE extension): one plus index of last character of match in input
    //    However, for String.match with a global match, the result is an array of all match results
    //    (ignoring any group bindings). But in IE8 mode we also bind the above fields to that array,
    //    using the results of the last successful primitive match.
    //  - The regular expression object has writable field:
    //     - 'lastIndex': one plus index of last character of last match in last input
    //     - 'lastInput
    //  - (Host extension) The RegExp constructor object has fields:
    //     - '$n': last match substrings, using "" for undefined in all modes
    //     - etc (see JavascriptRegExpConstructorType.cpp)
    //
    // There are also three influences on what gets propagated where and when:
    //  - Whether the regular expression is global
    //  - Whether the primitive operations runs the regular expression until failure (e.g. String.match) or
    //    just once (e.g. RegExp.exec), or use the underlying matching machinery implicitly (e.g. String.split).
    //
    // Here are the rules:
    //  - RegExp is updated for the last *successful* primitive match, except for String.replace.
    //    In particular, for String.match with a global regex, the final failing match *does not* reset RegExp.
    //  - Except for String.search in EC5 mode (which does not update 'lastIndex'), the regular expressions
    //    lastIndex is updated as follows:
    //     - ES5 mode, if a primitive match fails then the regular expression 'lastIndex' is set to 0. In particular,
    //       the final failing primitive match for String.match with a global regex forces 'lastIndex' to be reset.
    //       However, if a primitive match succeeds then the regular expression 'lastIndex' is updated only for
    //       a global regex.
    //       for success. However:
    //        - The last failing match in a String.match with a global regex does NOT reset 'lastIndex'.
    //        - If the regular expression matched empty, the last index is set assuming the pattern actually matched
    //          one input character. This applies even if the pattern matched empty one beyond the end of the string
    //          in a String.match with a global regex (!). For our own sanity, we isolate this particular case
    //          within JavascriptRegExp when setting the lastIndexVar value.
    //  - In all modes, 'lastIndex' determines the starting search index only for global regular expressions.
    //
    // ======================================================================

    void RegexHelper::PropagateLastMatch
        ( ScriptContext* scriptContext
        , bool isGlobal
        , bool isSticky
        , JavascriptRegExp* regularExpression
        , JavascriptString* lastInput
        , UnifiedRegex::GroupInfo lastSuccessfulMatch
        , UnifiedRegex::GroupInfo lastActualMatch
        , bool updateRegex
        , bool updateCtor
        , bool useSplitPattern )
    {
        if (updateRegex)
        {
            PropagateLastMatchToRegex(scriptContext, isGlobal, isSticky, regularExpression, lastSuccessfulMatch, lastActualMatch);
        }
        if (updateCtor)
        {
            PropagateLastMatchToCtor(scriptContext, regularExpression, lastInput, lastSuccessfulMatch, useSplitPattern);
        }
    }

    void RegexHelper::PropagateLastMatchToRegex
        ( ScriptContext* scriptContext
        , bool isGlobal
        , bool isSticky
        , JavascriptRegExp* regularExpression
        , UnifiedRegex::GroupInfo lastSuccessfulMatch
        , UnifiedRegex::GroupInfo lastActualMatch )
    {
        if (lastActualMatch.IsUndefined())
        {
            regularExpression->SetLastIndex(0);
        }
        else if (isGlobal || isSticky)
        {
            CharCount lastIndex = lastActualMatch.EndOffset();
            Assert(lastIndex <= MaxCharCount);
            regularExpression->SetLastIndex((int32)lastIndex);
        }
    }

    void RegexHelper::PropagateLastMatchToCtor
        ( ScriptContext* scriptContext
        , JavascriptRegExp* regularExpression
        , JavascriptString* lastInput
        , UnifiedRegex::GroupInfo lastSuccessfulMatch
        , bool useSplitPattern )
    {
        Assert(lastInput);

        if (!lastSuccessfulMatch.IsUndefined())
        {
            // Notes:
            // - SPEC DEVIATION: The RegExp ctor holds some details of the last successful match on any regular expression.
            // - For updating regex ctor's stats we are using entry function's context, rather than regex context,
            //   the rational is: use same context of RegExp.prototype, on which the function was called.
            //   So, if you call the function with remoteContext.regexInstance.exec.call(localRegexInstance, "match string"),
            //   we will update stats in the context related to the exec function, i.e. remoteContext.
            //   This is consistent with other browsers
            UnifiedRegex::RegexPattern* pattern = useSplitPattern
                ? regularExpression->GetSplitPattern()
                : regularExpression->GetPattern();
            scriptContext->GetLibrary()->GetRegExpConstructor()->SetLastMatch(pattern, lastInput, lastSuccessfulMatch);
        }
    }

    bool RegexHelper::GetInitialOffset(bool isGlobal, bool isSticky, JavascriptRegExp* regularExpression, CharCount inputLength, CharCount& offset)
    {
        if (isGlobal || isSticky)
        {
            offset = regularExpression->GetLastIndex();
            if (offset <= MaxCharCount)
                return true;
            else
            {
                regularExpression->SetLastIndex(0);
                return false;
            }
        }
        else
        {
            offset = 0;
            return true;
        }
    }

    JavascriptArray* RegexHelper::CreateMatchResult(void *const stackAllocationPointer, ScriptContext* scriptContext, bool isGlobal, int numGroups, JavascriptString* input)
    {
        if (isGlobal)
        {
            // Use an ordinary array, with default initial capacity
            return scriptContext->GetLibrary()->CreateArrayOnStack(stackAllocationPointer);
        }
        else
            return JavascriptRegularExpressionResult::Create(stackAllocationPointer, numGroups, input, scriptContext);
    }

    void RegexHelper::FinalizeMatchResult(ScriptContext* scriptContext, bool isGlobal, JavascriptArray* arr, UnifiedRegex::GroupInfo match)
    {
        if (!isGlobal)
            JavascriptRegularExpressionResult::SetMatch(arr, match);
        // else: arr is an ordinary array
    }

    JavascriptArray* RegexHelper::CreateExecResult(void *const stackAllocationPointer, ScriptContext* scriptContext, int numGroups, JavascriptString* input, UnifiedRegex::GroupInfo match)
    {
        JavascriptArray* res = JavascriptRegularExpressionResult::Create(stackAllocationPointer, numGroups, input, scriptContext);
        JavascriptRegularExpressionResult::SetMatch(res, match);
        return res;
    }

    template<bool mustMatchEntireInput>
    BOOL RegexHelper::RegexTest_NonScript(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, const char16 *const input, const CharCount inputLength)
    {
        // This version of the function should only be used when testing the regex against a non-javascript string. That is,
        // this call was not initiated by script code. Hence, the RegExp constructor is not updated with the last match. If
        // 'mustMatchEntireInput' is true, this function also ignores the global/sticky flag and the lastIndex property, since it tests
        // for a match on the entire input string; in that case, the lastIndex property is not modified.

        UnifiedRegex::RegexPattern* pattern = regularExpression->GetPattern();
        UnifiedRegex::GroupInfo match; // initially undefined

#if ENABLE_REGEX_CONFIG_OPTIONS
        RegexHelperTrace(scriptContext, UnifiedRegex::RegexStats::Test, regularExpression, input, inputLength);
#endif
        const bool isGlobal = pattern->IsGlobal();
        const bool isSticky = pattern->IsSticky();
        CharCount offset;
        if (mustMatchEntireInput)
            offset = 0; // needs to match the entire input, so ignore 'lastIndex' and always start from the beginning
        else if (!GetInitialOffset(isGlobal, isSticky, regularExpression, inputLength, offset))
            return false;

        if (mustMatchEntireInput || offset <= inputLength)
        {
            match = RegexHelper::SimpleMatch(scriptContext, pattern, input, inputLength, offset);
        }
        // else: match remains undefined

        if (!mustMatchEntireInput) // don't update 'lastIndex' when mustMatchEntireInput is true since the global flag is ignored
        {
            PropagateLastMatchToRegex(scriptContext, isGlobal, isSticky, regularExpression, match, match);
        }

        return mustMatchEntireInput ? match.offset == 0 && match.length == inputLength : !match.IsUndefined();
    }

    // explicit instantiation
    template BOOL RegexHelper::RegexTest_NonScript<true>(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, const char16 *const input, const CharCount inputLength);
    template BOOL RegexHelper::RegexTest_NonScript<false>(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, const char16 *const input, const CharCount inputLength);

    // Asserts if the value needs to be marshaled to target context.
    // Returns the resulting value.
    // This is supposed to be called for result/return value of the RegexXXX functions.
    // static
    template<typename T>
    T RegexHelper::CheckCrossContextAndMarshalResult(T value, ScriptContext* targetContext)
    {
        Assert(targetContext);
        Assert(!CrossSite::NeedMarshalVar(value, targetContext));
        return value;
    }

    Var RegexHelper::RegexMatchResultUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        return RegexHelper::RegexMatch(scriptContext, regularExpression, input, false);
    }

    Var RegexHelper::RegexMatchResultUsedAndMayBeTemp(void *const stackAllocationPointer, ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        return RegexHelper::RegexMatch(scriptContext, regularExpression, input, false, stackAllocationPointer);
    }

    Var RegexHelper::RegexMatchResultNotUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        if (!PHASE_OFF1(Js::RegexResultNotUsedPhase))
        {
            return RegexHelper::RegexMatch(scriptContext, regularExpression, input, true);
        }
        else
        {
            return RegexHelper::RegexMatch(scriptContext, regularExpression, input, false);
        }
    }

    Var RegexHelper::RegexMatch(ScriptContext* entryFunctionContext, RecyclableObject *thisObj, JavascriptString *input, bool noResult, void *const stackAllocationPointer)
    {
        Var result = RegexHelper::RegexMatchImpl<true>(entryFunctionContext, thisObj, input, noResult, stackAllocationPointer);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexMatchNoHistory(ScriptContext* entryFunctionContext, JavascriptRegExp *regularExpression, JavascriptString *input, bool noResult)
    {
        // RegexMatchNoHistory() is used only by Intl internally and there is no need for ES6
        // observable RegExp actions. Therefore, we can directly use the ES5 logic.
        Var result = RegexHelper::RegexEs5MatchImpl<false>(entryFunctionContext, regularExpression, input, noResult);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexExecResultUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        return RegexHelper::RegexExec(scriptContext, regularExpression, input, false);
    }

    Var RegexHelper::RegexExecResultUsedAndMayBeTemp(void *const stackAllocationPointer, ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        return RegexHelper::RegexExec(scriptContext, regularExpression, input, false, stackAllocationPointer);
    }

    Var RegexHelper::RegexExecResultNotUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        if (!PHASE_OFF1(Js::RegexResultNotUsedPhase))
        {
            return RegexHelper::RegexExec(scriptContext, regularExpression, input, true);
        }
        else
        {
            return RegexHelper::RegexExec(scriptContext, regularExpression, input, false);
        }
    }

    Var RegexHelper::RegexExec(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input, bool noResult, void *const stackAllocationPointer)
    {
        Var result = RegexHelper::RegexExecImpl(entryFunctionContext, regularExpression, input, noResult, stackAllocationPointer);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexReplaceResultUsed(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace)
    {
        return entryFunctionContext->GetConfig()->IsES6RegExSymbolsEnabled()
            ? RegexHelper::RegexReplace(entryFunctionContext, regularExpression, input, replace, false)
            : RegexHelper::RegexEs5Replace(entryFunctionContext, regularExpression, input, replace, false);
    }

    Var RegexHelper::RegexReplaceResultNotUsed(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace)
    {
        if (!PHASE_OFF1(Js::RegexResultNotUsedPhase))
        {
            return entryFunctionContext->GetConfig()->IsES6RegExSymbolsEnabled()
                ? RegexHelper::RegexReplace(entryFunctionContext, regularExpression, input, replace, true)
                : RegexHelper::RegexEs5Replace(entryFunctionContext, regularExpression, input, replace, true);
        }
        else
        {
            return entryFunctionContext->GetConfig()->IsES6RegExSymbolsEnabled()
                ? RegexHelper::RegexReplace(entryFunctionContext, regularExpression, input, replace, false)
                : RegexHelper::RegexEs5Replace(entryFunctionContext, regularExpression, input, replace, false);
        }

    }

    Var RegexHelper::RegexReplace(ScriptContext* entryFunctionContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptString* replace, bool noResult)
    {
        Var result = RegexHelper::RegexReplaceImpl(entryFunctionContext, thisObj, input, replace, noResult);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexEs5Replace(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input, JavascriptString* replace, bool noResult)
    {
        // We can have RegexReplaceResult... functions defer their job to RegexReplace. However, their regularExpression argument
        // would first be cast to RecyclableObject when the call is made, and then back to JavascriptRegExp in RegexReplaceImpl.
        // The conversion back slows down the perf, so we use this ES5 version of RegexReplace in RegexReplaceResult... if we know
        // that the ES6 logic isn't needed.

        Var result = RegexHelper::RegexEs5ReplaceImpl(entryFunctionContext, regularExpression, input, replace, noResult);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexReplaceFunction(ScriptContext* entryFunctionContext, RecyclableObject* thisObj, JavascriptString* input, JavascriptFunction* replacefn)
    {
        Var result = RegexHelper::RegexReplaceImpl(entryFunctionContext, thisObj, input, replacefn);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexSearch(ScriptContext* entryFunctionContext, JavascriptRegExp* regularExpression, JavascriptString* input)
    {
        Var result = RegexHelper::RegexSearchImpl(entryFunctionContext, regularExpression, input);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    Var RegexHelper::RegexSplitResultUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit)
    {
        return RegexHelper::RegexSplit(scriptContext, regularExpression, input, limit, false);
    }

    Var RegexHelper::RegexSplitResultUsedAndMayBeTemp(void *const stackAllocationPointer, ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit)
    {
        return RegexHelper::RegexSplit(scriptContext, regularExpression, input, limit, false, stackAllocationPointer);
    }

    Var RegexHelper::RegexSplitResultNotUsed(ScriptContext* scriptContext, JavascriptRegExp* regularExpression, JavascriptString* input, CharCount limit)
    {
        if (!PHASE_OFF1(Js::RegexResultNotUsedPhase))
        {
            return RegexHelper::RegexSplit(scriptContext, regularExpression, input, limit, true);
        }
        else
        {
            return RegexHelper::RegexSplit(scriptContext, regularExpression, input, limit, false);
        }
    }

    Var RegexHelper::RegexSplit(ScriptContext* entryFunctionContext, RecyclableObject* thisObj, JavascriptString* input, CharCount limit, bool noResult, void *const stackAllocationPointer)
    {
        Var result = RegexHelper::RegexSplitImpl(entryFunctionContext, thisObj, input, limit, noResult, stackAllocationPointer);
        return RegexHelper::CheckCrossContextAndMarshalResult(result, entryFunctionContext);
    }

    RecyclableObject* RegexHelper::ExecResultToRecyclableObject(Var result)
    {
        // "result" is the result of the "exec" call. "CallExec" makes sure that it is either
        // an Object or Null. RegExp algorithms have special conditions for when the result is Null,
        // so we can directly cast to RecyclableObject.
        Assert(!JavascriptOperators::IsNull(result));
        return RecyclableObject::FromVar(result);
    }

    JavascriptString* RegexHelper::GetMatchStrFromResult(RecyclableObject* result, ScriptContext* scriptContext)
    {
        return JavascriptConversion::ToString(
            JavascriptOperators::GetItem(result, (uint32)0, scriptContext),
            scriptContext);
    }

    void RegexHelper::AdvanceLastIndex(
        RecyclableObject* instance,
        JavascriptString* input,
        JavascriptString* matchStr,
        bool unicode,
        ScriptContext* scriptContext)
    {
        if (matchStr->GetLength() == 0)
        {
            CharCount lastIndex = JavascriptRegExp::GetLastIndexProperty(instance, scriptContext);
            lastIndex = AdvanceStringIndex(input, lastIndex, unicode);
            JavascriptRegExp::SetLastIndexProperty(instance, lastIndex, scriptContext);
        }
    }

    CharCount RegexHelper::AdvanceStringIndex(JavascriptString* string, CharCount index, bool isUnicode)
    {
        // TODO: Change the increment to 2 depending on the "unicode" flag and
        // the code point at "index". The increment is currently constant at 1
        // in order to be compatible with the rest of the RegExp code.

        return JavascriptRegExp::AddIndex(index, 1);
    }
}
