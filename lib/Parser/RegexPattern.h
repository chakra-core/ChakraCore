//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptLibrary;
}

namespace UnifiedRegex
{
    struct Program;
    class Matcher;
    struct TrigramInfo;

    static const uint TestCacheSize = 8;

    struct RegExpTestCache
    {
        Field(BVStatic<TestCacheSize>) resultBV;
        Field(RecyclerWeakReference<Js::JavascriptString>*) inputArray[];
    };

    struct RegexPattern : FinalizableObject
    {
        Field(RegExpTestCache*) testCache;

        struct UnifiedRep
        {
            Field(Program*) program;
            Field(Matcher*) matcher;
            Field(TrigramInfo*) trigramInfo;
        };

        Field(Js::JavascriptLibrary *) const library;

        Field(bool) isLiteral : 1;
        Field(bool) isShallowClone : 1;

        union Rep
        {
            Field(UnifiedRep) unified;

            Rep() : unified() {}
        };
        Field(Rep) rep;

        RegexPattern(Js::JavascriptLibrary *const library, Program* program, bool isLiteral);

        static RegexPattern *New(Js::ScriptContext *scriptContext, Program* program, bool isLiteral);

        virtual void Finalize(bool isShutdown) override;
        virtual void Dispose(bool isShutdown) override;
        virtual void Mark(Recycler *recycler) override { AssertMsg(false, "Mark called on object that isn't TrackableObject"); }

        Js::ScriptContext *GetScriptContext() const;

        inline bool IsLiteral() const { return isLiteral; }
        uint16 NumGroups() const;
        bool IsIgnoreCase() const;
        bool IsGlobal() const;
        bool IsMultiline() const;
        bool IsDotAll() const;
        bool IsUnicode() const;
        bool IsSticky() const;
        bool WasLastMatchSuccessful() const;
        GroupInfo GetGroup(int groupId) const;

        Js::InternalString GetSource() const;
        RegexFlags GetFlags() const;

        Field(RegExpTestCache*) EnsureTestCache();
        static uint GetTestCacheIndex(Js::JavascriptString* str);

#if ENABLE_REGEX_CONFIG_OPTIONS
        void Print(DebugWriter* w);
#endif
        RegexPattern *CopyToScriptContext(Js::ScriptContext *scriptContext);

#if ENABLE_REGEX_CONFIG_OPTIONS
        static void TraceTestCache(bool cacheHit, Js::JavascriptString* input, Js::JavascriptString* cachedValue, bool disabled);
#endif
    };
}
