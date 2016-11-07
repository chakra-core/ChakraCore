//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
const __filename = WScript.GetFileName();
const __dirname = WScript.GetDirectory();
const {__filename2, __dirname2} = WScript.LoadScriptFile(`${__dirname}\\scriptDirFile_2.js`, "samethread");
WScript.LoadScriptFile(`${__dirname}\\scriptDirFile_3.js`, "self");
const {__filename4, __dirname4} = WScript.LoadScriptFile(`${__dirname}\\nestedDir\\scriptDirFile_4.js`, "samethread");
WScript.LoadScriptFile(`${__dirname}\\nestedDir\\scriptDirFile_5.js`, "self");

// Because of a limitation in how we load scripts 3 and 5 ("self")
// From these scripts perspective, WScript.GetFileName() & WScript.GetDirectory()
// will be equal to the current script's path
if (
  __dirname === __dirname2 &&
  __dirname === __dirname3 &&
  __dirname === __dirname5 && // This should be different without "self"
  __filename === __filename3 && // This should be different without "self"
  __filename === __filename5 && // This should be different without "self"
  __dirname !== __dirname4 &&
  __filename !== __filename2 &&
  __filename !== __filename4
) {
  print("PASSED");
} else {
  print("FAILED");
}
