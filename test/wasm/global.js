//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function printAll (a) {
    
    print("printing module");
    print("get-a " + a["get-a"]()); 
    print("get-x " + a["get-x"]()); 
    print("get-y " + a["get-y"]()); 
    print("get-z " + a["get-z"]()); 
    print("get-b " + a["get-b"]()); 
    print("get-c " + a["get-c"]()); 
    print("get-d " + a["get-d"]()); 
    print("get-e " + a["get-e"]()); 
    print("get-f " + a["get-f"]()); 
}

function printExportedGlobals (a) {
    
    print("printing exported globals");
    print("x " + a["x"]); 
    print("z " + a["z"]); 
    print("c " + a["c"]); 
    print("d " + a["d"]); 
    print("e " + a["e"]); 
}

const blob = WScript.LoadBinaryFile('global.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;

const blob2 = WScript.LoadBinaryFile('import.wasm');
const moduleBytesView2 = new Uint8Array(blob2);
var b = Wasm.instantiateModule(moduleBytesView2, {"m" : a}).exports;

printAll(a);
printExportedGlobals(a);
a["set-y"](123);
printAll(a);
a["set-b"](0.125); 
printAll(a);
printExportedGlobals(a);
print ("printing module b");
printAll(b);

