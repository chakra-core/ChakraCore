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

    struct RegexPattern : FinalizableObject
    {

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
        int NumGroups() const;
        bool IsIgnoreCase() const;
        bool IsGlobal() const;
        bool IsMultiline() const;
        bool IsUnicode() const;
        bool IsSticky() const;
        bool WasLastMatchSuccessful() const;
        GroupInfo GetGroup(int groupId) const;

        Js::InternalString GetSource() const;
        RegexFlags GetFlags() const;
#if ENABLE_REGEX_CONFIG_OPTIONS
        void Print(DebugWriter* w);
#endif
        RegexPattern *CopyToScriptContext(Js::ScriptContext *scriptContext);
    };
}
