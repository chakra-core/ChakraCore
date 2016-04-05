//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

using namespace Js;

const char16 Constants::AnonymousFunction[] = _u("Anonymous function");
const char16 Constants::Anonymous[] = _u("anonymous");
const char16 Constants::Empty[] = _u("");
const char16 Constants::FunctionCode[] = _u("Function code");
const char16 Constants::GlobalCode[] = _u("Global code");
const char16 Constants::EvalCode[] = _u("eval code");
const char16 Constants::GlobalFunction[] = _u("glo");
const char16 Constants::UnknownScriptCode[] = _u("Unknown script code");

#ifdef _M_AMD64
const PBYTE Constants::StackLimitForScriptInterrupt = (PBYTE)0x7fffffffffffffff;
#else
const PBYTE Constants::StackLimitForScriptInterrupt = (PBYTE)0x7fffffff;
#endif

#pragma warning(push)
#pragma warning(disable:4815) // Allow no storage for zero-sized array at end of NullFrameDisplay struct.
const Js::FrameDisplay Js::NullFrameDisplay = 0;
const Js::FrameDisplay Js::StrictNullFrameDisplay = FrameDisplay(0, true);
#pragma warning(pop)
