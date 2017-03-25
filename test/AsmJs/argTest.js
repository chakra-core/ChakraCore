//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var tests = [
    function()
    {
        function AsmModuleDouble() {
            "use asm";

            function test0(x) { x = +x; return +test0sub(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 0.5, 10.5); }
            function test0sub(a,b,c,d,e,f,g,h,j,i,k) {
                a = +a; b = +b;
                c = +c; d = +d;
                e = +e; f = +f;
                g = +g; h = +h;
                j = +j; i = +i;
                k = +k; return +(a); }

            function test1(x) { x = +x; return +test1sub(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 0.5, 10.5); }
            function test1sub(a,b,c,d,e,f,g,h,j,i,k) {
                a = +a; b = +b;
                c = +c; d = +d;
                e = +e; f = +f;
                g = +g; h = +h;
                j = +j; i = +i;
                k = +k; return +(f); }

            function test2(x) { x = +x; return +test2sub(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 0.5, 10.5); }
            function test2sub(a,b,c,d,e,f,g,h,j,i,k) {
                a = +a; b = +b;
                c = +c; d = +d;
                e = +e; f = +f;
                g = +g; h = +h;
                j = +j; i = +i;
                k = +k; return +(g); }

            function test3(x) { x = +x; return +test3sub(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 0.5, 10.5); }
            function test3sub(a,b,c,d,e,f,g,h,j,i,k) {
                a = +a; b = +b;
                c = +c; d = +d;
                e = +e; f = +f;
                g = +g; h = +h;
                j = +j; i = +i;
                k = +k; return +(h); }

            function test4(x) { x = +x; return +test4sub(1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 0.5, 10.5); }
            function test4sub(a,b,c,d,e,f,g,h,j,i,k) {
                a = +a; b = +b;
                c = +c; d = +d;
                e = +e; f = +f;
                g = +g; h = +h;
                j = +j; i = +i;
                k = +k; return +(k); }

            return { test0: test0, test1: test1, test2: test2, test3: test3, test4: test4 };
        }

        var asmModuleDouble = AsmModuleDouble();
        if (asmModuleDouble.test0(2, 3, 4) !=  1.5) throw ('a -  1st arg. storage has failed.');
        if (asmModuleDouble.test1(2, 3, 4) !=  6.5) throw ('f -  6th arg. storage has failed.');
        if (asmModuleDouble.test2(2, 3, 4) !=  7.5) throw ('g -  7th arg. storage has failed.');
        if (asmModuleDouble.test3(2, 3, 4) !=  8.5) throw ('h -  8th arg. storage has failed.');
        if (asmModuleDouble.test4(2, 3, 4) != 10.5) throw ('k - 11th arg. storage has failed.');
    },
    function()
    {
        function asm() {
            "use asm"
            function SubCount(a,b,c,d,e,f,g,h,j,k,l) {
                a = +a;
                b = +b;
                c = c | 0;
                d = d | 0;
                e = +e;
                f = +f;
                g = +g;
                h = +h;
                j = +j;
                k = +k;
                l = +l;

                return +(l);
            }

            function Count(a) {
                a = +a;
                return +SubCount(3.2, 1.2, 2, 5, 6.33, 4.88, 1.2, 2.6, 3.99, 1.2, 2.6);
            }

            return { Count: Count };
        }

        var total = 0;
        var fnc = asm ();
        for(var i = 0;i < 1e6; i++) {
            total += fnc.Count(1, 2);
        }

        if (parseInt(total) != parseInt(1e6 * 2.6)) throw new Error('Test Failed -> ' + total);
    },
    function() {
        function asm() {
            "use asm"
            function SubCount(a,b,c,d,e,f,g) {
                a = a | 0;
                b = b | 0;
                c = c | 0;
                d = d | 0;
                e = e | 0;
                f = f | 0;
                g = +g;

                return +(g);
            }

            function Count(a) {
                a = +a;
                return +SubCount(1, 2, 3, 4, 5, 6, 1.3);
            }

            return { Count: Count };
        }

        var total = 0;
        var fnc = asm ();
        for(var i = 0;i < 1e6; i++) {
            total += fnc.Count(1, 2);
        }

        if (parseInt(total) != parseInt(1e6 * 1.3)) throw new Error('Test Failed -> ' + total);
    }];

for(var i = 0; i < tests.length; i++)
    tests[i]();

print('PASS')
