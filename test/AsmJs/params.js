//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function* paramTypes() {
  while (true) {
    yield "i32";
    yield "f32";
    yield "f64";
  }
}

function* callArguments() {
  while (true) {
    yield {value: -12345, i32: "-12345", f32: `fr(${Math.fround(-12345)})`, f64: "-12345.0"};
    yield {value: Math.PI, i32: `${Math.PI|0}`, f32: `fr(${Math.fround(Math.PI)})`, f64: `+${Math.PI}`};
  }
}

function wrapType(type, name) {
  switch (type) {
    case "i32": return `${name}|0`;
    case "f32": return `fr(${name})`;
    case "f64": return `+${name}`;
    default: throw new Error("Invalid Type");
  }
}

const tested = {};
function test(n) {
  if (n in tested) {
    return tested[n];
  }
  print(`Test(${n})`);
  const typeGen = paramTypes();
  const argGen = callArguments();
  const params = [], callArgs = [], verify = [];
  for (let i = 0; i < n; ++i) {
    const type = typeGen.next().value;
    const paramName = `a${i|0}`;
    const arg = argGen.next().value;

    params.push({type, name: paramName});
    callArgs.push(arg.value);
    verify.push(`if (${wrapType(type, paramName)} != ${arg[type]}) return 1;`);
  }
  const paramList = params.map(p => p.name).join(",");
  const paramsTypes = params.map(({type, name}) => `${name} = ${wrapType(type, name)}`).join("; ");
  const txt = `
  return function AsmModule(stdlib) {
    "use asm";
    var fr = stdlib.Math.fround;
    function verify(${paramList}) {
      ${paramsTypes};
      ${verify.join("\n")}
      return 0;
    }
    function foo(${paramList}) {
      ${paramsTypes}
      return verify(${paramList})|0;
    }
    return foo;
  }`;

  //print(txt);
  try {
    var AsmModule = (new Function(txt))();
    var foo = AsmModule({Math});
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

let nParams = 8201;
let inc = 100;
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
      nParams -= inc;
    }
  }

  if (nParams > 100000 || nParams < 0) {
    print(`FAILED. Params reached ${nParams} long. Expected an error by now`);
    break;
  }
}

print(`Support at most ${nParams} params`);
