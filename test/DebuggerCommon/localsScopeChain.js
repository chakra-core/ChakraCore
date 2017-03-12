//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validates the locals scopes enumeration work, where scopes are mix of slot array and activation object.

function f0()
{
    var j0 =20;
    var j01 =20;
    
    function f1(a1)
    {
        var j = 10;
        var k = 20;
        var m = 20 + j0 + j01;
        function f2(b21, b22)
        {
            var j2 = arguments.length;
            var k2 = 10;
            function f3(c31,c32)
            {
                var a = 10;
                a++;                            /**bp:locals(1)**/
                a++;                            /**bp:evaluate('a');evaluate('j')**/
                return c31+c32+a;               /**bp:locals(1)**/
            }
            
            function f32()
            {
                j;
            }
            f3();
        }
        
        f2();
    }
    f1();
}
f0();

// Validating that valid locals value is shown, instead of picking from the dummy object

function foo(constructor){
  constructor.name;               /**bp:evaluate('constructor.name');evaluate('constructor',1)**/
}

foo({name:123});

WScript.Echo("Pass");
