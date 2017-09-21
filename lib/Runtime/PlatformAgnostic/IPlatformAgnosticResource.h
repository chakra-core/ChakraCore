//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_COMMON_IFINALIZABLE
#define RUNTIME_PLATFORM_AGNOSTIC_COMMON_IFINALIZABLE

namespace PlatformAgnostic
{
namespace Resource
{
    // Interface
    class IPlatformAgnosticResource
    {
    public:
        virtual void Cleanup() = 0;
    };
} // namespace Intl
} // namespace PlatformAgnostic

#endif /* RUNTIME_PLATFORM_AGNOSTIC_COMMON_IFINALIZABLE */
