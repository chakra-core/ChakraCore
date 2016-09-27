//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Common.h"

namespace PlatformAgnostic
{

Event::Event(const bool autoReset, const bool signaled) : handle(nullptr)
{
}

bool Event::Wait(const unsigned int milliseconds) const
{
}

void Event::Close() const
{
}

void Event::Set() const
{
}

void Event::Reset() const
{
}

Event::~Event()
{
}
} // namespace PlatformAgnostic
