//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test that ch handles relative paths for module imports correctly
// See orriginal issue https://github.com/Microsoft/ChakraCore/issues/3257
// and repeat (following reversion) https://github.com/Microsoft/ChakraCore/issues/5237

WScript.LoadScriptFile("bug_issue_3257/mod/mod0.js", "module");
WScript.LoadScriptFile("bug_issue_3257/script/script0.js");
