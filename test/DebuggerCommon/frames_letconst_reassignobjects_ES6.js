//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
 Stack Frames Test
 Let const variables pass by value

*/
function Run(){

    function foo() {
        let a = "a";
        bar(a);
        a; /**bp:evaluate('a')**/
    }

    function bar(a) {
        let b = "b";
        baz(a,b);
        b; /**bp:evaluate('b')**/
    }

    function baz(a,b) {
        let c = "baz";
        c =  c + a + b; 
        c;
        /**bp:
            stack();
            setFrame(1);
            evaluate('b = \'b1\'');
            setFrame(2);
            evaluate('a = \'a1\'');
            setFrame(0);
            evaluate('a + b + c')
       **/
    }

    foo();
    WScript.Echo('PASSED');
}

Run();

