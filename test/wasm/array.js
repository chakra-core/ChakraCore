const blob = WScript.LoadBinaryFile('array.wasm');
const moduleBytesView = new Uint8Array(blob);
var a = Wasm.instantiateModule(moduleBytesView, {}).exports;
print(a["goodload"](0));
//print(a["badload"](0));
a["badstore"](0);
