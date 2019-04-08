//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var var_0 = '';
for (var var_1 = 0; var_1 < 100000; var_1++) {
    var_0 += `this.a${ var_1 } = 0;\n`;
}
var var_2 = new Function(var_0);
function func_0() {}
func_0.prototype = new var_2();

function func_1() {}
func_1.prototype = new var_2();

print("Pass")