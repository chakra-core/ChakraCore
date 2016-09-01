//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validation of No-refresh attach and the function expression with a name with forcedeferparse. Bug : 150491

var top = {};
function Pass(f)
{
    f();
}
function test(aa, dd)
{
    top.f1 = aa;
}

Pass(function d2() {
    var i = false,
        f = "body",
        m, g = 0,
        a = 0,
        b = 0,
        l = 0,
        n = 0,
        h = 0,
        o = true,
        k, c, q, p;
    try {
        i = true
    } catch (j) {}
    if (!i) {
        return
    }
    p = false;
    q = !p && true;

    function d3() {
        m = {width:10, height:20};
        if (q && false) {
            c = (Math.abs(n) > 1 || Math.abs(h) > 1)
        } else {
            c = (n || h)
        }
        if (c) {                            /**bp:locals(1)**/
            b = g;
            l = a;
            if (p) {
                var tt = function s(u, t) {
                    u; t;
                }
            } else {
                if (q) {
                    try {
                        if (o) {
                            o = false;
                        }
                    } catch (r) {}
                }
            }
        }
    }
    test(d3,200);
});

WScript.Attach(top.f1);
WScript.Echo("Pass");
