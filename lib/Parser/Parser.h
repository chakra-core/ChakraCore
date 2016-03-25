//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define REGEX_TRIGRAMS 1

#include "Common.h"

// FORWARD
namespace Js
{
    class ScriptContext;
    class JavascriptString;
}

// TODO: Get rid of this once we can move PlatformAgnostic back to Common
namespace PlatformAgnostic
{
    namespace UnicodeText
    {
        enum class CharacterClassificationType;
        enum CharacterTypeFlags: byte;
    }
}

namespace UnifiedRegex {
    struct RegexPattern;
    struct Program;
    template <typename T> class StandardChars;
    typedef StandardChars<uint8> UTF8StandardChars;
    typedef StandardChars<char16> UnicodeStandardChars;
#if ENABLE_REGEX_CONFIG_OPTIONS
    class DebugWriter;
    struct RegexStats;
    class RegexStatsDatabase;
#endif
}

#include "ParserCommon.h"
#include "Alloc.h"
#include "cmperr.h"
#include "idiom.h"
#include "popcode.h"
#include "ptree.h"
#include "tokens.h"
#include "Hash.h"
#include "CharClassifier.h"
#include "Scan.h"
#include "screrror.h"
#include "rterror.h"
#include "Parse.h"

#include "BackgroundParser.h"
