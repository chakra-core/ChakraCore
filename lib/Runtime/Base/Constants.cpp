//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

using namespace Js;

// The VS2013 linker treats this as a redefinition of an already
// defined constant and complains. So skip the declaration if we're compiling
// with VS2013 or below.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const uint Constants::InvalidSourceIndex;
const RegSlot Constants::NoRegister;
#endif

const char16 Constants::AnonymousFunction[] = _u("Anonymous function");
const char16 Constants::Anonymous[] = _u("anonymous");
const char16 Constants::Empty[] = _u("");
const char16 Constants::FunctionCode[] = _u("Function code");
const char16 Constants::GlobalCode[] = _u("Global code");
const char16 Constants::EvalCode[] = _u("eval code");
const char16 Constants::GlobalFunction[] = _u("glo");
const char16 Constants::UnknownScriptCode[] = _u("Unknown script code");
const char16 Constants::StringReplace[] = _u("String.prototype.replace");
const char16 Constants::StringMatch[] = _u("String.prototype.match");

const uint64 Constants::ExponentMask = 0x3FF0000000000000;
const uint64 Constants::MantissaMask = 0x000FFFFFFFFFFFFF;

#ifdef _M_AMD64
const size_t Constants::StackLimitForScriptInterrupt = 0x7fffffffffffffff;
#else
const size_t Constants::StackLimitForScriptInterrupt = 0x7fffffff;
#endif

#pragma warning(push)
#pragma warning(disable:4815) // Allow no storage for zero-sized array at end of NullFrameDisplay struct.
const Js::FrameDisplay Js::NullFrameDisplay = 0;
const Js::FrameDisplay Js::StrictNullFrameDisplay(0, true);
#pragma warning(pop)
