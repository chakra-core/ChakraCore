//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

(function func(arg = function () {
                         return func;
                     }()) {
    return func;
    function func() {}
})();

WScript.Echo('pass');
