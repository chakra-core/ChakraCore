//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";
var r = delete this;

function test1() {
    "use strict";
    return delete this;
}

function test2() {
    "use strict";
    return delete new.target;
}

function test3() {
    "use strict";
    try {
        eval('delete arguments;');
    } catch(e) {
        return true;
    }
    return false;
}

function test4() {
    "use strict";
    let a = 'a';
    try {
        eval('delete a;');
    } catch(e) {
        return true;
    }
    return false;
}

function test5() {
    "use strict";
    try {
        eval(`
            function test5_eval() {
                "use strict";
                let a = 'a';
                delete a;
            }
            test5_eval();
        `);
    } catch(e) {
        return true;
    }
    return false;
}

r = r && test1() && test2() && test3() && test4() && test5();

console.log(r ? 'PASS' : 'FAIL');
