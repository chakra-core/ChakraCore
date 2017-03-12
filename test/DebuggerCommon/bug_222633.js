//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Bug 222633 validation

function level1Func(level_1_identifier_0) {
    
    let level_1_identifier_1= "level1";
    const level_1_identifier_2= "level1";

    
    var level_1_identifier_3 = "level1";
    {	
		const level_1_identifier_2= "level2";
		eval("let level_3_identifier_0= \"level3\"; var _____dummyvar________ = 1;/**bp:evaluate(\'level_3_identifier_0==\\\'level3level3\\\'\')**/;");
    }         
};
level1Func("level1");
WScript.Echo("Pass");
