//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function* gf1 () {
    yield 10;
    yield 20;
    yield 30;

    function a() { }
    function b() { }
    function c() { }

    yield a();

    yield b() + (yield c());
}

// Try step-into on gf(), shouldn't step into native code, skip to next statement.
// Try step-into g.next() and step-out, step-over, and step-into across yield expressions.
// Also when resuming gf, instruction pointer remains at previous yield after stepping in.
// Then try step-into on yield expressions with function calls, should step into the calls
// in correct order, and then step-out returns to the yield expression right before yielding.
let g = gf1(); /**bp:
                stack();resume('step_into');

                stack();resume('step_into');
                stack();resume('step_out');

                stack();resume('step_into');
                stack();resume('step_over');
                stack();resume('step_over');

                stack();resume('step_into');
                stack();resume('step_over');
                stack();resume('step_into');

                stack();resume('step_into');
                stack();resume('step_over');
                stack();resume('step_into');
                stack();resume('step_out');
                stack();resume('step_into');

                stack();resume('step_into');
                stack();resume('step_over');
                stack();resume('step_into');
                stack();resume('step_out');
                stack();resume('step_into');
                stack();resume('step_out');
                stack();resume('step_into');

                stack();
               **/

g.next(1);
g.next(2);
g.next(3);
g.next(4);
g.next(5);
g.next(6);
g;

function* gf2(p, q) {
    var a = 1;
    yield a; /**bp: locals();**/

    let b = 2;
    yield b; /**bp: locals();**/
}

g = gf2(10, 20);
g.next();
g.next();
g.next();

function* gf3() {
    yield 1;
    yield 2;
    yield 3;
}
function* gf4() {
    yield* gf3();
}

g = gf4(); /**bp:
                stack();resume('step_into');
                stack();resume('step_into');
                stack();resume('step_into');
                stack();resume('step_out');
                stack();resume('step_out');

                stack();resume('step_into');
                stack();resume('step_over');

                stack();resume('step_into');
                stack();resume('step_into');
                stack();resume('step_out');
                stack();resume('step_out');
                stack();
            **/

g.next(1);
g.next(2);
g.next(3);

g = gf3(); /**bp:
                resume('step_over');

                resume('step_over');

                stack();resume('step_into');
                stack();resume('step_into');

                stack();
            **/
g.next();
g.return(1);

g = gf4(); /**bp:
                resume('step_over');

                resume('step_over');

                stack();resume('step_into');
                stack();resume('step_into');
                stack();resume('step_into');
                stack();resume('step_out');
                stack();resume('step_out');
            **/

g.next(1);
g.return(2);

WScript.Echo("PASS");
