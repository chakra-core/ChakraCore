//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// native int array
var arr1 = [1, 2, 3, 4, 5];
arr1[arr1.length + 5] = 10;/**bp:evaluate('arr1[arr1.length + 3] == undefined')**/
WScript.Echo(arr1[arr1.length]); 

// native float array
var arr2 = [1, 2, 3, 4, 5];
arr1[arr2.length + 5] = 6.5;/**bp:evaluate('arr2[arr2.length + 3] == undefined')**/
WScript.Echo(arr1[arr2.length]); 

// native var array
var arr3 = [1, 2, 3, 4, 5];
arr3[arr3.length + 5] = arr1;/**bp:evaluate('arr3[arr3.length + 3] == undefined')**/
WScript.Echo(arr3[arr3.length]); 

// native float => var array
var arr4 = [1.3, 5.3, -0];
arr4[arr4.length + 5] = arr2;/**bp:evaluate('arr4[arr4.length + 3] == undefined');evaluate('arr4[2] == -0')**/
WScript.Echo(arr4[arr4.length]);

// native array with int and 0x80000002
var arr5 = [1, 4,5];
arr5[arr5.length + 5] = 0x80000002;/**bp:evaluate(' arr5[arr5.length + 3] == undefined');evaluate('arr5[arr5.length + 5] == 0x80000002')**/
WScript.Echo(arr5[arr5.length]); 

// native array with float and 0x80000002
var arr6 = [1.3, 3.4, 0x80000002];
arr6[arr6.length + 5] = -0;/**bp:evaluate('arr6[2] == 0x80000002');evaluate('arr6[arr6.length + 4] == undefined');evaluate('arr6[arr6.length + 5] == -0');**/
