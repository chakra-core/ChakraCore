//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
function throwAtHost() {
  throw "throwing";
}

function callHostWithTryCatch() {
  try {
    callHost();
  }
  catch (error) {
    return true;
  }
  
  return false;
}

function callHostWithNoTryCatch() {
  callHost();
  
  return false;
}
