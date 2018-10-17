//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

namespace UnifiedRegex
{
    RegexPattern::RegexPattern(Js::JavascriptLibrary *const library, Program* program, bool isLiteral)
        : library(library), isLiteral(isLiteral), isShallowClone(false), testCache(nullptr)
    {
        rep.unified.program = program;
        rep.unified.matcher = nullptr;
        rep.unified.trigramInfo = nullptr;
    }

    RegexPattern *RegexPattern::New(Js::ScriptContext *scriptContext, Program* program, bool isLiteral)
    {
        return
            RecyclerNewFinalized(
                scriptContext->GetRecycler(),
                RegexPattern,
                scriptContext->GetLibrary(),
                program,
                isLiteral);
    }

    void RegexPattern::Finalize(bool isShutdown)
    {
        if (isShutdown)
        {
            return;
        }

        const auto scriptContext = GetScriptContext();
        if (!scriptContext)
        {
            return;
        }

#if DBG
        // In JSRT or ChakraEngine, we might not have a chance to close at finalize time
        if (!isLiteral && !scriptContext->IsClosed() &&
            !scriptContext->GetThreadContext()->IsJSRT() &&
            !scriptContext->GetLibrary()->IsChakraEngine())
        {
            const auto source = GetSource();
            RegexPattern *p = nullptr;

            bool hasRegexPatternForSourceKey = GetScriptContext()->GetDynamicRegexMap()->TryGetValue(
                RegexKey(source.GetBuffer(), source.GetLength(), GetFlags()), &p);
            bool isSourceLengthZero = source.GetLength() == 0;
            bool isUniquePattern = p != this;

            Assert(!hasRegexPatternForSourceKey || isSourceLengthZero || isUniquePattern);
        }
#endif

        if (isShallowClone)
        {
            return;
        }

        rep.unified.program->FreeBody(scriptContext->RegexAllocator());
    }

    void RegexPattern::Dispose(bool isShutdown)
    {
    }

    Js::ScriptContext *RegexPattern::GetScriptContext() const
    {
        return library->GetScriptContext();
    }

    Js::InternalString RegexPattern::GetSource() const
    {
        return Js::InternalString(rep.unified.program->source, rep.unified.program->sourceLen);
    }

    RegexFlags RegexPattern::GetFlags() const
    {
        return rep.unified.program->flags;
    }

    uint16 RegexPattern::NumGroups() const
    {
        return rep.unified.program->numGroups;
    }

    bool RegexPattern::IsIgnoreCase() const
    {
        return (rep.unified.program->flags & IgnoreCaseRegexFlag) != 0;
    }

    bool RegexPattern::IsDotAll() const
    {
        return GetScriptContext()->GetConfig()->IsES2018RegExDotAllEnabled() && (rep.unified.program->flags & DotAllRegexFlag) != 0;
    }

    bool RegexPattern::IsGlobal() const
    {
        return (rep.unified.program->flags & GlobalRegexFlag) != 0;
    }

    bool RegexPattern::IsMultiline() const
    {
        return (rep.unified.program->flags & MultilineRegexFlag) != 0;
    }

    bool RegexPattern::IsUnicode() const
    {
        return GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled() && (rep.unified.program->flags & UnicodeRegexFlag) != 0;
    }

    bool RegexPattern::IsSticky() const
    {
        return GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled() && (rep.unified.program->flags & StickyRegexFlag) != 0;
    }

    bool RegexPattern::WasLastMatchSuccessful() const
    {
        return rep.unified.matcher != 0 && rep.unified.matcher->WasLastMatchSuccessful();
    }

    GroupInfo RegexPattern::GetGroup(int groupId) const
    {
        Assert(groupId == 0 || WasLastMatchSuccessful());
        Assert(groupId >= 0 && groupId < NumGroups());
        return rep.unified.matcher->GetGroup(groupId);
    }

    RegexPattern *RegexPattern::CopyToScriptContext(Js::ScriptContext *scriptContext)
    {
        // This routine assumes that this instance will outlive the copy, which is the case for copy-on-write,
        // and therefore doesn't copy the immutable parts of the pattern. This should not be confused with a
        // would be CloneToScriptContext which will would clone the immutable parts as well because the lifetime
        // of a clone might be longer than the original.

        RegexPattern *result = UnifiedRegex::RegexPattern::New(scriptContext, rep.unified.program, isLiteral);
        Matcher *matcherClone = rep.unified.matcher ? rep.unified.matcher->CloneToScriptContext(scriptContext, result) : nullptr;
        result->rep.unified.matcher = matcherClone;
        result->isShallowClone = true;
        return result;
    }

    Field(RegExpTestCache*) RegexPattern::EnsureTestCache()
    {
        if (this->testCache == nullptr)
        {
            this->testCache = RecyclerNewPlusZ(this->library->GetRecycler(), TestCacheSize * sizeof(void*), RegExpTestCache);
        }
        return this->testCache;
    }

    /* static */
    uint RegexPattern::GetTestCacheIndex(Js::JavascriptString* str)
    {
        return (uint)(((uintptr_t)str) >> PolymorphicInlineCacheShift) & (TestCacheSize - 1);
    }

#if ENABLE_REGEX_CONFIG_OPTIONS
    void RegexPattern::Print(DebugWriter* w)
    {
        w->Print(_u("/"));

        Js::InternalString str = GetSource();
        if (str.GetLength() == 0)
            w->Print(_u("(?:)"));
        else
        {
            for (charcount_t i = 0; i < str.GetLength(); ++i)
            {
                const char16 c = str.GetBuffer()[i];
                switch(c)
                {
                case _u('/'):
                    w->Print(_u("\\%lc"), c);
                    break;
                case _u('\n'):
                case _u('\r'):
                case _u('\x2028'):
                case _u('\x2029'):
                    w->PrintEscapedChar(c);
                    break;
                case _u('-'):
                    w->Print(_u("-"));
                    break;
                case _u('\\'):
                    Assert(i + 1 < str.GetLength()); // cannot end in a '\'
                    w->Print(_u("\\%lc"), str.GetBuffer()[++i]);
                    break;
                default:
                    w->PrintEscapedChar(c);
                    break;
                }
            }
        }
        w->Print(_u("/"));
        if (IsIgnoreCase())
            w->Print(_u("i"));
        if (IsGlobal())
            w->Print(_u("g"));
        if (IsMultiline())
            w->Print(_u("m"));
        if (IsDotAll())
            w->Print(_u("s"));
        if (IsUnicode())
            w->Print(_u("u"));
        if (IsSticky())
            w->Print(_u("y"));
        w->Print(_u(" /* "));
        w->Print(_u(", "));
        w->Print(isLiteral ? _u("literal") : _u("dynamic"));
        w->Print(_u(" */"));
    }

    /* static */
    void RegexPattern::TraceTestCache(bool cacheHit, Js::JavascriptString* input, Js::JavascriptString* cachedValue, bool disabled)
    {
        if (REGEX_CONFIG_FLAG(RegexTracing))
        {
            if (disabled)
            {
                Output::Print(_u("Regexp Test Cache Disabled.\n"));
            }
            else if (cacheHit)
            {
                Output::Print(_u("Regexp Test Cache Hit.\n"));
            }
            else
            {
                Output::Print(_u("Regexp Test Cache Miss. "));
                if (cachedValue != nullptr)
                {
                    Output::Print(_u("Input: (%p); Cached String: (%p) '%s'\n"), input, cachedValue, cachedValue->GetString());
                }
                else
                {
                    Output::Print(_u("Cache was empty\n"));
                }
            }
        }
    }
#endif
}
