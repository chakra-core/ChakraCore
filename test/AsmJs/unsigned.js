//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const ops = ["/", "%", "<", "<=", ">", ">=", "==", "!="];

const makeFunc = (name, op1, op2) => `
  function ${name}(x, y) {
    x = x|0;
    y = y|0;
    var a1 = 0, a2 = 0;
    a1 = ${op1};
    a2 = ${op2};
    multiReturn(a1|0, a2|0);
  }`;

const info = ops.reduce((all, op, i) => {
  const unsignedOp = `((x>>>0)${op}(y>>>0))|0`;
  const signedOp = `((x|0)${op}(y|0))|0`;
  const n1 = `f${i}_u_s`;
  const n2 = `f${i}_s_u`;
  all.module += makeFunc(n1, unsignedOp, signedOp);
  all.module += makeFunc(n2, unsignedOp, signedOp);
  all.funcs.push({op, name: n1}, {op, name: n2});
  return all;
}, {module: "", funcs: []});

const moduleCore = `
  var multiReturn = imports.multiReturn;
  var multiReturnDouble = imports.multiReturnDouble;
  ${info.module}
  return {
    ${info.funcs.map(({name}) => `${name}: ${name},`).join("\n    ")}
  }
`;
const asmModuleTxt = `return function(stdlib, imports) { "use asm" ${moduleCore} }`;
const nonAsmModuleTxt = `return function(stdlib, imports) { ${moduleCore} }`;
const asmImp = (new Function(asmModuleTxt))()({}, {multiReturn, multiReturnDouble: multiReturn});
const nonAsmImp = (new Function(nonAsmModuleTxt))()({}, {multiReturn, multiReturnDouble: multiReturn});


let results;
function multiReturn(a, b) {
  results.push([a, b]);
}

function test(a, b) {
  for (const fnInfo of info.funcs) {
    const {op, name: fn} = fnInfo;
    results = [];
    nonAsmImp[fn](a, b);
    asmImp[fn](a, b);
    if (results.length !== 2) {
      print("Bad length of results " + results);
    } else if (
      results[0][0] !== results[1][0] ||
      results[0][1] !== results[1][1]
    ) {
      print(`Failed. Expected ${fn}(${a} ${op} ${b}) to be [${results[0].join(", ")}]. Got [${results[1].join(", ")}]`);
    }
  }
}
function foo() {}
var all = [ undefined, null,
            true, false, new Boolean(true), new Boolean(false),
            NaN, +0, -0, 0, 1, 10.0, 10.1, -1, -5, 5,
            124, 248, 654, 987, -1026, +98768.2546, -88754.15478,
            1<<32, -(1<<32), (1<<32)-1, 1<<31, -(1<<31), 1<<25, -1<<25,
            Number.MAX_VALUE, Number.MIN_VALUE, Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY,
            new Number(NaN), new Number(+0), new Number( -0), new Number(0), new Number(1),
            new Number(10.0), new Number(10.1),
            new Number(Number.MAX_VALUE), new Number(Number.MIN_VALUE), new Number(Number.NaN),
            new Number(Number.POSITIVE_INFINITY), new Number(Number.NEGATIVE_INFINITY),
            "", "hello", "hel" + "lo", "+0", "-0", "0", "1", "10.0", "10.1",
            new String(""), new String("hello"), new String("he" + "llo"),
            new Object(), [1,2,3], new Object(), [1,2,3] , foo
          ];

for (const a1 of all) {
  for (const a2 of all) {
    test(a1, a2);
  }
}
print("PASSED");
