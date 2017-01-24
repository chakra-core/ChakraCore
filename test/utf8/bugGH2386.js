//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function toHexCP(c, cp) {
    var hex = "0123456789abcdef";
    return String.fromCharCode(hex.charCodeAt((c >> (cp * 4)) & 0xf));
}

function toHex(str) {
    var result = "";
    for(var i = 0; i < str.length; i++) {
        var c = str.charCodeAt(i);
        for (var cp = 3; cp >= 0; cp--) {
            result += toHexCP(c, cp);
        }
    }
    return "0x" + result;
}

var CHECK = function(h)
{
    var hex_str = String.fromCharCode(h);
    var pattern = eval("/" + hex_str + "/");
    if (toHex(hex_str) != toHex(pattern.source)) {
        throw new Error("String encoding has failed? "
          + toHex(hex_str) + " != " + toHex(pattern.source));
    }
}

CHECK("0x0000");
CHECK("0x0080");
CHECK("0x0800");
CHECK("0xFF80");
CHECK("0xFFFD");
CHECK("0xFFFFFF");
CHECK("0xFFFFFF80");
CHECK("0xFFFFFF80FF");

function CHECK_EVAL(s)
{
    var eval_s = new RegExp( s ).source;
    if (s !== eval_s) throw new Error(
      "String Encoding is broken ? ->" + s);
}

var CH1 = String.fromCharCode('0xe4b8ad');
var CH2 = String.fromCharCode('0xe69687');
var CH3 = String.fromCharCode('0xe336b2');
var CH4 = String.fromCharCode('0xe336b2aa');
var CHX = String.fromCharCode("0x80808080");

var BUFF = '';
for(var i = 0; i < 16; i++)
{
    var str = CH1;

    CHECK_EVAL(str + CHX + BUFF)
    CHECK_EVAL(str + BUFF + CHX)
    CHECK_EVAL(str + BUFF + CHX + '1')
    str += BUFF + CH2 + CHX;
    BUFF += '1';

    CHECK_EVAL(str + '1' + CH3);
    CHECK_EVAL(str + '12' + CH3);
    CHECK_EVAL(str + '123' + CH3);

    CHECK_EVAL(str + '1' + CH4);
    CHECK_EVAL(str + '12' + CH4);
    CHECK_EVAL(str + '123' + CH4)
}

console.log("PASS");