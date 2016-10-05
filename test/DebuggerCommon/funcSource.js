//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// Inspect function source
//

(function foo() {
    var arguments = null;
    var o = {
        f: function() { /*f*/},
        g: function g() { /*g*/ }
    };

    /**bp:locals(2)**/
}).apply({});

WScript.Echo("pass");