//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#define CATCH_CONFIG_RUNNER
#pragma warning(push)
// conversion from 'int' to 'char', possible loss of data
#pragma warning(disable:4244)
#include "catch.hpp"
#pragma warning(pop)

// Use nativetests.exe -? to get all command line options

int _cdecl main(int argc, char * const argv[])
{
    return Catch::Session().run(argc, argv);
}
