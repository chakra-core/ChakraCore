//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that sibling scopes are properly updating the debugger
// scope index tracker when they are created (so they can be properly
// updated during another bytecode generation pass).
// Bug #263635.

function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";
const level_0_identifier_2= "level0";

function level1Func(level_1_identifier_0) {
    
    var level_1_identifier_1 = arguments;
    
    let level_1_identifier_2= "level1";
const level_1_identifier_3= "level1";

    
    var level_1_identifier_4 = "level1";

    
    verify(level_0_identifier_0, "level0", "[Function Declaration with Args - With] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
    verify(level_0_identifier_1, "level0", "[Function Declaration with Args - With] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
    verify(level_0_identifier_2, "level0", "[Function Declaration with Args - With] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
    verify(level_1_identifier_0, "level1", "[Function Declaration with Args - With] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0==\'level1\'')**/;
    
    verify(arguments[0], "level1", "[Function Declaration with Args - With] arguments[0] at level 1");/**bp:evaluate('arguments[0]==\'level1\'')**/;
    
    verify(level_1_identifier_1[0], "level1", "[Function Declaration with Args - With] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1[0]==\'level1\'')**/;
    
    verify(level_1_identifier_2, "level1", "[Function Declaration with Args - With] level_1_identifier_2 at level 1");/**bp:evaluate('level_1_identifier_2==\'level1\'')**/;
    
    verify(level_1_identifier_3, "level1", "[Function Declaration with Args - With] level_1_identifier_3 at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
    verify(level_1_identifier_4, "level1", "[Function Declaration with Args - With] level_1_identifier_4 at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    

    
    arguments[0] += "level1";
    level_1_identifier_1[0] += "level1";
    level_1_identifier_2 += "level1";

    with({ level_2_identifier_0: "level2" }) {
        let level_2_identifier_1= "level2";
const level_2_identifier_2= "level2";

    
    
        verify(level_0_identifier_0, "level0", "[Function Declaration with Args - With] level_0_identifier_0 at level 2");/**bp:evaluate('level_0_identifier_0==\'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Function Declaration with Args - With] level_0_identifier_1 at level 2");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Function Declaration with Args - With] level_0_identifier_2 at level 2");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/;
    
        verify(level_1_identifier_0, "level1level1level1", "[Function Declaration with Args - With] level_1_identifier_0 at level 2");/**bp:evaluate('level_1_identifier_0==\'level1level1level1\'')**/;
    
        verify(arguments[0], "level1level1level1", "[Function Declaration with Args - With] arguments[0] at level 2");/**bp:evaluate('arguments[0]==\'level1level1level1\'')**/;
    
        verify(level_1_identifier_1[0], "level1level1level1", "[Function Declaration with Args - With] level_1_identifier_1 at level 2");/**bp:evaluate('level_1_identifier_1[0]==\'level1level1level1\'')**/;
    
        verify(level_1_identifier_2, "level1level1", "[Function Declaration with Args - With] level_1_identifier_2 at level 2");/**bp:evaluate('level_1_identifier_2==\'level1level1\'')**/;
    
        verify(level_1_identifier_3, "level1", "[Function Declaration with Args - With] level_1_identifier_3 at level 2");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/;
    
        verify(level_1_identifier_4, "level1", "[Function Declaration with Args - With] level_1_identifier_4 at level 2");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/;
    
        verify(level_2_identifier_0, "level2", "[Function Declaration with Args - With] level_2_identifier_0 at level 2");/**bp:evaluate('level_2_identifier_0==\'level2\'')**/;
    
        verify(level_2_identifier_1, "level2", "[Function Declaration with Args - With] level_2_identifier_1 at level 2");/**bp:evaluate('level_2_identifier_1==\'level2\'')**/;
    
        verify(level_2_identifier_2, "level2", "[Function Declaration with Args - With] level_2_identifier_2 at level 2");/**bp:evaluate('level_2_identifier_2==\'level2\'')**/;
    

    
        level_0_identifier_0 += "level2";
        level_1_identifier_0 += "level2";
        arguments[0] += "level2";
        level_1_identifier_2 += "level2";
        level_2_identifier_1 += "level2";

    
    
        verify(level_2_identifier_0, "level2", "[Function Declaration with Args - With] level_2_identifier_0 after assignment at level 2");/**bp:evaluate('level_2_identifier_0==\'level2\'')**/; 
        verify(level_2_identifier_1, "level2level2", "[Function Declaration with Args - With] level_2_identifier_1 after assignment at level 2");/**bp:evaluate('level_2_identifier_1==\'level2level2\'')**/; 
        verify(level_2_identifier_2, "level2", "[Function Declaration with Args - With] level_2_identifier_2 after assignment at level 2");/**bp:evaluate('level_2_identifier_2==\'level2\'')**/; 

    }

    
    verify(level_1_identifier_0, "level1level1level1level2level2", "[Function Declaration with Args - With] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0==\'level1level1level1level2level2\'')**/; 
    verify(arguments[0], "level1level1level1level2level2", "[Function Declaration with Args - With] arguments[0] after assignment at level 1");/**bp:evaluate('arguments[0]==\'level1level1level1level2level2\'')**/; 
    verify(level_1_identifier_1[0], "level1level1level1level2level2", "[Function Declaration with Args - With] level_1_identifier_1[0] after assignment at level 1");/**bp:evaluate('level_1_identifier_1[0]==\'level1level1level1level2level2\'')**/; 
    verify(level_1_identifier_2, "level1level1level2", "[Function Declaration with Args - With] level_1_identifier_2 after assignment at level 1");/**bp:evaluate('level_1_identifier_2==\'level1level1level2\'')**/; 
    verify(level_1_identifier_3, "level1", "[Function Declaration with Args - With] level_1_identifier_3 after assignment at level 1");/**bp:evaluate('level_1_identifier_3==\'level1\'')**/; 
    verify(level_1_identifier_4, "level1", "[Function Declaration with Args - With] level_1_identifier_4 after assignment at level 1");/**bp:evaluate('level_1_identifier_4==\'level1\'')**/; 

}
level1Func("level1");
level1Func = undefined;


verify(level_0_identifier_0, "level0level2", "[Function Declaration with Args - With] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0==\'level0level2\'')**/; 
verify(level_0_identifier_1, "level0", "[Function Declaration with Args - With] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1==\'level0\'')**/; 
verify(level_0_identifier_2, "level0", "[Function Declaration with Args - With] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2==\'level0\'')**/; 

WScript.Echo("PASSED");
