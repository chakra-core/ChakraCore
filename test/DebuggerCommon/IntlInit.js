//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let x = 0;
Intl.getCanonicalLocales("en-us") /**bp:resume('step_over');locals();**/
x++;
Intl.getCanonicalLocales("en-us") /**bp:resume('step_over');locals();**/
x++;
print("PASS");
