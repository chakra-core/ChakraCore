//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
import './moduletest2_mod1a.js';
import './moduletest2_mod1b.js';

loaded++;
if (loaded == 5) console.log("Pass");
