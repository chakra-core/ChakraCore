//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(){
    return Array.isArray(1);
};

let failed = foo();
failed |= foo();
failed |= foo();
failed ? print("Fail") : print("Pass");
