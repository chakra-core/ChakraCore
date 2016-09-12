//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function verify(){}
function Run(){
    function verify(act, exp, msg) { }
    var level_0_identifier_0 = "level0";
    
    // upper scope - that is closed over in level2func
    {
        let level_1_identifier_0= "level1_0";  /* captured */
        const level_1_identifier_1= "level1_1";   /* not captured by nested function */
        let level_1_identifier_2; /**bp:locals(1); evaluate('level_0_identifier_0'); evaluate('level_1_identifier_0')**/  /* not initialized & captured */ 
        
		function level2Func(level_2_identifier_0) {
            var level_2_identifier_1 = "level2";  
            verify(level_1_identifier_0, "level1level1", "[Let Const - Function Declaration] level_1_identifier_0 at level 2"); 
            level_1_identifier_2; /**bp:locals(1);evaluate('level_1_identifier_0'); evaluate('level_2_identifier_1')**/ 
        }
        level2Func("level2");
        level2Func = undefined;
        level_1_identifier_2 = "test";
    }
}
Run.apply({});


function Run1(){
    var level_0_identifier_0 = "level0";
    let level_0_identifier_1= "level0";
    const level_0_identifier_2= "level0";

    {
        let level_0_identifier_0= "level1";
        let level_0_identifier_1= "level1";
        let level_0_identifier_2= "level1";

        var level2Func = function level_2_identifier_0(level_2_identifier_1) {
            var level_2_identifier_2 = arguments;
            let level_2_identifier_3= "level2";
            const level_2_identifier_4= "level2";
            var level_2_identifier_5 = "level2";
            var _____dummyvar________ = 1;/**bp:evaluate('level_2_identifier_3')**/;
            level_0_identifier_2 += "level2";         
    };
    level2Func("level2");
    }
}
Run1.apply({});
WScript.Echo("PASSED");