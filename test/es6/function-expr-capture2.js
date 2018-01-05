//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

eval(
    '(function f() {' +
    '     with({}) {' +
    '         (function () {' +
    '             return f;' +
    '         })();' +
    '     }' +
    ' }());'
);

WScript.Echo('pass');
