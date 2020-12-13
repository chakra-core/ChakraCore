//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

ParseExceptionObject::ParseExceptionObject(HRESULT hr, LPCWSTR stringOneIn, LPCWSTR stringTwoIn)
{
    m_hr = hr;
    stringOne = SysAllocString(stringOneIn);
    stringTwo = SysAllocString(stringTwoIn);
    Assert(FAILED(hr));
}

ParseExceptionObject::~ParseExceptionObject()
{
    SysFreeString(stringOne);
    SysFreeString(stringTwo);
}
