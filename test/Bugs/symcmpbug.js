//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function symcmp(c1, c2) {
    return (c1 !== c2);
}

var s = Symbol();
symcmp(s, s);
symcmp(s, s);
symcmp(1, 2);
print("Passed");
