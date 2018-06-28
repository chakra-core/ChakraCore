//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/**exception(uncaught):stack();**/

function noThrowFunction() {
  try {
    throw new Error("throw exception from noThrowFunction");
  } catch (err) {
  }
}
noThrowFunction();

// calling throwFunction() will terminate program, so this has to come last
function throwFunction() { 
   throw new Error("throw exception from throwFunction");
}
throwFunction();
