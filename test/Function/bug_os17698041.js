//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Should not crash with switches:
// -off:deferparse -force:redeferral -collectgarbage -parserstatecache -useparserstatecache

function test() { }

test();
CollectGarbage();
test();

console.log('pass'); 
