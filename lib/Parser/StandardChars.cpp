//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

namespace UnifiedRegex
{

    // ----------------------------------------------------------------------
    // ASCIIChars
    // ----------------------------------------------------------------------

/*
To get these two tables run:
  ch.exe ascii.js
where ascii.js is:
----------------------------------------------------------------------
function echo(s) { WScript.Echo(s); }

var NumChars = 1 << 8;

var Word = 1 << 0;
var Newline = 1 << 1;
var Whitespace = 1 << 2;
var Letter     = 1 << 3;
var Digit      = 1 << 4;
var Octal      = 1 << 5;
var Hex        = 1 << 6;

var classes = [];
var values = [];

function cc(s) {
    return s.charCodeAt(0);
}

var c;
for (c = 0; c < NumChars; c++)
{
    classes[c] = 0;
    values[c] = 0;
}
for (c = cc('0'); c <= cc('7'); c++)
{
    classes[c] |= Word | Octal | Digit | Hex;
    values[c] = c - cc('0');
}
for (c = cc('8'); c <= cc('9'); c++)
{
    classes[c] |= Word | Digit | Hex;
    values[c] = c - cc('0');
}
for (c = cc('a'); c <= cc('f'); c++)
{
    classes[c] |= Word | Hex | Letter;
    values[c] = 10 + c - cc('a');
}
for (c = cc('g'); c <= cc('z'); c++)
    classes[c] |= Word | Letter;
for (c = cc('A'); c <= cc('F'); c++)
{
    classes[c] |= Word | Hex | Letter;
    values[c] = 10 + c - cc('A');
}
for (c = cc('G'); c <= cc('Z'); c++)
    classes[c] |= Word | Letter;
classes[cc('_')] |= Word;

classes[cc('\n')] |= Newline;
classes[cc('\r')] |= Newline;

for (c = cc('\t'); c <= cc('\r'); c++)
    classes[c] |= Whitespace;
classes[cc(' ')] |= Whitespace;
classes[cc('\x85')] |= Whitespace;
classes[cc('\xa0')] |= Whitespace;

hex = "0123456789abcdef";
function toHex(n) {
    return "0x" + hex[n >> 4] + hex[n & 0xf];
}

function dump(a) {
    for (c = 0; c < NumChars; c++) {
        if (c % 16 == 0)
            str = "        ";
        else
            str += ", ";
        str += toHex(a[c]);
        if (c % 16 == 15)
        {
            if (c < NumChars - 1)
                str += ",";
            echo(str);
        }
    }
}

echo("    const uint8 ASCIIChars::classes[] = {");
dump(classes);
echo("    };");
echo("    const uint8 ASCIIChars::values[] = {");
dump(values);
echo("    };");
----------------------------------------------------------------------
*/

    // Character classes represented as a bit vector for each character.
    const uint8 ASCIIChars::classes[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x06, 0x04, 0x04, 0x06, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x71, 0x71, 0x71, 0x71, 0x71, 0x71, 0x71, 0x71, 0x51, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    // Numeric values of ASCII characters interpreted as hex digits (applies to [0-9a-fA-F], all others are 0x00).
    const uint8 ASCIIChars::values[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    // ----------------------------------------------------------------------
    // TrivialCaseMapper
    // ----------------------------------------------------------------------

    const TrivialCaseMapper TrivialCaseMapper::Instance;

    // ----------------------------------------------------------------------
    // StandardChars<char16>
    // ----------------------------------------------------------------------

/*
To get the whitespaces string, run:
  gawk -f spaces.gawk http://www.unicode.org/Public/9.0.0/ucd/UnicodeData.txt
where spaces.gawk is
----------------------------------------------------------------------
BEGIN {
  FS = ";";
  start = -1;
  last = -1;
  str = "";
}
{
  code = strtonum("0x" $1);
  if ($3 == "Zs" || code == 0x0009 || code == 0x000B || code == 0x000C || code == 0x0020 || code == 0x00A0 || code == 0xFEFF || code == 0x000A || code == 0x000D || code == 0x2028 || code == 0x2029)
  {
    if (start < 0)
      start = code;
    else if (code > last + 1) {
      str = sprintf("%s\\x%04x\\x%04x", str, start, last);
      start = code;
    }
    last = code;
  }
}
END {
  str = sprintf("%s\\x%04x\\x%04x", str, start, last);
  print str;
}
----------------------------------------------------------------------
*/

    const int StandardChars<char16>::numDigitPairs = 1;
    const char16* const StandardChars<char16>::digitStr = _u("09");
    const int StandardChars<char16>::numWhitespacePairs = 10;
    const char16* const StandardChars<char16>::whitespaceStr = _u("\x0009\x000d\x0020\x0020\x00a0\x00a0\x1680\x1680\x2000\x200a\x2028\x2029\x202f\x202f\x205f\x205f\x3000\x3000\xfeff\xfeff");
    const int StandardChars<char16>::numWordPairs = 4;
    const char16* const StandardChars<char16>::wordStr = _u("09AZ__az");
    const int StandardChars<char16>::numWordIUPairs = 6; // Under /iu flags, Sharp S and Kelvin sign map to S and K, respectively.
    const char16* const StandardChars<char16>::wordIUStr = _u("09AZ__az\x017F\x017F\x212A\x212A");
    const int StandardChars<char16>::numNewlinePairs = 3;
    const char16* const StandardChars<char16>::newlineStr = _u("\x000a\x000a\x000d\x000d\x2028\x2029");

    StandardChars<char16>::StandardChars(ArenaAllocator* allocator)
        : allocator(allocator)
        , unicodeDataCaseMapper(allocator, CaseInsensitive::MappingSource::UnicodeData, &TrivialCaseMapper::Instance)
        , caseFoldingCaseMapper(allocator, CaseInsensitive::MappingSource::CaseFolding, &unicodeDataCaseMapper)
        , fullSet(0)
        , emptySet(0)
        , wordSet(0)
        , nonWordSet(0)
        , newlineSet(0)
        , whitespaceSet(0)
    {
    }

    void StandardChars<char16>::SetDigits(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetRanges(setAllocator, numDigitPairs, digitStr);
    }

    void StandardChars<char16>::SetNonDigits(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetNotRanges(setAllocator, numDigitPairs, digitStr);
    }

    void StandardChars<char16>::SetWhitespace(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetRanges(setAllocator, numWhitespacePairs, whitespaceStr);
    }

    void StandardChars<char16>::SetNonWhitespace(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetNotRanges(setAllocator, numWhitespacePairs, whitespaceStr);
    }

    void StandardChars<char16>::SetWordChars(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetRanges(setAllocator, numWordPairs, wordStr);
    }

    void StandardChars<char16>::SetNonWordChars(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetNotRanges(setAllocator, numWordPairs, wordStr);
    }

    void StandardChars<char16>::SetWordIUChars(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetRanges(setAllocator, numWordIUPairs, wordIUStr);
    }

    void StandardChars<char16>::SetNonWordIUChars(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetNotRanges(setAllocator, numWordIUPairs, wordIUStr);
    }

    void StandardChars<char16>::SetNewline(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetRanges(setAllocator, numNewlinePairs, newlineStr);
    }

    void StandardChars<char16>::SetNonNewline(ArenaAllocator* setAllocator, CharSet<Char> &set)
    {
        set.SetNotRanges(setAllocator, numNewlinePairs, newlineStr);
    }

    CharSet<char16>* StandardChars<char16>::GetFullSet()
    {
        if (fullSet == 0)
        {
            fullSet = Anew(allocator, UnicodeCharSet);
            fullSet->SetRange(allocator, MinChar, MaxChar);
        }
        return fullSet;
    }

    CharSet<char16>* StandardChars<char16>::GetEmptySet()
    {
        if (emptySet == 0)
        {
            emptySet = Anew(allocator, UnicodeCharSet);
            // leave empty
        }
        return emptySet;
    }

    CharSet<char16>* StandardChars<char16>::GetWordSet()
    {
        if (wordSet == 0)
        {
            wordSet = Anew(allocator, UnicodeCharSet);
            wordSet->SetRanges(allocator, numWordPairs, wordStr);
        }
        return wordSet;
    }

    CharSet<char16>* StandardChars<char16>::GetNonWordSet()
    {
        if (nonWordSet == 0)
        {
            nonWordSet = Anew(allocator, UnicodeCharSet);
            nonWordSet->SetNotRanges(allocator, numWordPairs, wordStr);
        }
        return nonWordSet;
    }

    CharSet<char16>* StandardChars<char16>::GetNewlineSet()
    {
        if (newlineSet == 0)
        {
            newlineSet = Anew(allocator, UnicodeCharSet);
            newlineSet->SetRanges(allocator, numNewlinePairs, newlineStr);
        }
        return newlineSet;
    }

    CharSet<char16>* StandardChars<char16>::GetWhitespaceSet()
    {
        if (whitespaceSet == 0)
        {
            whitespaceSet = Anew(allocator, UnicodeCharSet);
            whitespaceSet->SetRanges(allocator, numWhitespacePairs, whitespaceStr);
        }
        return whitespaceSet;
    }
    CharSet<char16>* StandardChars<char16>::GetSurrogateUpperRange()
    {
        if (surrogateUpperRange == 0)
        {
            surrogateUpperRange = Anew(allocator, UnicodeCharSet);
            surrogateUpperRange->SetRange(allocator, (char16)0xDC00u, (char16)0xDFFFu);
        }
        return surrogateUpperRange;
    }
}
