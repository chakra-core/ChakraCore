//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that locals inspection in non-top frames properly
// shows let/const variables when inspecting at the end of a block
// (adjusts the byte code offset to be contained within the block braces).
// Bug #163347.

function topFrame() {
    ; /**bp:setFrame(1);locals()**/
}

function secondFrame() {
    {
        let y = 100;
        topFrame();
    }
}

secondFrame();

WScript.Echo("PASSED");