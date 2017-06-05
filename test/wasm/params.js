//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmI64");

function* paramTypes() {
  while(true) {
    yield "i32";
    yield "i64";
    yield "f32";
    yield "f64";
  }
}

function* callArguments() {
  // 3 values to rotate which type will receive which arg
  while(true) {
    yield {value: -12345, i32: "(i32.const -12345)", i64: "(i64.const -12345)", f32: `(f32.const ${Math.fround(-12345)})`, f64: `(f64.const -12345.0)`};
    yield {value: "0xabcdef12345678", i32: "(i32.const 0)", i64: "(i64.const 0xabcdef12345678)", f32: "(f32.const 0)", f64: "(f64.const 0)"};
    yield {value: Math.PI, i32: `(i32.const ${Math.PI|0})`, i64: `(i64.const ${Math.PI|0})`, f32: `(f32.const ${Math.fround(Math.PI)})`, f64: `(f64.const ${Math.PI})`};
  }
}
const tested = {};
function test(n) {
  if (n in tested) {
    return tested[n];
  }
  print(`Test(${n})`)
  const typeGen = paramTypes();
  const argGen = callArguments();
  let params = [], wasmCallTxt = [], callArgs = [], verify = [];
  for (let i = 0; i < n; ++i) {
    const type = typeGen.next().value;
    const getLocal = `(get_local ${i|0})`;
    const arg = argGen.next().value;

    params.push(type);
    wasmCallTxt.push(getLocal);
    callArgs.push(arg.value);
    verify.push(`(${type}.ne ${getLocal} ${arg[type]}) (br_if 0 (i32.const 0)) (drop)`);
  }
  const paramsTxt = params.join(" ");
  const txt = `(module
    (func $verify (param ${paramsTxt}) (result i32)
      ${verify.join("\n      ")}
      (i32.const 1)
    )
    (func (export "foo") (param ${paramsTxt}) (result i32)
      (call $verify
        ${wasmCallTxt.join("\n        ")}
      )
    )
  )`;
  //print(txt);
  const buf = WebAssembly.wabt.convertWast2Wasm(txt);
  try {
    const module = new WebAssembly.Module(buf);
    const {exports: {foo}} = new WebAssembly.Instance(module);
    if (foo(...callArgs) !== 1) {
      print(`FAILED. Failed to validate with ${n} arguments`);
    }
    tested[n] = true;
    return true;
  } catch (e) {
  }
  tested[n] = false;
}

const [forceTest] = WScript.Arguments;
if (forceTest !== undefined) {
  const res = test(forceTest);
  print(res ? "Module is valid" : "Module is invalid");
  WScript.Quit(0);
}

let nParams = 9000;
let inc = 1000;
let direction = true;

while (inc !== 0) {
  if (test(nParams)) {
    if (direction) {
      nParams += inc;
    } else {
      direction = true;
      inc >>= 1;
      nParams += inc;
    }
  } else {
    if (!direction) {
      nParams -= inc;
    } else {
      direction = false;
      inc >>= 1;
      // make sure the last test is a passing one
      inc = inc || 1;
      nParams -= inc;
    }
  }

  if (nParams > 100000 || nParams < 0) {
    print(`FAILED. Params reached ${nParams} long. Expected an error by now`);
    break;
  }
}

print(`Support at most ${nParams} params`);
