//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class NumberConstants : public NumberConstantsBase
    {
    public:
        // Our float tagging scheme relies on NaNs to be of this value - changing the NaN value
        // will break float tagging for x64.
        static const uint64     k_PosInf    = 0x7FF0000000000000ull;
        static const uint64     k_NegInf    = 0xFFF0000000000000ull;
        static const uint64     k_PosMin    = 0x0000000000000001ull;
        static const uint64     k_PosMax    = 0x7FEFFFFFFFFFFFFFull;
        static const uint64     k_NegZero   = 0x8000000000000000ull;
        static const uint64     k_Zero      = 0x0000000000000000ull;
        static const uint64     k_PointFive = 0x3FE0000000000000ull;
        static const uint64     k_NegPointFive = 0xBFE0000000000000ull;
        static const uint64     k_NegOne    = 0xBFF0000000000000ull;
        static const uint64     k_OnePointZero = 0x3FF0000000000000ull;
        // 2^52
        static const uint64     k_TwoToFraction = 0x4330000000000000ull;
        // -2^52
        static const uint64     k_NegTwoToFraction = 0xC330000000000000ull;

        static const uint64     k_TwoTo63 = 0x43e0000000000000ull;
        static const uint64     k_NegTwoTo63 = 0xc3e0000000000000ull;
        static const uint64     k_TwoTo64 = 0x43f0000000000000ull;
        static const uint64     k_TwoTo31 = 0x41dfffffffc00000ull;
        static const uint64     k_NegTwoTo31 = 0xc1e0000000000000ull;
        static const uint64     k_TwoTo32 = 0x41efffffffe00000ull;
        static const uint64     k_MinIntMinusOne = 0xc1e0000000200000ull;
        static const uint64     k_UintMaxPlusOne = 0x41f0000000000000ull;
        static const uint64     k_IntMaxPlusOne = 0x41e0000000000000ull;

        static const uint32     k_Float32Zero      = 0x00000000ul;
        static const uint32     k_Float32PointFive = 0x3F000000ul;
        static const uint32     k_Float32NegPointFive = 0xBF000000ul;
        static const uint32     k_Float32NegZero   = 0x80000000ul;
        // 2^23
        static const uint32     k_Float32TwoToFraction = 0x4B000000ul;
        // -2^23
        static const uint32     k_Float32NegTwoToFraction = 0xCB000000ul;

        static const uint32     k_Float32TwoTo63 = 0x5f000000u;
        static const uint32     k_Float32NegTwoTo63 = 0xdf000000u;
        static const uint32     k_Float32TwoTo64 = 0x5f800000u;
        static const uint32     k_Float32NegOne = 0xbf800000u;
        static const uint32     k_Float32TwoTo31 = 0x4f000000u;
        static const uint32     k_Float32NegTwoTo31 = 0xcf000000u;
        static const uint32     k_Float32TwoTo32 = 0x4f800000u;

        static const double     MAX_VALUE;
        static const double     MIN_VALUE;
        static const double     NaN;
        static const double     NEGATIVE_INFINITY;
        static const double     POSITIVE_INFINITY;
        static const double     NEG_ZERO;
        static const double     ONE_POINT_ZERO;
        static const double     DOUBLE_INT_MIN;
        static const double     DOUBLE_TWO_TO_31;

        static const BYTE AbsDoubleCst[];
        static const BYTE AbsFloatCst[];
        static const BYTE SgnFloatBitCst[];
        static const BYTE SgnDoubleBitCst[];
        static double const UIntConvertConst[];

        static double const MaskNegDouble[];
        static float const MaskNegFloat[];

    };

    class NumberUtilities : public NumberUtilitiesBase
    {
    public:
        static bool IsDigit(int ch);
        static BOOL FHexDigit(char16 ch, int *pw);
        static uint32 MulLu(uint32 lu1, uint32 lu2, uint32 *pluHi);
        static int AddLu(uint32 *plu1, uint32 lu2);

        static uint32 &LuHiDbl(double &dbl);
        static uint32 &LuLoDbl(double &dbl);
        static INT64 TryToInt64(double d);
        static bool IsValidTryToInt64(__int64 value);   // Whether TryToInt64 resulted in a valid value.

        static int CbitZeroLeft(uint32 lu);

        static bool IsFinite(double value);
        static bool IsNan(double value);
        static bool IsFloat32NegZero(float value);
        static bool IsSpecial(double value, uint64 nSpecial);
        static uint64 ToSpecial(double value);
        static uint32 ToSpecial(float value);
        static float VECTORCALL ReinterpretBits(int value);
        static double VECTORCALL ReinterpretBits(int64 value);

        // Convert a given UINT16 into its corresponding string.
        // outBufferSize is in WCHAR elements (and used only for ASSERTs)
        // Returns the number of characters written to outBuffer (not including the \0)
        static charcount_t UInt16ToString(uint16 integer, __out __ecount(outBufferSize) WCHAR* outBuffer, charcount_t outBufferSize, char widthForPaddingZerosInsteadSpaces);

        // Try to parse an integer string to find out if the string contains an index property name.
        static BOOL TryConvertToUInt32(const char16* str, int length, uint32* intVal);

        static double Modulus(double dblLeft, double dblRight);

        enum FormatType
        {
            FormatFixed,
            FormatExponential,
            FormatPrecision
        };

        // Implemented in lib\parser\common.  Should move to lib\common
        template<typename EncodedChar>
        static double StrToDbl(const EncodedChar *psz, const EncodedChar **ppchLim, bool& likelyInt);

        static BOOL FDblToStr(double dbl, __out_ecount(nDstBufSize) char16 *psz, int nDstBufSize);
        static int FDblToStr(double dbl, NumberUtilities::FormatType ft, int nDigits, __out_ecount(cchDst) char16 *pchDst, int cchDst);

        static BOOL FNonZeroFiniteDblToStr(double dbl, _Out_writes_(nDstBufSize) WCHAR* psz, int nDstBufSize);
        _Success_(return) static BOOL FNonZeroFiniteDblToStr(double dbl, _In_range_(2, 36) int radix, _Out_writes_(nDstBufSize) WCHAR* psz, int nDstBufSize);

        static double DblFromDecimal(DECIMAL * pdecIn);

        static void CodePointAsSurrogatePair(codepoint_t codePointValue, __out char16* first, __out char16* second);
        static codepoint_t SurrogatePairAsCodePoint(codepoint_t first, codepoint_t second);

        static bool IsSurrogateUpperPart(codepoint_t codePointValue);
        static bool IsSurrogateLowerPart(codepoint_t codePointValue);

        static bool IsInSupplementaryPlane(codepoint_t codePointValue);

        static int32 LwFromDblNearest(double dbl);
        static uint32 LuFromDblNearest(double dbl);
        static BOOL FDblIsInt32(double dbl, int32 *plw);

        template<typename EncodedChar>
        static double DblFromHex(const EncodedChar *psz, const EncodedChar **ppchLim);
        template <typename EncodedChar>
        static double DblFromBinary(const EncodedChar *psz, const EncodedChar **ppchLim);
        template<typename EncodedChar>
        static double DblFromOctal(const EncodedChar *psz, const EncodedChar **ppchLim);
        template<typename EncodedChar>
        static double StrToDbl(const EncodedChar *psz, const EncodedChar **ppchLim, Js::ScriptContext *const scriptContext);

        const NumberUtilitiesBase* GetNumberUtilitiesBase() const { return static_cast<const NumberUtilitiesBase*>(this); }

    };
}
