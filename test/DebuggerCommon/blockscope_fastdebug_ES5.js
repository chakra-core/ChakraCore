//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    The call is to a function is made from different scope
*/


{
    function foo() {
        var marker = 100;
        var foo = 1; /**loc(bp1):
                    stack();
                    setFrame(1);
                    locals()
                  **/
    }
}

function bar(){
    var x = 100;
    {
        var y = 100;
        {
            foo();
        }
    }
}

function Run() {
    bar();
    bar();
    bar(); /**bp:enableBp('bp1')**/
    bar; /**bp:disableBp('bp1')**/
	WScript.Echo('PASSED');
}

WScript.Attach(Run); //-maxinterpretcount:1
