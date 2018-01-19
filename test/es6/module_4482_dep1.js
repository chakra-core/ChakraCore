//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//this test case's circular dependencies would segfault
//with CC's orriginal module implementation if JIT was enabled
//issue was that:
//i) dep 1 requires dep2 and dep3
//ii) MDI would therefore be triggerred for dep2 (with dep3 queued)
//iii) MDI for dep2 would not explicitly require dep3 as the import is via dep1
//iv) second half of MDI for dep2 would attempt to reference the function Thing2 defined in dep 3
//v) Thing2 would not yet exist = segfault


import Thing1 from 'module_4482_dep2.js';
import Thing2 from 'module_4482_dep3.js';

export { Thing1, Thing2 };

export default
function main()
{
	Thing1();
	Thing2();
}
