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

var mod1 = new WebAssembly.Module(readbuffer('global.wasm'));
var a = new WebAssembly.Instance(mod1).exports;

var mod2 = new WebAssembly.Module(readbuffer('import.wasm'));
var b = new WebAssembly.Instance(mod2, {"m" : a}).exports;

printAll(a);
printExportedGlobals(a);
a["set-y"](123);
printAll(a);
a["set-b"](0.125);
printAll(a);
printExportedGlobals(a);
print(`get-i64 ${WebAssembly.nativeTypeCallTest(a["get-i64"])}`);
print("printing module b");
printAll(b);

const mod3 = new WebAssembly.Module(readbuffer('binaries/i64_invalid_global_import.wasm'));
try {
    new WebAssembly.Instance(mod3, {test: {global: 5}});
    print("should have trap");
} catch (e) {
    if (e instanceof TypeError) {
        print(`Should be invalid type conversion: ${e.message}`);
    } else {
        print(`Invalid error ${e}`);
    }
}

const mod4 = new WebAssembly.Module(readbuffer('binaries/i64_invalid_global_export.wasm'));
try {
    new WebAssembly.Instance(mod4, {});
    print("should have trap");
} catch (e) {
    if (e instanceof TypeError) {
        print(`Should be invalid type conversion: ${e.message}`);
    } else {
        print(`Invalid error ${e}`);
    }
}

