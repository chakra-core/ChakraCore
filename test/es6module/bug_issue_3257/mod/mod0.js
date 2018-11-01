//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// static imports in a module - relative to the module

import '../mod1.js';
import '../mod2/mod2.js';

// dynamic import in a module - relative to the module

import('../mod1.js').catch(e => print("Should not be printed"));

print("mod0");
