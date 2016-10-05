//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating bug 256729

let level_0_identifier_1= "level0";

eval("let level_1_identifier_0= \"level1\";\n\
      const level_1_identifier_1= \"level1\";\n\
      {  \n\
         function level3Func(level_1_identifier_0) {\n    \n\
            let level_3_identifier_1= \"level3\";\n\n    \n\
            \
            level_1_identifier_1; /**bp:evaluate('level_1_identifier_1')**/;\n    \n\
            \n\
                 level_1_identifier_0 += \"level3\";\n\
            \
            level_3_identifier_1; /**bp:evaluate('level_3_identifier_1')**/; \n\
            }\n\
            level3Func(\"level3\");\nlevel3Func = undefined;\n\
            level_1_identifier_0; /**bp:evaluate('level_1_identifier_0')**/; \n\n\
        };    \n\
        \n");
        
WScript.Echo("Pass");
