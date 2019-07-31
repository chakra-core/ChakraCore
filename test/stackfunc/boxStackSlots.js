//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

try {
    function p(c) {
        (function () {
            try {
                throw p(d);
            } catch (a) {
                (function () {
                    return ParseInt();
                }());
                c;
            }
        }());
        function d() {
        }
    }
    throw p();
}
catch (e) {
    console.log("PASS");
}