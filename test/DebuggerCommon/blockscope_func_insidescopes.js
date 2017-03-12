//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Function declaration inside scopes
 */

//self executing anonymous function
function Run() {
    (function () {
        function foo1() {
            return 'newfoo';
        }
        eval("foo1 = null");
        eval(''); /**bp:locals(1)**/
    })();

    var x = 1; /**bp:locals(1)**/

    function foo2() {
        {
            function foo3() { }
        }
    }
    foo2();

    x;/**bp:locals(1)**/

    function bar4() {
        function bar5() {
        }
    }
    bar4();

    WScript.Echo('PASSED')/**bp:locals(1)**/
}

WScript.Attach(Run);
//WScript.Detach(Run);