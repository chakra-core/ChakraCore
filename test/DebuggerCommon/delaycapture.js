//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function baselineVerify(act, msg) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act);} else { print(msg + ": " + act); } }
function verify(act, exp, msg) { if(act !== exp) { if(typeof WScript !== "undefined") { WScript.Echo(msg + ": " + act + " = " + exp);} else { print(msg + ": " + act + " = " + exp); } } }
function Run(){
var level_0_identifier_0 = "level0";
let level_0_identifier_1= "level0";const level_0_identifier_2= "level0";
{
	let level_1_identifier_0= "level1";const level_1_identifier_1= "level1";
    
    
        verify(level_0_identifier_0, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_0 at level 1");/**bp:evaluate('level_0_identifier_0 == \'level0\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_1 at level 1");/**bp:evaluate('level_0_identifier_1 == \'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_2 at level 1");/**bp:evaluate('level_0_identifier_2 == \'level0\'')**/;
    
        verify(level_1_identifier_0, "level1", "[Let Const - Function Declaration with Args] level_1_identifier_0 at level 1");/**bp:evaluate('level_1_identifier_0 == \'level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Let Const - Function Declaration with Args] level_1_identifier_1 at level 1");/**bp:evaluate('level_1_identifier_1 == \'level1\'')**/;
    

    
           level_0_identifier_0 += "level1";
           level_1_identifier_0 += "level1";

    function level2Func(level_2_identifier_0) {
    
        var level_2_identifier_1 = arguments;
    
	let level_2_identifier_2= "level2";const level_2_identifier_3= "level2";
    
    var level_2_identifier_4 = "level2";

    
        verify(level_0_identifier_0, "level0level1", "[Let Const - Function Declaration with Args] level_0_identifier_0 at level 2");/**bp:evaluate('level_0_identifier_0 == \'level0level1\'')**/;
    
        verify(level_0_identifier_1, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_1 at level 2");/**bp:evaluate('level_0_identifier_1 == \'level0\'')**/;
    
        verify(level_0_identifier_2, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_2 at level 2");/**bp:evaluate('level_0_identifier_2 == \'level0\'')**/;
    
        verify(level_1_identifier_0, "level1level1", "[Let Const - Function Declaration with Args] level_1_identifier_0 at level 2");/**bp:evaluate('level_1_identifier_0 == \'level1level1\'')**/;
    
        verify(level_1_identifier_1, "level1", "[Let Const - Function Declaration with Args] level_1_identifier_1 at level 2");/**bp:evaluate('level_1_identifier_1 == \'level1\'')**/;
    
        verify(level_2_identifier_0, "level2", "[Let Const - Function Declaration with Args] level_2_identifier_0 at level 2");/**bp:evaluate('level_2_identifier_0 == \'level2\'')**/;
    
        verify(arguments[0], "level2", "[Let Const - Function Declaration with Args] arguments[0] at level 2");/**bp:evaluate('arguments[0] == \'level2\'')**/;
    
        verify(level_2_identifier_1[0], "level2", "[Let Const - Function Declaration with Args] level_2_identifier_1 at level 2");/**bp:evaluate('level_2_identifier_1[0] == \'level2\'')**/;
    
        verify(level_2_identifier_2, "level2", "[Let Const - Function Declaration with Args] level_2_identifier_2 at level 2");/**bp:evaluate('level_2_identifier_2 == \'level2\'')**/;
    
        verify(level_2_identifier_3, "level2", "[Let Const - Function Declaration with Args] level_2_identifier_3 at level 2");/**bp:evaluate('level_2_identifier_3 == \'level2\'')**/;
    
        verify(level_2_identifier_4, "level2", "[Let Const - Function Declaration with Args] level_2_identifier_4 at level 2");/**bp:evaluate('level_2_identifier_4 == \'level2\'')**/;
    

    
           level_0_identifier_0 += "level2";
           arguments[0] += "level2";
           level_2_identifier_1[0] += "level2";
           level_2_identifier_4 += "level2";

    
    
            verify(level_2_identifier_0, "level2level2level2", "[Let Const - Function Declaration with Args] level_2_identifier_0 after assignment at level 2");/**bp:evaluate('level_2_identifier_0 == \'level2level2level2\'')**/; 
            verify(arguments[0], "level2level2level2", "[Let Const - Function Declaration with Args] arguments[0] after assignment at level 2");/**bp:evaluate('arguments[0] == \'level2level2level2\'')**/; 
            verify(level_2_identifier_1[0], "level2level2level2", "[Let Const - Function Declaration with Args] level_2_identifier_1[0] after assignment at level 2");/**bp:evaluate('level_2_identifier_1[0] == \'level2level2level2\'')**/; 
            verify(level_2_identifier_2, "level2", "[Let Const - Function Declaration with Args] level_2_identifier_2 after assignment at level 2");/**bp:evaluate('level_2_identifier_2 == \'level2\'')**/; 
            verify(level_2_identifier_3, "level2", "[Let Const - Function Declaration with Args] level_2_identifier_3 after assignment at level 2");/**bp:evaluate('level_2_identifier_3 == \'level2\'')**/; 
            verify(level_2_identifier_4, "level2level2", "[Let Const - Function Declaration with Args] level_2_identifier_4 after assignment at level 2");/**bp:evaluate('level_2_identifier_4 == \'level2level2\'')**/; 

}
level2Func("level2");
level2Func = undefined;

    
            verify(level_1_identifier_0, "level1level1", "[Let Const - Function Declaration with Args] level_1_identifier_0 after assignment at level 1");/**bp:evaluate('level_1_identifier_0 == \'level1level1\'')**/; 
            verify(level_1_identifier_1, "level1", "[Let Const - Function Declaration with Args] level_1_identifier_1 after assignment at level 1");/**bp:evaluate('level_1_identifier_1 == \'level1\'')**/; 

}

            verify(level_0_identifier_0, "level0level1level2", "[Let Const - Function Declaration with Args] level_0_identifier_0 after assignment at level 0");/**bp:evaluate('level_0_identifier_0 == \'level0level1level2\'')**/; 
            verify(level_0_identifier_1, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_1 after assignment at level 0");/**bp:evaluate('level_0_identifier_1 == \'level0\'')**/; 
            verify(level_0_identifier_2, "level0", "[Let Const - Function Declaration with Args] level_0_identifier_2 after assignment at level 0");/**bp:evaluate('level_0_identifier_2 == \'level0\'')**/; 


};
Run();Run(); WScript.Attach(function(){Run();Run();});

WScript.Echo('pass');