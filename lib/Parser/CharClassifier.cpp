//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
        // Technically, this is not specific to Unicode (in fact, it's used in the non-Unicode case)
        // But it's in this namespace for convenience
        static const CharacterTypeFlags charFlags[128] =
        {
            UnknownChar,                 /* 0x00   */
            UnknownChar,                 /* 0x01   */
            UnknownChar,                 /* 0x02   */
            UnknownChar,                 /* 0x03   */
            UnknownChar,                 /* 0x04   */
            UnknownChar,                 /* 0x05   */
            UnknownChar,                 /* 0x06   */
            UnknownChar,                 /* 0x07   */
            UnknownChar,                 /* 0x08   */
            SpaceChar,                   /* 0x09   */
            LineCharGroup,               /* 0x0A   */
            SpaceChar,                   /* 0x0B   */
            SpaceChar,                   /* 0x0C   */
            LineCharGroup,               /* 0x0D   */
            UnknownChar,                 /* 0x0E   */
            UnknownChar,                 /* 0x0F   */
            UnknownChar,                 /* 0x10   */
            UnknownChar,                 /* 0x11   */
            UnknownChar,                 /* 0x12   */
            UnknownChar,                 /* 0x13   */
            UnknownChar,                 /* 0x14   */
            UnknownChar,                 /* 0x15   */
            UnknownChar,                 /* 0x16   */
            UnknownChar,                 /* 0x17   */
            UnknownChar,                 /* 0x18   */
            UnknownChar,                 /* 0x19   */
            UnknownChar,                 /* 0x1A   */
            UnknownChar,                 /* 0x1B   */
            UnknownChar,                 /* 0x1C   */
            UnknownChar,                 /* 0x1D   */
            UnknownChar,                 /* 0x1E   */
            UnknownChar,                 /* 0x1F   */
            SpaceChar,                   /* 0x20   */
            UnknownChar,                 /* 0x21 ! */
            UnknownChar,                 /* 0x22   */
            UnknownChar,                 /* 0x23 # */
            LetterCharGroup,             /* 0x24 $ */
            UnknownChar,                 /* 0x25 % */
            UnknownChar,                 /* 0x26 & */
            UnknownChar,                 /* 0x27   */
            UnknownChar,                 /* 0x28   */
            UnknownChar,                 /* 0x29   */
            UnknownChar,                 /* 0x2A   */
            UnknownChar,                 /* 0x2B   */
            UnknownChar,                 /* 0x2C   */
            UnknownChar,                 /* 0x2D   */
            UnknownChar,                 /* 0x2E   */
            UnknownChar,                 /* 0x2F   */
            DecimalCharGroup,            /* 0x30 0 */
            DecimalCharGroup,            /* 0x31 1 */
            DecimalCharGroup,            /* 0x32 2 */
            DecimalCharGroup,            /* 0x33 3 */
            DecimalCharGroup,            /* 0x34 4 */
            DecimalCharGroup,            /* 0x35 5 */
            DecimalCharGroup,            /* 0x36 6 */
            DecimalCharGroup,            /* 0x37 7 */
            DecimalCharGroup,            /* 0x38 8 */
            DecimalCharGroup,            /* 0x39 9 */
            UnknownChar,                 /* 0x3A   */
            UnknownChar,                 /* 0x3B   */
            UnknownChar,                 /* 0x3C < */
            UnknownChar,                 /* 0x3D = */
            UnknownChar,                 /* 0x3E > */
            UnknownChar,                 /* 0x3F   */
            UnknownChar,                 /* 0x40 @ */
            HexCharGroup,                /* 0x41 A */
            HexCharGroup,                /* 0x42 B */
            HexCharGroup,                /* 0x43 C */
            HexCharGroup,                /* 0x44 D */
            HexCharGroup,                /* 0x45 E */
            HexCharGroup,                /* 0x46 F */
            LetterCharGroup,             /* 0x47 G */
            LetterCharGroup,             /* 0x48 H */
            LetterCharGroup,             /* 0x49 I */
            LetterCharGroup,             /* 0x4A J */
            LetterCharGroup,             /* 0x4B K */
            LetterCharGroup,             /* 0x4C L */
            LetterCharGroup,             /* 0x4D M */
            LetterCharGroup,             /* 0x4E N */
            LetterCharGroup,             /* 0x4F O */
            LetterCharGroup,             /* 0x50 P */
            LetterCharGroup,             /* 0x51 Q */
            LetterCharGroup,             /* 0x52 R */
            LetterCharGroup,             /* 0x53 S */
            LetterCharGroup,             /* 0x54 T */
            LetterCharGroup,             /* 0x55 U */
            LetterCharGroup,             /* 0x56 V */
            LetterCharGroup,             /* 0x57 W */
            LetterCharGroup,             /* 0x58 X */
            LetterCharGroup,             /* 0x59 Y */
            LetterCharGroup,             /* 0x5A Z */
            UnknownChar,                 /* 0x5B   */
            UnknownChar,                 /* 0x5C   */
            UnknownChar,                 /* 0x5D   */
            UnknownChar,                 /* 0x5E   */
            LetterCharGroup,             /* 0x5F _ */
            UnknownChar,                 /* 0x60   */
            HexCharGroup,                /* 0x61 a */
            HexCharGroup,                /* 0x62 b */
            HexCharGroup,                /* 0x63 c */
            HexCharGroup,                /* 0x64 d */
            HexCharGroup,                /* 0x65 e */
            HexCharGroup,                /* 0x66 f */
            LetterCharGroup,             /* 0x67 g */
            LetterCharGroup,             /* 0x68 h */
            LetterCharGroup,             /* 0x69 i */
            LetterCharGroup,             /* 0x6A j */
            LetterCharGroup,             /* 0x6B k */
            LetterCharGroup,             /* 0x6C l */
            LetterCharGroup,             /* 0x6D m */
            LetterCharGroup,             /* 0x6E n */
            LetterCharGroup,             /* 0x6F o */
            LetterCharGroup,             /* 0x70 p */
            LetterCharGroup,             /* 0x71 q */
            LetterCharGroup,             /* 0x72 r */
            LetterCharGroup,             /* 0x73 s */
            LetterCharGroup,             /* 0x74 t */
            LetterCharGroup,             /* 0x75 u */
            LetterCharGroup,             /* 0x76 v */
            LetterCharGroup,             /* 0x77 w */
            LetterCharGroup,             /* 0x78 x */
            LetterCharGroup,             /* 0x79 y */
            LetterCharGroup,             /* 0x7A z */
            UnknownChar,                 /* 0x7B   */
            UnknownChar,                 /* 0x7C   */
            UnknownChar,                 /* 0x7D   */
            UnknownChar,                 /* 0x7E   */
            UnknownChar                  /* 0x7F   */
        };
    }
};

    /*****************************************************************************
*
*  The _C_xxx enum and charTypes[] table are used to map a character to
*  simple classification values and flags.
*/

static const CharTypes charTypes[128] =
{
    _C_NUL, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR,     /* 00-07 */
    _C_ERR, _C_WSP, _C_NWL, _C_WSP, _C_WSP, _C_NWL, _C_ERR, _C_ERR,     /* 08-0F */

    _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR,     /* 10-17 */
    _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR,     /* 18-1F */

    _C_WSP, _C_BNG, _C_QUO, _C_SHP, _C_DOL, _C_PCT, _C_AMP, _C_APO,     /* 20-27 */
    _C_LPR, _C_RPR, _C_MUL, _C_PLS, _C_CMA, _C_MIN, _C_DOT, _C_SLH,     /* 28-2F */

    _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG,     /* 30-37 */
    _C_DIG, _C_DIG, _C_COL, _C_SMC, _C_LT , _C_EQ , _C_GT , _C_QUE,     /* 38-3F */

    _C_AT , _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET,     /* 40-47 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET,     /* 48-4F */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET,     /* 50-57 */
    _C_LET, _C_LET, _C_LET, _C_LBR, _C_BSL, _C_RBR, _C_XOR, _C_USC,     /* 58-5F */

    _C_BKQ, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET,     /* 60-67 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET,     /* 68-6F */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET,     /* 70-77 */
    _C_LET, _C_LET, _C_LET, _C_LC , _C_BAR, _C_RC , _C_TIL, _C_ERR,     /* 78-7F */
};


#if ENABLE_UNICODE_API
bool Js::CharClassifier::BigCharIsWhitespaceDefault(codepoint_t ch, const Js::CharClassifier *instance)
{
    return (instance->getBigCharFlagsFunc(ch, instance) & PlatformAgnostic::UnicodeText::CharacterTypeFlags::SpaceChar) != 0;
}

bool Js::CharClassifier::BigCharIsIdStartDefault(codepoint_t ch, const Js::CharClassifier *instance)
{
    return (instance->getBigCharFlagsFunc(ch, instance) & PlatformAgnostic::UnicodeText::CharacterTypeFlags::IdLeadChar) != 0;
}

bool Js::CharClassifier::BigCharIsIdContinueDefault(codepoint_t ch, const Js::CharClassifier *instance)
{
    return (instance->getBigCharFlagsFunc(ch, instance) & PlatformAgnostic::UnicodeText::CharacterTypeFlags::IdChar) != 0;
}
#endif

CharTypes Js::CharClassifier::GetBigCharTypeES5(codepoint_t codepoint, const Js::CharClassifier *instance)
{
    using namespace PlatformAgnostic::UnicodeText;

    if (codepoint > 0xFFFF)
    {
        return CharTypes::_C_ERR;
    }

    if (codepoint == kchLS || codepoint == kchPS)
    {
        return _C_NWL;
    }

    auto charType = GetLegacyCharacterClassificationType((char16)codepoint);
    if (charType == CharacterClassificationType::Letter)
    {
        return CharTypes::_C_LET;
    }
    else if (charType == CharacterClassificationType::Whitespace)
    {
        return CharTypes::_C_WSP;
    }

    return CharTypes::_C_ERR;
}

PlatformAgnostic::UnicodeText::CharacterTypeFlags Js::CharClassifier::GetBigCharFlagsES5(codepoint_t ch, const Js::CharClassifier *instance)
{
    using namespace PlatformAgnostic::UnicodeText;
    //In ES5 the unicode <ZWNJ> and <ZWJ> could be identifier parts
    if (ch == 0x200c || ch == 0x200d)
    {
        return PlatformAgnostic::UnicodeText::CharacterTypeFlags::IdChar;
    }

    // Make sure that the codepoint fits within the char16 range
    if (ch > 0xFFFF)
    {
        return UnknownChar;
    }

    return PlatformAgnostic::UnicodeText::GetLegacyCharacterTypeFlags((char16)ch);
}



/*
 * CharClassifier implementation
 */

CharTypes Js::CharClassifier::GetBigCharTypeES6(codepoint_t ch, const Js::CharClassifier *instance)
{
    using namespace PlatformAgnostic::UnicodeText;

    Assert(ch > 0x7F);
    if (ch == 0xFEFF)
    {
        return CharTypes::_C_WSP;
    }

    UnicodeGeneralCategoryClass categoryClass = PlatformAgnostic::UnicodeText::GetGeneralCategoryClass(ch);

    switch(categoryClass)
    {
        case UnicodeGeneralCategoryClass::CategoryClassLetter:
            return CharTypes::_C_LET;
        case UnicodeGeneralCategoryClass::CategoryClassDigit:
            return CharTypes::_C_DIG;
        case UnicodeGeneralCategoryClass::CategoryClassLineSeparator:
        case UnicodeGeneralCategoryClass::CategoryClassParagraphSeparator:
            return CharTypes::_C_NWL;
        case UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator:
        case UnicodeGeneralCategoryClass::CategoryClassSpacingCombiningMark:
        case UnicodeGeneralCategoryClass::CategoryClassNonSpacingMark:
        case UnicodeGeneralCategoryClass::CategoryClassConnectorPunctuation:
            return CharTypes::_C_WSP;
        default:
            break;
    }

    return CharTypes::_C_UNK;
}

/*
From Unicode 6.3 http://www.unicode.org/reports/tr31/tr31-19.html

ID_Start:::
    Characters having the Unicode General_Category of uppercase letters (Lu), lowercase letters (Ll), titlecase letters (Lt), modifier letters (Lm), other letters (Lo), letter numbers (Nl), minus Pattern_Syntax and Pattern_White_Space code points, plus stability extensions. Note that "other letters" includes ideographs.
    In set notation, this is [[:L:][:Nl:]--[:Pattern_Syntax:]--[:Pattern_White_Space:]] plus stability extensions.

ID_Continue:::
    All of the above, plus characters having the Unicode General_Category of nonspacing marks (Mn), spacing combining marks (Mc), decimal number (Nd), connector punctuations (Pc), plus stability extensions, minus Pattern_Syntax and Pattern_White_Space code points.
    In set notation, this is [[:L:][:Nl:][:Mn:][:Mc:][:Nd:][:Pc:]--[:Pattern_Syntax:]--[:Pattern_White_Space:]] plus stability extensions.

These are also known simply as Identifier Characters, because they are a superset of the ID_Start characters.
*/

PlatformAgnostic::UnicodeText::CharacterTypeFlags Js::CharClassifier::GetBigCharFlagsES6(codepoint_t ch, const Js::CharClassifier *instance)
{
    using namespace PlatformAgnostic::UnicodeText;
    Assert(ch > 0x7F);

    UnicodeGeneralCategoryClass categoryClass = PlatformAgnostic::UnicodeText::GetGeneralCategoryClass(ch);

    switch(categoryClass)
    {
        case UnicodeGeneralCategoryClass::CategoryClassLetter:
            return BigCharIsIdStartES6(ch, instance) ? CharacterTypeFlags::LetterCharGroup : CharacterTypeFlags::UnknownChar;

        case UnicodeGeneralCategoryClass::CategoryClassSpacingCombiningMark:
            return BigCharIsIdContinueES6(ch, instance) ? CharacterTypeFlags::IdChar : CharacterTypeFlags::SpaceChar;

        case UnicodeGeneralCategoryClass::CategoryClassNonSpacingMark:
        case UnicodeGeneralCategoryClass::CategoryClassConnectorPunctuation:
            return BigCharIsIdContinueES6(ch, instance) ? CharacterTypeFlags::IdChar : CharacterTypeFlags::UnknownChar;

        case UnicodeGeneralCategoryClass::CategoryClassDigit:
            return BigCharIsIdContinueES6(ch, instance) ? CharacterTypeFlags::DecimalCharGroup : CharacterTypeFlags::DecimalChar;

        case UnicodeGeneralCategoryClass::CategoryClassLineSeparator:
            return CharacterTypeFlags::LineFeedChar;

        case UnicodeGeneralCategoryClass::CategoryClassParagraphSeparator:
        case UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator:
            return CharacterTypeFlags::SpaceChar;

        default:
            break;
    }

    return CharacterTypeFlags::UnknownChar;
}

bool Js::CharClassifier::BigCharIsWhitespaceES6(codepoint_t ch, const CharClassifier *instance)
{
    Assert(ch > 0x7F);

    if (ch == 0xFEFF)
    {
        return true;
    }

    return PlatformAgnostic::UnicodeText::IsWhitespace(ch);
}

bool Js::CharClassifier::BigCharIsIdStartES6(codepoint_t codePoint, const CharClassifier *instance)
{
    Assert(codePoint > 0x7F);

    return PlatformAgnostic::UnicodeText::IsIdStart(codePoint);
}

bool Js::CharClassifier::BigCharIsIdContinueES6(codepoint_t codePoint, const CharClassifier *instance)
{
    Assert(codePoint > 0x7F);

    if (codePoint == '$' || codePoint == '_' || codePoint == 0x200C /* Zero-width non-joiner */ || codePoint == 0x200D /* Zero-width joiner */)
    {
        return true;
    }

    return PlatformAgnostic::UnicodeText::IsIdContinue(codePoint);
}

template <bool isBigChar>
bool Js::CharClassifier::IsWhiteSpaceFast(codepoint_t ch) const
{
    using namespace PlatformAgnostic::UnicodeText;
    Assert(isBigChar ? ch > 0x7F : ch < 0x80);
    return isBigChar ? this->bigCharIsWhitespaceFunc(ch, this) : (charFlags[ch] & CharacterTypeFlags::SpaceChar) != 0;
}

bool Js::CharClassifier::IsBiDirectionalChar(codepoint_t ch) const
{
    //From http://www.unicode.org/reports/tr9/#Directional_Formatting_Codes
    switch (ch)
    {
    case 0x202A: //LEFT-TO-RIGHT EMBEDDING Treat the following text as embedded left-to-right
    case 0x202B: //RIGHT-TO-LEFT EMBEDDING Treat the following text as embedded right-to-left.
    case 0x202D: //LEFT-TO-RIGHT OVERRIDE Force following characters to be treated as strong left-to-right characters.
    case 0x202E: //RIGHT-TO-LEFT OVERRIDE Force following characters to be treated as strong right-to-left characters.
    case 0x202C: //POP DIRECTIONAL FORMATTING End the scope of the last LRE, RLE, RLO, or LRO.
    case 0x2066: //LEFT-TO-RIGHT ISOLATE Treat the following text as isolated and left-to-right.
    case 0x2067: //RIGHT-TO-LEFT ISOLATE Treat the following text as isolated and right-to-left.
    case 0x2068: //FIRST STRONG ISOLATE Treat the following text as isolated and in the direction of its first strong directional character that is not inside a nested isolate.
    case 0x2069: //POP DIRECTIONAL ISOLATE End the scope of the last LRI, RLI, or FSI.
    case 0x200E: //LEFT-TO-RIGHT MARK Left-to-right zero-width character
    case 0x200F: //RIGHT-TO-LEFT MARK Right-to-left zero-width non-Arabic character
    case 0x061C: //ARABIC LETTER MARK Right-to-left zero-width Arabic character
        return true;
    default:
        return false;
    }
}

template<bool isBigChar>
bool Js::CharClassifier::IsIdStartFast(codepoint_t ch) const
{
    using namespace PlatformAgnostic::UnicodeText;
    Assert(isBigChar ? ch > 0x7F : ch < 0x80);
    return isBigChar ? this->bigCharIsIdStartFunc(ch, this) : (charFlags[ch] & CharacterTypeFlags::IdLeadChar) != 0;
}
template<bool isBigChar>
bool Js::CharClassifier::IsIdContinueFast(codepoint_t ch) const
{
    using namespace PlatformAgnostic::UnicodeText;
    Assert(isBigChar ? ch > 0x7F : ch < 0x80);
    return isBigChar ? this->bigCharIsIdContinueFunc(ch, this) : (charFlags[ch] & CharacterTypeFlags::IdChar) != 0;
}

Js::CharClassifier::CharClassifier(ScriptContext * scriptContext)
{
    bool isES6UnicodeModeEnabled = CONFIG_FLAG(ES6Unicode);
    bool isFullUnicodeSupportAvailable = PlatformAgnostic::UnicodeText::IsExternalUnicodeLibraryAvailable();

#ifdef NTBUILD
    AssertMsg(isFullUnicodeSupportAvailable, "Windows.Globalization needs to present with IUnicodeCharacterStatics support for Chakra.dll to work");
    if (!isFullUnicodeSupportAvailable)
    {
        Js::Throw::FatalInternalError();
    }
#endif

    // If we're in ES6 mode, and we have full support for Unicode character classification
    // from an external library, then use the ES6/Surrogate pair supported versions of the functions
    // Otherwise, fallback to the ES5 versions which don't need an external library
#if ENABLE_UNICODE_API
    if (isES6UnicodeModeEnabled && isFullUnicodeSupportAvailable)
#endif
    {
        bigCharIsIdStartFunc = &CharClassifier::BigCharIsIdStartES6;
        bigCharIsIdContinueFunc = &CharClassifier::BigCharIsIdContinueES6;
        bigCharIsWhitespaceFunc = &CharClassifier::BigCharIsWhitespaceES6;
        skipWhiteSpaceFunc = &CharClassifier::SkipWhiteSpaceSurrogate;
        skipWhiteSpaceStartEndFunc = &CharClassifier::SkipWhiteSpaceSurrogateStartEnd;
        skipIdentifierFunc = &CharClassifier::SkipIdentifierSurrogate;
        skipIdentifierStartEndFunc = &CharClassifier::SkipIdentifierSurrogateStartEnd;
        getBigCharTypeFunc = &CharClassifier::GetBigCharTypeES6;
        getBigCharFlagsFunc = &CharClassifier::GetBigCharFlagsES6;
    }
#if ENABLE_UNICODE_API
    else
    {
        bigCharIsIdStartFunc = &CharClassifier::BigCharIsIdStartDefault;
        bigCharIsIdContinueFunc = &CharClassifier::BigCharIsIdContinueDefault;
        bigCharIsWhitespaceFunc = &CharClassifier::BigCharIsWhitespaceDefault;
        skipWhiteSpaceFunc = &CharClassifier::SkipWhiteSpaceNonSurrogate;
        skipWhiteSpaceStartEndFunc = &CharClassifier::SkipWhiteSpaceNonSurrogateStartEnd;
        skipIdentifierFunc = &CharClassifier::SkipIdentifierNonSurrogate;
        skipIdentifierStartEndFunc = &CharClassifier::SkipIdentifierNonSurrogateStartEnd;
        getBigCharTypeFunc = &CharClassifier::GetBigCharTypeES5;
        getBigCharFlagsFunc = &CharClassifier::GetBigCharFlagsES5;
    }
#endif

}

const OLECHAR* Js::CharClassifier::SkipWhiteSpaceNonSurrogate(LPCOLESTR psz, const CharClassifier *instance)
{
    for ( ; instance->IsWhiteSpace(*psz); psz++)
    {
    }
    return psz;
}

const OLECHAR* Js::CharClassifier::SkipWhiteSpaceNonSurrogateStartEnd(_In_reads_(pStrEnd - pStr) LPCOLESTR pStr, _In_ LPCOLESTR pStrEnd, const CharClassifier *instance)
{
    for ( ; instance->IsWhiteSpace(*pStr) && pStr < pStrEnd; pStr++)
    {
    }
    return pStr;
}

const OLECHAR* Js::CharClassifier::SkipIdentifierNonSurrogate(LPCOLESTR psz, const CharClassifier *instance)
{
    if (!instance->IsIdStart(*psz))
    {
        return psz;
    }

    for (psz++; instance->IsIdContinue(*psz); psz++)
    {
    }

    return psz;
}

const LPCUTF8 Js::CharClassifier::SkipIdentifierNonSurrogateStartEnd(LPCUTF8 psz, LPCUTF8 end, const CharClassifier *instance)
{
    utf8::DecodeOptions options = utf8::doAllowThreeByteSurrogates;

    LPCUTF8 p = psz;

    if (!instance->IsIdStart(utf8::Decode(p, end, options)))
    {
        return psz;
    }

    psz = p;

    while (instance->IsIdContinue(utf8::Decode(p, end, options)))
    {
        psz = p;
    }

    return psz;
}

const OLECHAR* Js::CharClassifier::SkipWhiteSpaceSurrogate(LPCOLESTR psz, const CharClassifier *instance)
{
    char16 currentChar = 0x0;

    // Slow path is to check for a surrogate each iteration.
    // There is no new surrogate whitespaces as of yet, however, might be in the future, so surrogates still need to be checked
    // So, based on that, best way is to hit the slow path if the current character is not a whitespace in [0, FFFF];
    while((currentChar = *psz) != '\0')
    {
        if (!instance->IsWhiteSpace(*psz))
        {
            if (Js::NumberUtilities::IsSurrogateLowerPart(currentChar) && Js::NumberUtilities::IsSurrogateUpperPart(*(psz + 1)))
            {
                if (instance->IsWhiteSpace(Js::NumberUtilities::SurrogatePairAsCodePoint(currentChar, *(psz + 1))))
                {
                    psz += 2;
                    continue;
                }
            }

            // Above case failed, so we have reached the last whitespace
            return psz;
        }

        psz++;
    }

    return psz;
}

const OLECHAR* Js::CharClassifier::SkipWhiteSpaceSurrogateStartEnd(_In_reads_(pStrEnd - pStr) LPCOLESTR pStr, _In_ LPCOLESTR pStrEnd, const CharClassifier *instance)
{
    char16 currentChar = 0x0;

    // Same reasoning as above
    while(pStr < pStrEnd && (currentChar = *pStr) != '\0')
    {
        if (!instance->IsWhiteSpace(currentChar))
        {
            if (Js::NumberUtilities::IsSurrogateLowerPart(currentChar) && (pStr + 1) < pStrEnd && Js::NumberUtilities::IsSurrogateUpperPart(*(pStr + 1)))
            {
                if (instance->IsWhiteSpace(Js::NumberUtilities::SurrogatePairAsCodePoint(currentChar, *(pStr + 1))))
                {
                    pStr += 2;
                    continue;
                }
            }

            // Above case failed, so we have reached the last whitespace
            return pStr;
        }

        pStr++;
    }

    return pStr;
}

const OLECHAR* Js::CharClassifier::SkipIdentifierSurrogate(LPCOLESTR psz, const CharClassifier *instance)
{
    // Similar reasoning to above, however we do have surrogate identifiers, but less likely to occur in code.
    char16 currentChar = *psz;

    if (!instance->IsIdStart(currentChar))
    {
        if (Js::NumberUtilities::IsSurrogateLowerPart(currentChar) && Js::NumberUtilities::IsSurrogateUpperPart(*(psz + 1))
            && instance->IsIdStart(Js::NumberUtilities::SurrogatePairAsCodePoint(currentChar, *(psz + 1))))
        {
            // For the extra surrogate char
            psz ++;
        }
        else
        {
            return psz;
        }
    }

    psz++;

    while((currentChar = *psz) != '\0')
    {
        if (!instance->IsIdContinue(*psz))
        {
            if (Js::NumberUtilities::IsSurrogateLowerPart(currentChar) && Js::NumberUtilities::IsSurrogateUpperPart(*(psz + 1)))
            {
                if (instance->IsIdContinue(Js::NumberUtilities::SurrogatePairAsCodePoint(currentChar, *(psz + 1))))
                {
                    psz += 2;
                    continue;
                }
            }

            // Above case failed, so we have reached the last IDContinue
            return psz;
        }

        psz++;
    }

    return psz;
}

const LPCUTF8 Js::CharClassifier::SkipIdentifierSurrogateStartEnd(LPCUTF8 psz, LPCUTF8 end, const CharClassifier *instance)
{

    LPCUTF8 currentPosition = psz;
    utf8::DecodeOptions options = utf8::doAllowThreeByteSurrogates;

    // Similar reasoning to above, however we do have surrogate identifiers, but less likely to occur in code.
    codepoint_t currentChar = utf8::Decode(currentPosition, end, options);

    if (options & utf8::doSecondSurrogatePair)
    {
        currentChar = Js::NumberUtilities::SurrogatePairAsCodePoint(currentChar, utf8::Decode(currentPosition, end, options));
    }

    if (!instance->IsIdStart(currentChar))
    {
        return psz;
    }

    psz = currentPosition;

    // Slow path is to check for a surrogate each iteration.
    // There is no new surrogate whitespaces as of yet, however, might be in the future, so surrogates still need to be checked
    // So, based on that, best way is to hit the slow path if the current character is not a whitespace in [0, FFFF];
    while((currentChar = utf8::Decode(currentPosition, end, options)) != '\0')
    {
        if (options & utf8::doSecondSurrogatePair)
        {
            currentChar = Js::NumberUtilities::SurrogatePairAsCodePoint(currentChar, utf8::Decode(currentPosition, end, options));
        }

        if (!instance->IsIdContinue(currentChar))
        {
            return psz;
        }

        psz = currentPosition;
    }

    return psz;
}

CharTypes Js::CharClassifier::GetCharType(codepoint_t ch) const
{
    return FBigChar(ch) ? getBigCharTypeFunc(ch, this) : charTypes[ch];
}

#if ENABLE_UNICODE_API
PlatformAgnostic::UnicodeText::CharacterTypeFlags Js::CharClassifier::GetCharFlags(codepoint_t ch) const
{
#if ENABLE_UNICODE_API
    return FBigChar(ch) ? getBigCharFlagsFunc(ch, this) : PlatformAgnostic::UnicodeText::charFlags[ch];
#else
    return PlatformAgnostics::UnicodeText::charFlags[ch];
#endif
}
#endif

// Explicit instantiation
template bool Js::CharClassifier::IsIdStartFast<true>(codepoint_t) const;
template bool Js::CharClassifier::IsIdStartFast<false>(codepoint_t) const;
template bool Js::CharClassifier::IsIdContinueFast<true>(codepoint_t) const;
template bool Js::CharClassifier::IsIdContinueFast<false>(codepoint_t) const;
template bool Js::CharClassifier::IsWhiteSpaceFast<true>(codepoint_t) const;
template bool Js::CharClassifier::IsWhiteSpaceFast<false>(codepoint_t) const;
