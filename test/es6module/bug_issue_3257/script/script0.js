//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Dynamic import from a Script - relative to CWD not the script
print("script0");
import('bug_issue_3257/mod1.js').catch(e => print ("Should not be printed"));
import('bug_issue_3257/mod2/mod2.js').catch(e => print ("Should not be printed"));
