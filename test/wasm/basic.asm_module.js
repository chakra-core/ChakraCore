//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function asmModule($a,$b,$c){
'use asm';
    function func0()
    {
        return 2|0;
    }
    function func1(param0)
    {
        param0 = param0|0;
        var local0 = 0;
        if ((26|0) > (25|0))
        {
            local0 = (local0 + 4)|0;
        }
        
        
        local0 = (local0 + 4)|0;
        local0 = (local0 + 4)|0;
        local0 = (local0 + 2)|0;
        
        
        return (local0 + 42) | 0;
        //return 0;
    }
return {a:func1};
}