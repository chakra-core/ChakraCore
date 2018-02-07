//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Make sure we correctly invalidate all of the appropriate missing-property caches when adding a property
// to an object with a dictionary type.
var obj = {
    get a() { return 1; },
    b: 2,
    c: 3,
    // skip d
    e: 5
};

function f(o) {
    return o.a + o.b + o.c + o.d + o.e;
}

f(obj);
f(obj);
f(obj);
f(obj);

// Force a rejit, then make sure that the property guards are correctly
// repopulated now that a once-missing field is present.

obj.d = 4;

f(obj);
f(obj);
f(obj);
f(obj);

WScript.Echo((f(obj) === 15) ? "pass" : "fail");
