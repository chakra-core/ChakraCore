//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// Blue 591937: CAS:WebCrawler: ASSERT:  Mismatched source encoded byte length (jscript9!Js::ParseableFunctionInfo::SetSourceInfo
//

var foo = (function x() { return x; }  )();

WScript.Attach(foo);
WScript.Echo("pass");