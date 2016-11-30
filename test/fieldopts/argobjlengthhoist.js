//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function Tm()
{
    var n = arguments[0];
    for(var s = 0; s<n.length; s++)
    {
        var f = n.charCodeAt(s);
    }
}

Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());
Tm("reallyLongTestString" + Math.random());

WScript.Echo("pass");
