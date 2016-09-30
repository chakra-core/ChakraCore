//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(x, y, z) {
  return x - y / z; /**bp:stack();locals();evaluate('arguments',1);**/
}

var a = [...[123], ...[45.01, 6789], 23.58];

/**bp:evaluate('a');evaluate('[...a]');evaluate('[...[123], ...[45.01, 6789], 23.58]')**/

foo(...a);
foo(...[123], ...[45.01, 6789], 23.58);
foo(123, 45.01, 6789, 23.58);

WScript.Echo('PASS');