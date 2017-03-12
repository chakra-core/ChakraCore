//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a1 = 10;

function foo() {
    return a1 * 5;
}

function sub_expression_step_into() {
    a1; /**bp:  logJson('start'); stack(); resume('step_over'); 
                logJson('stepOver1_at_obj_lit'); stack(); resume('step_into');
                logJson('stepIn1_inside_foo'); stack(); resume('step_out');
                logJson('stepOut1_back_in_caller'); stack(); resume('step_over'); 
                logJson('stepOver2_next_statement'); stack(); locals(1);
        **/
    var obj = { 
        a: a1,  
        b: a1 + 20, 
        c:  {
                ca: a1 * 2, 
                cb: foo(),
                cc: a1
            }
        };
    WScript.Echo(JSON.stringify(obj)); /**bp:  logJson('BP1'); stack(); locals(); **/
}

sub_expression_step_into();
