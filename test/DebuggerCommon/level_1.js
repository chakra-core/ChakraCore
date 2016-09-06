//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

eval("let level_1_identifier_0= \"level1\";\nconst level_1_identifier_1= \"level1\";\n;    ;    var level_1_identifier_2 = 'level1';    \n        verify(level_0_identifier_0, \"level0\", \"[Eval] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Eval] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Eval] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Eval] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Eval] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1, \"level1\", \"[Eval] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/;\n    \n;    \n           level_0_identifier_0 += \"level1\";\n           level_0_identifier_1 += \"level1\";\n           level_1_identifier_0 += \"level1\";\n;    ;    \n            verify(level_1_identifier_0, \"level1level1\", \"[Eval] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_1, \"level1\", \"[Eval] level_1_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/; \n");


            verify(level_0_identifier_0, "level0level1", "[Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 
            verify(level_1_identifier_2, "level1", "[Eval] level_1_identifier_2 after assignment at level 0");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

eval("let level_1_identifier_0= \"level1\";\nconst level_1_identifier_1= \"level1\";\n;    eval(\"var level_1_identifier_2 = \'level1\'\");\n;    var level_1_identifier_3 = 'level1';    \n        verify(level_0_identifier_0, \"level0\", \"[Eval with Eval] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Eval with Eval] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Eval with Eval] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Eval with Eval] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_3, \"level1\", \"[Eval with Eval] level_1_identifier_3 at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Eval with Eval] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1, \"level1\", \"[Eval with Eval] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/;\n    \n;    \n           level_0_identifier_1 += \"level1\";\n;    ;    \n            verify(level_1_identifier_0, \"level1\", \"[Eval with Eval] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_1, \"level1\", \"[Eval with Eval] level_1_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/; \n");


            verify(level_0_identifier_0, "level0", "[Eval with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Eval with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Eval with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 
            verify(level_1_identifier_2, "level1", "[Eval with Eval] level_1_identifier_2 after assignment at level 0");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Eval with Eval] level_1_identifier_3 after assignment at level 0");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

(new Function("level_1_identifier_0", ";let level_1_identifier_1= \"level1\";\nconst level_1_identifier_2= \"level1\";\n;    var level_1_identifier_3 = 'level1';    ;     \n        verify(level_0_identifier_0, \"level0\", \"[Function Constructor] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Function Constructor] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Function Constructor] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Function Constructor] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1, \"level1\", \"[Function Constructor] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Function Constructor] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_3, \"level1\", \"[Function Constructor] level_1_identifier_3 at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/;\n    \n ;    \n           level_1_identifier_0 += \"level1\";\n           level_1_identifier_1 += \"level1\";\n ;    ;    \n            verify(level_1_identifier_0, \"level1level1\", \"[Function Constructor] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_1, \"level1level1\", \"[Function Constructor] level_1_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_2, \"level1\", \"[Function Constructor] level_1_identifier_2 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_3, \"level1\", \"[Function Constructor] level_1_identifier_3 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/; \n"))("level1");


            verify(level_0_identifier_0, "level0", "[Function Constructor] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Function Constructor] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Constructor] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

(new Function("level_1_identifier_0", "var level_1_identifier_1 = arguments; ;let level_1_identifier_2= \"level1\";\nconst level_1_identifier_3= \"level1\";\n;    var level_1_identifier_4 = 'level1';    ;     \n        verify(level_0_identifier_0, \"level0\", \"[Function Constructor with Args] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Function Constructor with Args] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Function Constructor with Args] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Function Constructor with Args] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(arguments[0], \"level1\", \"[Function Constructor with Args] arguments[0] at level 1\");/**bp:evaluate(\'arguments[0]==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1[0], \"level1\", \"[Function Constructor with Args] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1[0]==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Function Constructor with Args] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_3, \"level1\", \"[Function Constructor with Args] level_1_identifier_3 at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_4, \"level1\", \"[Function Constructor with Args] level_1_identifier_4 at level 1\");/**bp:evaluate(\'level_1_identifier_4==\\\'level1\\\'\')**/;\n    \n ;    \n           level_0_identifier_0 += \"level1\";\n           level_0_identifier_1 += \"level1\";\n           arguments[0] += \"level1\";\n           level_1_identifier_2 += \"level1\";\n           level_1_identifier_4 += \"level1\";\n ;    ;    \n            verify(level_1_identifier_0, \"level1level1\", \"[Function Constructor with Args] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1level1\\\'\')**/; \n            verify(arguments[0], \"level1level1\", \"[Function Constructor with Args] arguments[0] after assignment at level 1\");/**bp:evaluate(\'arguments[0]==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_1[0], \"level1level1\", \"[Function Constructor with Args] level_1_identifier_1[0] after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1[0]==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_2, \"level1level1\", \"[Function Constructor with Args] level_1_identifier_2 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_3, \"level1\", \"[Function Constructor with Args] level_1_identifier_3 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_4, \"level1level1\", \"[Function Constructor with Args] level_1_identifier_4 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_4==\\\'level1level1\\\'\')**/; \n"))("level1");


            verify(level_0_identifier_0, "level0level1", "[Function Constructor with Args] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Function Constructor with Args] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Constructor with Args] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

(new Function("level_1_identifier_0", ";let level_1_identifier_1= \"level1\";\nconst level_1_identifier_2= \"level1\";\n;    var level_1_identifier_3 = 'level1';    eval(\"var level_1_identifier_4 = \'level1\'\");\n;     \n        verify(level_0_identifier_0, \"level0\", \"[Function Constructor with Eval] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Function Constructor with Eval] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Function Constructor with Eval] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Function Constructor with Eval] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1, \"level1\", \"[Function Constructor with Eval] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Function Constructor with Eval] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_3, \"level1\", \"[Function Constructor with Eval] level_1_identifier_3 at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_4, \"level1\", \"[Function Constructor with Eval] level_1_identifier_4 at level 1\");/**bp:evaluate(\'level_1_identifier_4==\\\'level1\\\'\')**/;\n    \n ;    \n           level_0_identifier_0 += \"level1\";\n           level_0_identifier_1 += \"level1\";\n           level_1_identifier_0 += \"level1\";\n           level_1_identifier_1 += \"level1\";\n ;    ;    \n            verify(level_1_identifier_0, \"level1level1\", \"[Function Constructor with Eval] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_1, \"level1level1\", \"[Function Constructor with Eval] level_1_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1level1\\\'\')**/; \n            verify(level_1_identifier_2, \"level1\", \"[Function Constructor with Eval] level_1_identifier_2 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_3, \"level1\", \"[Function Constructor with Eval] level_1_identifier_3 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_4, \"level1\", \"[Function Constructor with Eval] level_1_identifier_4 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_4==\\\'level1\\\'\')**/; \n"))("level1");


            verify(level_0_identifier_0, "level0level1", "[Function Constructor with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Function Constructor with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Constructor with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

function level1Func(level_1_identifier_0) {
    
	let level_1_identifier_1= "level1";
const level_1_identifier_2= "level1";

    
    var level_1_identifier_3 = "level1";

    
        verify(level_0_identifier_0, "level0", "[Function Declaration] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Declaration] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Declaration] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Function Declaration] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Function Declaration] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Function Declaration] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Declaration] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    

    
           level_1_identifier_0 += "level1";
           level_1_identifier_3 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[Function Declaration] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1", "[Function Declaration] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[Function Declaration] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1level1", "[Function Declaration] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0", "[Function Declaration] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Function Declaration] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Declaration] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

function level1Func(level_1_identifier_0) {
    
        var level_1_identifier_1 = arguments;
    
	let level_1_identifier_2= "level1";
const level_1_identifier_3= "level1";

    
    var level_1_identifier_4 = "level1";

    
        verify(level_0_identifier_0, "level0", "[Function Declaration with Args] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Declaration with Args] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Declaration with Args] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Function Declaration with Args] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(arguments[0], "level1", "[Function Declaration with Args] arguments[0] at level 1");/**bp:evaluate('arguments[0]==\'level1\'')**/;
    
        verify(level_1_identifier_1[0], "level1", "[Function Declaration with Args] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1[0]==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Function Declaration with Args] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Declaration with Args] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Function Declaration with Args] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_0_identifier_1 += "level1";
           arguments[0] += "level1";
           level_1_identifier_2 += "level1";
           level_1_identifier_4 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[Function Declaration with Args] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(arguments[0], "level1level1", "[Function Declaration with Args] arguments[0] after assignment at level 1");/**bp:evaluate('arguments[0]==\'level1level1\'')**/; 
            verify(level_1_identifier_1[0], "level1level1", "[Function Declaration with Args] level_1_identifier_1[0] after assignment at level 1");/**bp:evaluate('level_1_identifier_1[0]==\'level1level1\'')**/; 
            verify(level_1_identifier_2, "level1level1", "[Function Declaration with Args] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Function Declaration with Args] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 
            verify(level_1_identifier_4, "level1level1", "[Function Declaration with Args] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0level1", "[Function Declaration with Args] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Function Declaration with Args] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Declaration with Args] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

function level1Func(level_1_identifier_0) {
    
	let level_1_identifier_1= "level1";
const level_1_identifier_2= "level1";

    eval("var level_1_identifier_3 = 'level1'");

    var level_1_identifier_4 = "level1";

    
        verify(level_0_identifier_0, "level0", "[Function Declaration with Eval] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Declaration with Eval] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Declaration with Eval] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Function Declaration with Eval] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Function Declaration with Eval] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Function Declaration with Eval] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Declaration with Eval] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Function Declaration with Eval] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    

    
           level_1_identifier_0 += "level1";
           level_1_identifier_1 += "level1";
           level_1_identifier_4 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[Function Declaration with Eval] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1level1", "[Function Declaration with Eval] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[Function Declaration with Eval] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Function Declaration with Eval] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 
            verify(level_1_identifier_4, "level1level1", "[Function Declaration with Eval] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0", "[Function Declaration with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Function Declaration with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Declaration with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

var level1Func = function level_1_identifier_0(level_1_identifier_1) {
    
	let level_1_identifier_2= "level1";
const level_1_identifier_3= "level1";

    
    var level_1_identifier_4 = "level1";

    
        verify(level_0_identifier_0, "level0", "[Function Expression] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Expression] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Expression] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(typeof level_1_identifier_0, "function", "[Function Expression] level_1_identifier_0 at level 1");/**bp:evaluate('typeof level_1_identifier_0==\'function\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Function Expression] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Function Expression] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Expression] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Function Expression] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    

    
           level_0_identifier_1 += "level1";
           level_1_identifier_4 += "level1";

    
    
            verify(level_1_identifier_1, "level1", "[Function Expression] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[Function Expression] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Function Expression] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 
            verify(level_1_identifier_4, "level1level1", "[Function Expression] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0", "[Function Expression] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Function Expression] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Expression] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

var level1Func = function level_1_identifier_0(level_1_identifier_1) {
    
        var level_1_identifier_2 = arguments;
    
	let level_1_identifier_3= "level1";
const level_1_identifier_4= "level1";

    
    var level_1_identifier_5 = "level1";

    
        verify(level_0_identifier_0, "level0", "[Function Expression with Args] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Expression with Args] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Expression with Args] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(typeof level_1_identifier_0, "function", "[Function Expression with Args] level_1_identifier_0 at level 1");/**bp:evaluate('typeof level_1_identifier_0==\'function\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Function Expression with Args] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(arguments[0], "level1", "[Function Expression with Args] arguments[0] at level 1");/**bp:evaluate('arguments[0]==\'level1\'')**/;
    
        verify(level_1_identifier_2[0], "level1", "[Function Expression with Args] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2[0]==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Expression with Args] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Function Expression with Args] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    
        verify(level_1_identifier_5, "level1", "[Function Expression with Args] level_1_identifier_5 at level 1");/**bp:evaluate('level_1_identifier_5==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_1_identifier_3 += "level1";

    
    
            verify(level_1_identifier_1, "level1", "[Function Expression with Args] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/; 
            verify(arguments[0], "level1", "[Function Expression with Args] arguments[0] after assignment at level 1");/**bp:evaluate('arguments[0]==\'level1\'')**/; 
            verify(level_1_identifier_2[0], "level1", "[Function Expression with Args] level_1_identifier_2[0] after assignment at level 1");/**bp:evaluate('level_1_identifier_2[0]==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1level1", "[Function Expression with Args] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1level1\'')**/; 
            verify(level_1_identifier_4, "level1", "[Function Expression with Args] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/; 
            verify(level_1_identifier_5, "level1", "[Function Expression with Args] level_1_identifier_5 after assignment at level 1");/**bp:evaluate('level_1_identifier_5==\'level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0level1", "[Function Expression with Args] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0", "[Function Expression with Args] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Expression with Args] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

var level1Func = function level_1_identifier_0(level_1_identifier_1) {
    
	let level_1_identifier_2= "level1";
const level_1_identifier_3= "level1";

    eval("var level_1_identifier_4 = 'level1'");

    var level_1_identifier_5 = "level1";

    
        verify(level_0_identifier_0, "level0", "[Function Expression with Eval] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Expression with Eval] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Expression with Eval] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(typeof level_1_identifier_0, "function", "[Function Expression with Eval] level_1_identifier_0 at level 1");/**bp:evaluate('typeof level_1_identifier_0==\'function\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Function Expression with Eval] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Function Expression with Eval] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Expression with Eval] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Function Expression with Eval] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    
        verify(level_1_identifier_5, "level1", "[Function Expression with Eval] level_1_identifier_5 at level 1");/**bp:evaluate('level_1_identifier_5==\'level1\'')**/;
    

    
           level_1_identifier_1 += "level1";

    
    
            verify(level_1_identifier_1, "level1level1", "[Function Expression with Eval] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[Function Expression with Eval] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Function Expression with Eval] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 
            verify(level_1_identifier_4, "level1", "[Function Expression with Eval] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/; 
            verify(level_1_identifier_5, "level1", "[Function Expression with Eval] level_1_identifier_5 after assignment at level 1");/**bp:evaluate('level_1_identifier_5==\'level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0", "[Function Expression with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Function Expression with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Function Expression with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

var indirectEval = eval;
indirectEval("let level_1_identifier_0= \"level1\";\nconst level_1_identifier_1= \"level1\";\n;    ;    var level_1_identifier_2 = 'level1';    \n        verify(level_0_identifier_0, \"level0\", \"[Indirect Eval] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Indirect Eval] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Indirect Eval] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Indirect Eval] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Indirect Eval] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1, \"level1\", \"[Indirect Eval] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/;\n    \n;    \n           level_0_identifier_1 += \"level1\";\n           level_1_identifier_2 += \"level1\";\n;    ;    \n            verify(level_1_identifier_0, \"level1\", \"[Indirect Eval] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_1, \"level1\", \"[Indirect Eval] level_1_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/; \n");


            verify(level_0_identifier_0, "level0", "[Indirect Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Indirect Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Indirect Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 
            verify(level_1_identifier_2, "level1level1", "[Indirect Eval] level_1_identifier_2 after assignment at level 0");/**bp:evaluate('level_1_identifier_2==\'level1level1\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

var indirectEval = eval;
indirectEval("let level_1_identifier_0= \"level1\";\nconst level_1_identifier_1= \"level1\";\n;    eval(\"var level_1_identifier_2 = \'level1\'\");\n;    var level_1_identifier_3 = 'level1';    \n        verify(level_0_identifier_0, \"level0\", \"[Indirect Eval with Eval] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level0\", \"[Indirect Eval with Eval] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level0\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level0\", \"[Indirect Eval with Eval] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level0\\\'\')**/;\n    \n        verify(level_1_identifier_2, \"level1\", \"[Indirect Eval with Eval] level_1_identifier_2 at level 1\");/**bp:evaluate(\'level_1_identifier_2==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_3, \"level1\", \"[Indirect Eval with Eval] level_1_identifier_3 at level 1\");/**bp:evaluate(\'level_1_identifier_3==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_0, \"level1\", \"[Indirect Eval with Eval] level_1_identifier_0 at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_1_identifier_1, \"level1\", \"[Indirect Eval with Eval] level_1_identifier_1 at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/;\n    \n;    \n           level_0_identifier_0 += \"level1\";\n;    ;    \n            verify(level_1_identifier_0, \"level1\", \"[Indirect Eval with Eval] level_1_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_0==\\\'level1\\\'\')**/; \n            verify(level_1_identifier_1, \"level1\", \"[Indirect Eval with Eval] level_1_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_1_identifier_1==\\\'level1\\\'\')**/; \n");


            verify(level_0_identifier_0, "level0level1", "[Indirect Eval with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0", "[Indirect Eval with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Indirect Eval with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 
            verify(level_1_identifier_2, "level1", "[Indirect Eval with Eval] level_1_identifier_2 after assignment at level 0");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Indirect Eval with Eval] level_1_identifier_3 after assignment at level 0");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

{
	let level_1_identifier_0= "level1";
const level_1_identifier_1= "level1";

    
    
        verify(level_0_identifier_0, "level0", "[Let Const] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Let Const] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Let Const] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Let Const] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Let Const] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    

    
           level_1_identifier_0 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[Let Const] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1", "[Let Const] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/; 

}

            verify(level_0_identifier_0, "level0", "[Let Const] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Let Const] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Let Const] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

(new Function("level_0_identifier_0", ";let level_0_identifier_1= \"level1\";\n;    var level_0_identifier_2 = 'level1';    ;     \n        verify(level_0_identifier_0, \"level1\", \"[Masking Function Constructor] level_0_identifier_0 at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level1\\\'\')**/;\n    \n        verify(level_0_identifier_1, \"level1\", \"[Masking Function Constructor] level_0_identifier_1 at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level1\\\'\')**/;\n    \n        verify(level_0_identifier_2, \"level1\", \"[Masking Function Constructor] level_0_identifier_2 at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level1\\\'\')**/;\n    \n ;    \n           level_0_identifier_1 += \"level1\";\n           level_0_identifier_2 += \"level1\";\n ;    ;    \n            verify(level_0_identifier_0, \"level1\", \"[Masking Function Constructor] level_0_identifier_0 after assignment at level 1\");/**bp:evaluate(\'level_0_identifier_0==\\\'level1\\\'\')**/; \n            verify(level_0_identifier_1, \"level1level1\", \"[Masking Function Constructor] level_0_identifier_1 after assignment at level 1\");/**bp:evaluate(\'level_0_identifier_1==\\\'level1level1\\\'\')**/; \n            verify(level_0_identifier_2, \"level1level1\", \"[Masking Function Constructor] level_0_identifier_2 after assignment at level 1\");/**bp:evaluate(\'level_0_identifier_2==\\\'level1level1\\\'\')**/; \n"))("level1");


            verify(level_0_identifier_0, "level0", "[Masking Function Constructor] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Masking Function Constructor] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Masking Function Constructor] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

function level1Func(level_0_identifier_0) {
    
	
    
    var level_0_identifier_1 = "level1";

    
        verify(level_0_identifier_0, "level1", "[Masking Function Declaration] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level1\'')**/;
    
        verify(level_0_identifier_1, "level1", "[Masking Function Declaration] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Masking Function Declaration] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    

    
           level_0_identifier_0 += "level1";

    
    
            verify(level_0_identifier_0, "level1level1", "[Masking Function Declaration] level_0_identifier_0 after assignment at level 1");/**bp:evaluate('level_0_identifier_0==\'level1level1\'')**/; 
            verify(level_0_identifier_1, "level1", "[Masking Function Declaration] level_0_identifier_1 after assignment at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0", "[Masking Function Declaration] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Masking Function Declaration] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Masking Function Declaration] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

var level1Func = function level_1_identifier_0(level_0_identifier_0) {
    
	let level_0_identifier_1= "level1";
const level_0_identifier_2= "level1";
const level_1_identifier_4= "level1";

    
    var level_1_identifier_5 = "level1";

    
        verify(level_0_identifier_0, "level1", "[Masking Function Expression] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level1\'')**/;
    
        verify(level_0_identifier_1, "level1", "[Masking Function Expression] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/;
    
        verify(level_0_identifier_2, "level1", "[Masking Function Expression] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level1\'')**/;
    
        verify(typeof level_1_identifier_0, "function", "[Masking Function Expression] level_1_identifier_0 at level 1");/**bp:evaluate('typeof level_1_identifier_0==\'function\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Masking Function Expression] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    
        verify(level_1_identifier_5, "level1", "[Masking Function Expression] level_1_identifier_5 at level 1");/**bp:evaluate('level_1_identifier_5==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_1_identifier_5 += "level1";

    
    
            verify(level_0_identifier_0, "level1level1", "[Masking Function Expression] level_0_identifier_0 after assignment at level 1");/**bp:evaluate('level_0_identifier_0==\'level1level1\'')**/; 
            verify(level_0_identifier_1, "level1", "[Masking Function Expression] level_0_identifier_1 after assignment at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/; 
            verify(level_0_identifier_2, "level1", "[Masking Function Expression] level_0_identifier_2 after assignment at level 1");/**bp:evaluate('level_0_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_4, "level1", "[Masking Function Expression] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/; 
            verify(level_1_identifier_5, "level1level1", "[Masking Function Expression] level_1_identifier_5 after assignment at level 1");/**bp:evaluate('level_1_identifier_5==\'level1level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


            verify(level_0_identifier_0, "level0", "[Masking Function Expression] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Masking Function Expression] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Masking Function Expression] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

{
	let level_0_identifier_0= "level1";

    
    
        verify(level_0_identifier_0, "level1", "[Masking Let Const] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level1\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Masking Let Const] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Masking Let Const] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_0_identifier_1 += "level1";

    
    
            verify(level_0_identifier_0, "level1level1", "[Masking Let Const] level_0_identifier_0 after assignment at level 1");/**bp:evaluate('level_0_identifier_0==\'level1level1\'')**/; 

}

            verify(level_0_identifier_0, "level0", "[Masking Let Const] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[Masking Let Const] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[Masking Let Const] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

try {
    throw "level1";
} catch(level_0_identifier_0) {
	let level_0_identifier_1= "level1";
const level_0_identifier_2= "level1";
let level_1_identifier_3= "level1";

    
    
        verify(level_0_identifier_0, "level1", "[Masking Try Catch] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level1\'')**/;
    
        verify(level_0_identifier_1, "level1", "[Masking Try Catch] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/;
    
        verify(level_0_identifier_2, "level1", "[Masking Try Catch] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Masking Try Catch] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";

    
    
            verify(level_0_identifier_0, "level1level1", "[Masking Try Catch] level_0_identifier_0 after assignment at level 1");/**bp:evaluate('level_0_identifier_0==\'level1level1\'')**/; 
            verify(level_0_identifier_1, "level1", "[Masking Try Catch] level_0_identifier_1 after assignment at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/; 
            verify(level_0_identifier_2, "level1", "[Masking Try Catch] level_0_identifier_2 after assignment at level 1");/**bp:evaluate('level_0_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Masking Try Catch] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 

}


            verify(level_0_identifier_0, "level0", "[Masking Try Catch] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Masking Try Catch] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Masking Try Catch] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

with({ level_0_identifier_0: "level1",level_0_identifier_1: "level1",level_0_identifier_2: "level1" }) {
	let level_1_identifier_3= "level1";
let level_1_identifier_4= "level1";

    
    
        verify(level_0_identifier_0, "level1", "[Masking With] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level1\'')**/;
    
        verify(level_0_identifier_1, "level1", "[Masking With] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level1\'')**/;
    
        verify(level_0_identifier_2, "level1", "[Masking With] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Masking With] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Masking With] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_0_identifier_1 += "level1";
           level_1_identifier_4 += "level1";

    
    
            verify(level_0_identifier_0, "level1level1", "[Masking With] level_0_identifier_0 after assignment at level 1");/**bp:evaluate('level_0_identifier_0==\'level1level1\'')**/; 
            verify(level_0_identifier_1, "level1level1", "[Masking With] level_0_identifier_1 after assignment at level 1");/**bp:evaluate('level_0_identifier_1==\'level1level1\'')**/; 
            verify(level_0_identifier_2, "level1", "[Masking With] level_0_identifier_2 after assignment at level 1");/**bp:evaluate('level_0_identifier_2==\'level1\'')**/; 
            verify(level_1_identifier_3, "level1", "[Masking With] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 
            verify(level_1_identifier_4, "level1level1", "[Masking With] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1level1\'')**/; 

}


            verify(level_0_identifier_0, "level0", "[Masking With] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Masking With] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Masking With] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

try {
    throw "level1";
} catch(level_1_identifier_0) {
	let level_1_identifier_1= "level1";
const level_1_identifier_2= "level1";

    
    
        verify(level_0_identifier_0, "level0", "[Try Catch] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Try Catch] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Try Catch] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Try Catch] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Try Catch] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Try Catch] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_1_identifier_0 += "level1";
           level_1_identifier_1 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[Try Catch] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1level1", "[Try Catch] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[Try Catch] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 

}


            verify(level_0_identifier_0, "level0level1", "[Try Catch] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0", "[Try Catch] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Try Catch] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

try {
    throw "level1";
} catch(level_1_identifier_0) {
	let level_1_identifier_1= "level1";
const level_1_identifier_2= "level1";

    eval("var level_1_identifier_3 = 'level1'");

    
        verify(level_0_identifier_0, "level0", "[Try Catch with Eval] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Try Catch with Eval] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Try Catch with Eval] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Try Catch with Eval] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Try Catch with Eval] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Try Catch with Eval] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[Try Catch with Eval] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    

    

    
    
            verify(level_1_identifier_0, "level1", "[Try Catch with Eval] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/; 
            verify(level_1_identifier_1, "level1", "[Try Catch with Eval] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[Try Catch with Eval] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 

}


            verify(level_0_identifier_0, "level0", "[Try Catch with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/; 
            verify(level_0_identifier_1, "level0", "[Try Catch with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Try Catch with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 
            verify(level_1_identifier_3, "level1", "[Try Catch with Eval] level_1_identifier_3 after assignment at level 0");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

with({ level_1_identifier_0: "level1" }) {
	let level_1_identifier_1= "level1";
const level_1_identifier_2= "level1";

    
    
        verify(level_0_identifier_0, "level0", "[With] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[With] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[With] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[With] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[With] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[With] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_0_identifier_1 += "level1";
           level_1_identifier_0 += "level1";
           level_1_identifier_1 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[With] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1level1", "[With] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[With] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 

}


            verify(level_0_identifier_0, "level0level1", "[With] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0level1", "[With] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0level1\'')**/; 
            verify(level_0_identifier_2, "level0", "[With] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 



}; foo();


function foo(){function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

with({ level_1_identifier_0: "level1" }) {
	let level_1_identifier_1= "level1";
const level_1_identifier_2= "level1";

    eval("var level_1_identifier_3 = 'level1'");

    
        verify(level_0_identifier_0, "level0", "[With with Eval] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[With with Eval] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[With with Eval] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_3, "level1", "[With with Eval] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_0, "level1", "[With with Eval] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[With with Eval] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1==\'level1\'')**/;
    
        verify(level_1_identifier_2, "level1", "[With with Eval] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_1_identifier_0 += "level1";
           level_1_identifier_1 += "level1";

    
    
            verify(level_1_identifier_0, "level1level1", "[With with Eval] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1level1", "[With with Eval] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1==\'level1level1\'')**/; 
            verify(level_1_identifier_2, "level1", "[With with Eval] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/; 

}


            verify(level_0_identifier_0, "level0level1", "[With with Eval] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level1\'')**/; 
            verify(level_0_identifier_1, "level0", "[With with Eval] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[With with Eval] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 
            verify(level_1_identifier_3, "level1", "[With with Eval] level_1_identifier_3 after assignment at level 0");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 



}; foo();


WScript.Echo('PASSED');
