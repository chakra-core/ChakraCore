//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

import Thing1 from './module-4482-bug-dep1.js';
import Thing2 from './module-4482-bug-dep2.js';

export { Thing1, Thing2 };

export default function main()
{
    Thing1();
    Thing2();
}
