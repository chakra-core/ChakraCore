//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"

#ifdef _WIN32
// Windows Specific implementation
#include "UnicodeText.h"
#include <windows.h>
#include "Runtime.h"
#include "Base/ThreadContext.h"
#ifdef NTBUILD
#include "Windows.Globalization.h"
#else
#include "Windows.Data.Text.h"
using namespace ABI;
#endif

namespace PlatformAgnostic
{
    namespace UnicodeText
    {
        using namespace Windows::Data::Text;

        // Legacy Win32 Unicode Character Classification helpers for compat
        // Used to be in CharClassifier.cpp
        typedef struct
        {
            OLECHAR chStart;
            OLECHAR chFinish;

        } oldCharTypesRangeStruct;

        static const int cOldDigits = 156;
        static const oldCharTypesRangeStruct oldDigits[] = {
            {   688,   734 }, {   736,   745 }, {   768,   837 }, {   864,   865 }, {   884,   885 },
            {   890,   890 }, {   900,   901 }, {  1154,  1158 }, {  1369,  1369 }, {  1425,  1441 },
            {  1443,  1465 }, {  1467,  1469 }, {  1471,  1471 }, {  1473,  1474 }, {  1476,  1476 },
            {  1600,  1600 }, {  1611,  1618 }, {  1648,  1648 }, {  1750,  1773 }, {  2305,  2307 },
            {  2364,  2381 }, {  2384,  2388 }, {  2402,  2403 }, {  2433,  2435 }, {  2492,  2492 },
            {  2494,  2500 }, {  2503,  2504 }, {  2507,  2509 }, {  2519,  2519 }, {  2530,  2531 },
            {  2546,  2554 }, {  2562,  2562 }, {  2620,  2620 }, {  2622,  2626 }, {  2631,  2632 },
            {  2635,  2637 }, {  2672,  2676 }, {  2689,  2691 }, {  2748,  2757 }, {  2759,  2761 },
            {  2763,  2765 }, {  2768,  2768 }, {  2817,  2819 }, {  2876,  2883 }, {  2887,  2888 },
            {  2891,  2893 }, {  2902,  2903 }, {  2928,  2928 }, {  2946,  2947 }, {  3006,  3010 },
            {  3014,  3016 }, {  3018,  3021 }, {  3031,  3031 }, {  3056,  3058 }, {  3073,  3075 },
            {  3134,  3140 }, {  3142,  3144 }, {  3146,  3149 }, {  3157,  3158 }, {  3202,  3203 },
            {  3262,  3268 }, {  3270,  3272 }, {  3274,  3277 }, {  3285,  3286 }, {  3330,  3331 },
            {  3390,  3395 }, {  3398,  3400 }, {  3402,  3405 }, {  3415,  3415 }, {  3647,  3647 },
            {  3759,  3769 }, {  3771,  3773 }, {  3776,  3780 }, {  3782,  3782 }, {  3784,  3789 },
            {  3840,  3843 }, {  3859,  3871 }, {  3882,  3897 }, {  3902,  3903 }, {  3953,  3972 },
            {  3974,  3979 }, {  8125,  8129 }, {  8141,  8143 }, {  8157,  8159 }, {  8173,  8175 },
            {  8189,  8190 }, {  8192,  8207 }, {  8232,  8238 }, {  8260,  8260 }, {  8298,  8304 },
            {  8308,  8316 }, {  8319,  8332 }, {  8352,  8364 }, {  8400,  8417 }, {  8448,  8504 },
            {  8531,  8578 }, {  8592,  8682 }, {  8704,  8945 }, {  8960,  8960 }, {  8962,  9000 },
            {  9003,  9082 }, {  9216,  9252 }, {  9280,  9290 }, {  9312,  9371 }, {  9450,  9450 },
            {  9472,  9621 }, {  9632,  9711 }, {  9728,  9747 }, {  9754,  9839 }, {  9985,  9988 },
            {  9990,  9993 }, {  9996, 10023 }, { 10025, 10059 }, { 10061, 10061 }, { 10063, 10066 },
            { 10070, 10070 }, { 10072, 10078 }, { 10081, 10087 }, { 10102, 10132 }, { 10136, 10159 },
            { 10161, 10174 }, { 12292, 12292 }, { 12294, 12294 }, { 12306, 12307 }, { 12320, 12335 },
            { 12337, 12343 }, { 12351, 12351 }, { 12441, 12442 }, { 12688, 12703 }, { 12800, 12828 },
            { 12832, 12867 }, { 12896, 12923 }, { 12927, 12976 }, { 12992, 13003 }, { 13008, 13054 },
            { 13056, 13174 }, { 13179, 13277 }, { 13280, 13310 }, { 64286, 64286 }, { 65056, 65059 },
            { 65122, 65122 }, { 65124, 65126 }, { 65129, 65129 }, { 65136, 65138 }, { 65140, 65140 },
            { 65142, 65151 }, { 65284, 65284 }, { 65291, 65291 }, { 65308, 65310 }, { 65342, 65342 },
            { 65344, 65344 }, { 65372, 65372 }, { 65374, 65374 }, { 65440, 65440 }, { 65504, 65510 },
            { 65512, 65518 }
        };

        static const int cOldAlphas = 11;
        static const oldCharTypesRangeStruct oldAlphas[] = {
            {   402,   402 }, {  9372,  9449 }, { 12293, 12293 }, { 12295, 12295 }, { 12443, 12446 },
            { 12540, 12542 }, { 64297, 64297 }, { 65152, 65276 }, { 65392, 65392 }, { 65438, 65439 },
            { 65533, 65533 }
        };

        static BOOL doBinSearch(OLECHAR ch, const oldCharTypesRangeStruct *pRanges, int cSize)
        {
            int lo = 0;
            int hi = cSize;
            int mid;

            while (lo != hi)
            {
                mid = lo + (hi - lo) / 2;

                if (pRanges[mid].chStart <= ch && ch <= pRanges[mid].chFinish)
                {
                    return true;
                }

                if (ch < pRanges[mid].chStart)
                {
                    hi = mid;
                }
                else
                {
                    lo = mid + 1;
                }
            }

            return false;
        }

        static WORD oFindOldCharType(OLECHAR ch)
        {
            if ((OLECHAR)65279 == ch)
            {
                return C1_SPACE;
            }

            if (doBinSearch(ch, oldAlphas, cOldAlphas))
            {
                return C1_ALPHA;
            }

            if (doBinSearch(ch, oldDigits, cOldDigits))
            {
                return C1_DIGIT;
            }

            return 0;
        }

        // Helper wrapper methods
        template <typename TRet, typename Fn>
        static TRet ExecuteWithThreadContext(Fn fn, TRet defaultValue)
        {
            // TODO: We should remove the depedency on ThreadContext for this layer
            // Currently, this exists since Windows.Globalization.dll is delay loaded and the
            // handle is stored on the thread context. We should move the management of the
            // lifetime of that DLL to this layer, so that we can move the PAL out of Runtime
            // into Common.
            ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
            AssertMsg(threadContext, "Method should not be called after thread context is freed");

            if (threadContext != nullptr)
            {
                return fn(threadContext);
            }

            return defaultValue;
        }

#ifdef INTL_WINGLOB
        template <typename Fn, typename TDefaultValue>
        static TDefaultValue ExecuteWinGlobApi(Fn fn, TDefaultValue defaultValue)
        {
            return ExecuteWithThreadContext([&](ThreadContext* threadContext)
            {
                Js::WindowsGlobalizationAdapter* globalizationAdapter = threadContext->GetWindowsGlobalizationAdapter();
                Assert(globalizationAdapter != nullptr);
                IUnicodeCharactersStatics* pCharStatics = globalizationAdapter->GetUnicodeStatics();
                Assert(pCharStatics != nullptr);
                if (pCharStatics)
                {
                    return fn(pCharStatics);
                }

                // REVIEW: Alternatively, we could fail-fast here?
                return defaultValue;
            }, defaultValue);
        }

        template <typename Fn>
        static bool ExecuteWinGlobCodepointCheckApi(codepoint_t codepoint, Fn fn)
        {
            return ExecuteWinGlobApi([&](IUnicodeCharactersStatics* pUnicodeCharStatics) {
                boolean returnValue = false;
                HRESULT hr = (pUnicodeCharStatics->*fn)(codepoint, &returnValue);
                Assert(SUCCEEDED(hr));
                return (returnValue != 0);
            }, false);
        }
#endif

        // Helper Win32 conversion utilities
        static NORM_FORM TranslateToWin32NormForm(NormalizationForm normalizationForm)
        {
            // NormalizationForm is equivalent to NORM_FORM
            // The following statements assert this
            CompileAssert(NormalizationForm::C == NORM_FORM::NormalizationC);
            CompileAssert(NormalizationForm::D == NORM_FORM::NormalizationD);
            CompileAssert(NormalizationForm::KC == NORM_FORM::NormalizationKC);
            CompileAssert(NormalizationForm::KD == NORM_FORM::NormalizationKD);

            // Assert for NORM_FORM Other even though we don't accept it as a valid parameter at this point
            CompileAssert(NormalizationForm::Other == NORM_FORM::NormalizationOther);

            Assert(
                normalizationForm == NormalizationForm::C ||
                normalizationForm == NormalizationForm::D ||
                normalizationForm == NormalizationForm::KC ||
                normalizationForm == NormalizationForm::KD
                );
            return (NORM_FORM)normalizationForm;
        }

        static ApiError TranslateWin32Error(DWORD win32Error)
        {
            switch (win32Error)
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    return ApiError::InsufficientBuffer;
                case ERROR_INVALID_PARAMETER:
                    return ApiError::InvalidParameter;
                case ERROR_NO_UNICODE_TRANSLATION:
                    return ApiError::InvalidUnicodeText;
                case ERROR_SUCCESS:
                    return ApiError::NoError;
                default:
                    return ApiError::UntranslatedError;
            }
        }

        int32 NormalizeString(NormalizationForm normalizationForm, const char16* sourceString, uint32 sourceLength, char16* destString, int32 destLength, ApiError* pErrorOut)
        {
            // Assert pointers
            Assert(sourceString != nullptr);
            Assert(destString != nullptr || destLength == 0);

            // This is semantically different than the Win32 NormalizeString API
            // For our usage, we always pass in the length rather than letting Windows
            // calculate the length for us
            Assert(sourceLength > 0);
            Assert(destLength >= 0);

            *pErrorOut = NoError;

            // Initialize the error field on the TLS since we check it later
            ::SetLastError(ERROR_SUCCESS);

            int normalizedStringLength = ::NormalizeString(TranslateToWin32NormForm(normalizationForm), (LPCWSTR)sourceString, sourceLength, (LPWSTR)destString, destLength);

            if (destLength == 0 && normalizedStringLength >= 0)
            {
                *pErrorOut = ApiError::InsufficientBuffer;
                return normalizedStringLength;
            }

            if (normalizedStringLength <= 0)
            {
                DWORD win32Error = ::GetLastError();
                *pErrorOut = TranslateWin32Error(win32Error);
            }

            return normalizedStringLength;
        }

        bool IsNormalizedString(NormalizationForm normalizationForm, const char16* testString, int32 testStringLength)
        {
            Assert(testString != nullptr);

            return (::IsNormalizedString(TranslateToWin32NormForm(normalizationForm), (LPCWSTR)testString, testStringLength) == TRUE);
        }

        int32 ChangeStringLinguisticCase(CaseFlags caseFlags, const char16* sourceString, uint32 sourceLength, char16* destString, uint32 destLength, ApiError* pErrorOut)
        {
            // Assert pointers
            Assert(sourceString != nullptr);
            Assert(destString != nullptr || destLength == 0);

            // LCMapString does not allow the source length to be set to 0
            Assert(sourceLength > 0);

            *pErrorOut = NoError;

            DWORD dwFlags = caseFlags == CaseFlags::CaseFlagsUpper ? LCMAP_UPPERCASE : LCMAP_LOWERCASE;
            dwFlags |= LCMAP_LINGUISTIC_CASING;

            LCID lcid = GetUserDefaultLCID();

            int translatedStringLength = LCMapStringW(lcid, dwFlags, sourceString, sourceLength, destString, destLength);

            if (translatedStringLength == 0)
            {
                *pErrorOut = TranslateWin32Error(::GetLastError());
            }

            Assert(translatedStringLength >= 0);
            return (uint32) translatedStringLength;
        }

        uint32 ChangeStringCaseInPlace(CaseFlags caseFlags, char16* sourceString, uint32 sourceLength)
        {
            // Assert pointers
            Assert(sourceString != nullptr);

            if (sourceLength == 0 || sourceString == nullptr)
            {
                return 0;
            }

            if (caseFlags == CaseFlagsUpper)
            {
                return (uint32) CharUpperBuff(sourceString, sourceLength);
            }
            else if (caseFlags == CaseFlagsLower)
            {
                return (uint32) CharLowerBuff(sourceString, sourceLength);
            }

            AssertMsg(false, "Invalid flags passed to ChangeStringCaseInPlace");
            return 0;
        }

        UnicodeGeneralCategoryClass GetGeneralCategoryClass(codepoint_t codepoint)
        {
#ifdef INTL_WINGLOB
            return ExecuteWinGlobApi([&](IUnicodeCharactersStatics* pUnicodeCharStatics) {
                UnicodeGeneralCategory category = UnicodeGeneralCategory::UnicodeGeneralCategory_NotAssigned;

                HRESULT hr = pUnicodeCharStatics->GetGeneralCategory(codepoint, &category);
                Assert(SUCCEEDED(hr));
                if (SUCCEEDED(hr))
                {
                    switch (category)
                    {
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_LowercaseLetter:
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_UppercaseLetter:
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_TitlecaseLetter:
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_ModifierLetter:
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_OtherLetter:
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_LetterNumber:
                            return UnicodeGeneralCategoryClass::CategoryClassLetter;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_DecimalDigitNumber:
                            return UnicodeGeneralCategoryClass::CategoryClassDigit;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_LineSeparator:
                            return UnicodeGeneralCategoryClass::CategoryClassLineSeparator;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_ParagraphSeparator:
                            return UnicodeGeneralCategoryClass::CategoryClassParagraphSeparator;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_SpaceSeparator:
                            return UnicodeGeneralCategoryClass::CategoryClassSpaceSeparator;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_SpacingCombiningMark:
                            return UnicodeGeneralCategoryClass::CategoryClassSpacingCombiningMark;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_NonspacingMark:
                            return UnicodeGeneralCategoryClass::CategoryClassNonSpacingMark;
                        case UnicodeGeneralCategory::UnicodeGeneralCategory_ConnectorPunctuation:
                            return UnicodeGeneralCategoryClass::CategoryClassConnectorPunctuation;
                        default:
                            break;
                    }
                }

                return UnicodeGeneralCategoryClass::CategoryClassOther;
            }, UnicodeGeneralCategoryClass::CategoryClassOther);
#else
            // TODO (doilij) implement with ICU
            return UnicodeGeneralCategoryClass::CategoryClassOther;
#endif
        }

        bool IsIdStart(codepoint_t codepoint)
        {
#ifdef INTL_WINGLOB
            return ExecuteWinGlobCodepointCheckApi(codepoint, &IUnicodeCharactersStatics::IsIdStart);
#else
            // TODO (doilij) implement with ICU
            return false;
#endif
        }

        bool IsIdContinue(codepoint_t codepoint)
        {
#ifdef INTL_WINGLOB
            return ExecuteWinGlobCodepointCheckApi(codepoint, &IUnicodeCharactersStatics::IsIdContinue);
#else
            // TODO (doilij) implement with ICU
            return false;
#endif
        }

        bool IsWhitespace(codepoint_t codepoint)
        {
#ifdef INTL_WINGLOB
            return ExecuteWinGlobCodepointCheckApi(codepoint, &IUnicodeCharactersStatics::IsWhitespace);
#else
            // TODO (doilij) implement with ICU
            return false;
#endif
        }

        bool IsExternalUnicodeLibraryAvailable()
        {
#ifdef INTL_WINGLOB
            return ExecuteWithThreadContext([](ThreadContext* threadContext) {
                Js::WindowsGlobalizationAdapter* globalizationAdapter = threadContext->GetWindowsGlobalizationAdapter();
                Js::DelayLoadWindowsGlobalization* globLibrary = threadContext->GetWindowsGlobalizationLibrary();
                HRESULT hr = globalizationAdapter->EnsureDataTextObjectsInitialized(globLibrary);
                // Failed to load windows.globalization.dll or jsintl.dll. No unicodeStatics support
                // in that case.
                if (SUCCEEDED(hr))
                {
                    auto winGlobCharApi = globalizationAdapter->GetUnicodeStatics();
                    if (winGlobCharApi != nullptr)
                    {
                        return true;
                    }
                }

                return false;
            }, false);
#else
            // TODO (doilij) implement with ICU
            return false;
#endif
        }

        int LogicalStringCompare(const char16* string1, const char16* string2)
        {
            // CompareStringW called with these flags is equivalent to calling StrCmpLogicalW
            // and we have the added advantage of not having to link with shlwapi.lib just for one function
            int i = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, string1, -1, string2, -1);

            return i - CSTR_EQUAL;
        }

        // Win32 implementation of platform-agnostic Unicode interface
        // These are the public APIs of this interface
        CharacterClassificationType GetLegacyCharacterClassificationType(char16 ch)
        {
            WORD charType = 0;
            BOOL res = ::GetStringTypeW(CT_CTYPE1, &ch, 1, &charType);

            if (res == TRUE)
            {
                // BOM ( 0xfeff) is recognized as GetStringTypeW as WS.
                if ((0x03FF & charType) == 0x0200)
                {
                    // Some of the char types changed for Whistler (Unicode 3.0).
                    // They will return 0x0200 on Whistler, indicating a defined char
                    // with no type attributes. We want to continue to support these
                    // characters, so we return the Win2K (Unicode 2.1) attributes.
                    // We only return the ones we care about - ALPHA for ALPHA, PUNCT
                    // for PUNCT or DIGIT, and SPACE for SPACE or BLANK.
                    WORD wOldCharType = oFindOldCharType(ch);
                    if (0 == wOldCharType)
                    {
                        return CharacterClassificationType::Invalid;
                    }

                    charType = wOldCharType;
                }

                if (charType & C1_ALPHA)
                {
                    return CharacterClassificationType::Letter;
                }
                else if (charType & (C1_DIGIT | C1_PUNCT))
                {
                    return CharacterClassificationType::DigitOrPunct;
                }
                else if (charType & (C1_SPACE | C1_BLANK))
                {
                    return CharacterClassificationType::Whitespace;
                }
            }

            return CharacterClassificationType::Invalid;
        }
    };
};

#endif
