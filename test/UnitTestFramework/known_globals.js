//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function isKnownGlobal(name) {
  var knownGlobals = [
    "isKnownGlobal",
    "SCA",
    "ImageData",
    "read",
    "WScript",
    "print",
    "read",
    "readbuffer",
    "readline",
    "console",
  ];
  return knownGlobals.includes(name);
}
