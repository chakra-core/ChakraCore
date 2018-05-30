//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function makeArray() {
  return [new Array(true), new Array("some string"), new Array(1075133691)];
}

for (let i = 0; i < 100; ++i) {
  makeArray();
}

console.log("pass");
