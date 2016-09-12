//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a1 = 10;

var globObj = {
    ga: a1,
    gb: foo() /**bp:  logJson('globObj'); stack(); resume('step_over');
                logJson('GlstepOver1'); stack();
                **/
}


function foo() {
    return a1 * 5;
}

function bar() {
    a1 += 11;
    return a1;
}

function sub_expression_step_into_more() {
    a1; /**bp:  logJson('start'); stack(); resume('step_over'); 
                logJson('stepOver1_at_obj_lit'); stack(); resume('step_into'); resume('step_into');
                logJson('stepIn1_inside_eval'); stack(); resume('step_into');resume('step_into');
                logJson('stepIn3_inside_foo'); stack(); resume('step_into');resume('step_into');resume('step_into');
                logJson('stepIn6_next_subexp'); stack(); resume('step_into');resume('step_into');
                logJson('stepIn8_inside_bar'); stack(); resume('step_into');resume('step_into');resume('step_into');
                logJson('stepIn11_back_in_objLit'); stack();
        **/
    var obj = { 
        a: a1,  
        b: a1++, 
        func : function(a, b) {
            return 5;
        },
        globObj,
        ev: eval('a1 + a1*2'),
        c:  {
                ca: a1 * 2, 
                cb: foo(),
                cc: a1,
                cd: bar(),
                c3: a1 * 3
            },
        d: foo()    /**bp:  logJson('BP1'); stack(); resume('step_into');
                            logJson('BP1stepIn1_inside_foo'); stack();
                    **/
        };
    WScript.Echo(JSON.stringify(obj));
}

sub_expression_step_into_more();
