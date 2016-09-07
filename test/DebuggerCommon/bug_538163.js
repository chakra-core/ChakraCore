//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function bar ()
{
    var d = [];
    var m = function(){var a=d,b;if(!a.length)return;d=[];} /**bp(34):stack();resume('step_into')**/ /**bp(48):stack();resume('step_into')**/
    m();
}
bar();
WScript.Echo("Pass");