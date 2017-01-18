WScript.Flag("-wasmI64");
var {i64ToString} = WScript.LoadScriptFile("wasmutils.js");

function retToString(val) {
  if (typeof val === "object") {
    return i64ToString(val);
  }
  return val.toString();
}

WebAssembly.instantiate(readbuffer("binaries/signatures.wasm"))
  .then(({instance: {exports}}) => {
    for (const key in exports) {
      exports[key](32.123, -5);
      exports[key](3, -45.147);
    }
    return "Done";
  })
  .then(console.log, console.log)
