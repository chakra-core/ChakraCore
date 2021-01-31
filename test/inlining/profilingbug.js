//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

try {
    function f(){}
    function foo(){
        f.call();
        foo.call(0x1)++;
    }
    foo();
} catch(e) {   }

print("Pass")
