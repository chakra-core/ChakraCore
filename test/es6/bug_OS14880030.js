//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";
var r = delete this;

function foo() {
    "use strict";
    return delete this;
}
r = r && foo();

function bar() {
    "use strict";
    return delete new.target;
}
r = r && bar();

console.log(r ? 'PASS' : 'FAIL');
