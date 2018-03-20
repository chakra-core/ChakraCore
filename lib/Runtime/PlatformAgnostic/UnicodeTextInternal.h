//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace PlatformAgnostic
{
namespace UnicodeText
{

// Instantiate templates here rather than in each implementing file
template charcount_t ChangeStringLinguisticCase<true, true>(const char16* sourceString, charcount_t sourceLength, char16* destString, charcount_t destLength, ApiError* pErrorOut);
template charcount_t ChangeStringLinguisticCase<true, false>(const char16* sourceString, charcount_t sourceLength, char16* destString, charcount_t destLength, ApiError* pErrorOut);
template charcount_t ChangeStringLinguisticCase<false, true>(const char16* sourceString, charcount_t sourceLength, char16* destString, charcount_t destLength, ApiError* pErrorOut);
template charcount_t ChangeStringLinguisticCase<false, false>(const char16* sourceString, charcount_t sourceLength, char16* destString, charcount_t destLength, ApiError* pErrorOut);

namespace Internal
{

int LogicalStringCompareImpl(const char16* p1, const char16* p2);

}; // namespace Internal
}; // namespace UnicodeText
}; // namespace PlatformAgnostic
