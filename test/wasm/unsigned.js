//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmI64");
const cliArgs = WScript.Arguments || [];

if (cliArgs.indexOf("--help") !== -1) {
  print("usage: ch unsigned.js -args [start index] [end index] [-verbose] -endargs");
  WScript.quit(0);
}

// Parse arguments
let verbose = 0;
while (true) {
  const iVerbose = cliArgs.indexOf("-verbose");
  if (iVerbose === -1) {
    break;
  }
  cliArgs.splice(iVerbose, 1);
  ++verbose;
}
const start = cliArgs[0]|0;
const end = cliArgs[1] === undefined ? undefined : cliArgs[1]|0;
const moduleStart = cliArgs[2] === undefined ? undefined : cliArgs[2]|0;
const moduleEnd = cliArgs[3] === undefined ? undefined : cliArgs[3]|0;

const signatures = {
  I_II: ["i32", "i32", "i32"],
  I_LL: ["i32", "i64", "i64"],
  L_LL: ["i64", "i64", "i64"],
};
const ops = [
  {
    unsigned: "i32.lt_u",
    signed: "i32.lt_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i32.gt_u",
    signed: "i32.gt_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i32.le_u",
    signed: "i32.le_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i32.ge_u",
    signed: "i32.ge_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i64.lt_u",
    signed: "i64.lt_s",
    sig: signatures.I_LL,
  },
  {
    unsigned: "i64.gt_u",
    signed: "i64.gt_s",
    sig: signatures.I_LL,
  },
  {
    unsigned: "i64.le_u",
    signed: "i64.le_s",
    sig: signatures.I_LL,
  },
  {
    unsigned: "i64.ge_u",
    signed: "i64.ge_s",
    sig: signatures.I_LL,
  },
  {
    unsigned: "i32.div_u",
    signed: "i32.div_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i32.rem_u",
    signed: "i32.rem_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i32.shr_u",
    signed: "i32.shr_s",
    sig: signatures.I_II,
  },
  {
    unsigned: "i64.div_u",
    signed: "i64.div_s",
    sig: signatures.L_LL,
  },
  {
    unsigned: "i64.rem_u",
    signed: "i64.rem_s",
    sig: signatures.L_LL,
  },
  {
    unsigned: "i64.shr_u",
    signed: "i64.shr_s",
    sig: signatures.L_LL,
  },
  {
    unsigned: "i32.trunc_u/f32",
    signed: "i32.trunc_s/f32",
    sig: ["i32", "f32"],
  },
  {
    unsigned: "i32.trunc_u/f64",
    signed: "i32.trunc_s/f64",
    sig: ["i32", "f64"],
  },
  {
    unsigned: "i64.extend_u/i32",
    signed: "i64.extend_s/i32",
    sig: ["i64", "i32"],
  },
  {
    unsigned: "i64.trunc_u/f32",
    signed: "i64.trunc_s/f32",
    sig: ["i64", "f32"],
  },
  {
    unsigned: "i64.trunc_u/f64",
    signed: "i64.trunc_s/f64",
    sig: ["i64", "f64"],
  },
  {
    unsigned: "f32.convert_u/i32",
    signed: "f32.convert_s/i32",
    sig: ["f32", "i32"],
  },
  {
    unsigned: "f32.convert_u/i64",
    signed: "f32.convert_s/i64",
    sig: ["f32", "i64"],
  },
  {
    unsigned: "f64.convert_u/i32",
    signed: "f64.convert_s/i32",
    sig: ["f64", "i32"],
  },
  {
    unsigned: "f64.convert_u/i64",
    signed: "f64.convert_s/i64",
    sig: ["f64", "i64"],
  },
];
const allValues = {
  i32: [0, 1, -1, 3, 432123, 0x80000000, 0xCCCCCCCC],
  f32: [0, 1, -1, Math.PI, -Math.PI, Math.E, -Math.E]
};
allValues.i64 = allValues.i32.concat("0x8000000000000000", "0xCCCCCCCCCCCCCCCC");
allValues.f64 = allValues.f32;

let moduleId = 1;
let scriptId = 1;
function generateTest({unsigned, signed, sig}, hardCoded) {
  const [retType, ...argTypes] = sig;
  let nparam = 0;
  let callArgs = "";
  let params = "";

  const getParam = (type) => {
    const getLocal = `(get_local ${nparam++})`;
    callArgs += getLocal;
    params = (params || "(param ") + type + " ";
    return getLocal;
  };
  const args = (hardCoded
    ? hardCoded.map((val, i) => {
      if (val === null) {
        return getParam(argTypes[i]);
      }
      return `(${argTypes[i]}.const ${val.toString()})`;
    })
    : argTypes.map(getParam)
  ).join(" ");
  if (params.length > 0) params += ")";
  const funcSig = `${params} (result ${retType})`;
  const unsignedOp = `(${unsigned} ${args})`;
  const signedOp = `(${signed} ${args})`;
  return (
`  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; Functions to test operator ${unsigned}
  ;; Range ${scriptId}.0-${scriptId}.7
  (func $${unsigned} ${funcSig} ${unsignedOp})
  (func $${signed} ${funcSig} ${signedOp})
  (func $${unsigned}-${signed} ${funcSig}
    (${retType}.add ${unsignedOp} ${signedOp})
  )
  (func $${signed}-${unsigned} ${funcSig}
    (${retType}.add ${signedOp} ${unsignedOp})
  )
  (func $${unsigned}-${unsigned} ${funcSig}
    (${retType}.add ${unsignedOp} ${unsignedOp})
  )
  (func $${signed}-${signed} ${funcSig}
    (${retType}.add ${signedOp} ${signedOp})
  )
  (func (export "test") ${params} (result i32)
    (${retType}.sub
      (call $${unsigned}-${signed} ${callArgs})
      (call $${signed}-${unsigned} ${callArgs})
    )
    (${retType}.ne (${retType}.const 0))
    (br_if 0 (i32.const 1)) (drop)
    ${[[unsigned, signed], [signed, unsigned], [unsigned, unsigned], [signed, signed]]
      .map(([o1, o2], i) => `
    (${retType}.ne
      (call $${o1}-${o2} ${callArgs})
      (${retType}.add (call $${o1} ${callArgs}) (call $${o2} ${callArgs}))
    )
    (br_if 0 (i32.const ${i + 2})) (drop)
      `).join("")}

    (i32.const 0)
  )`);
}

function generateModule(op, hardCoded) {
  const moduleTxt = `(module
${generateTest(op, hardCoded)}
)`;
  ++moduleId;
  ++scriptId;
  if (verbose > 1) {
    print(moduleTxt);
  }
  const mod = new WebAssembly.Module(WebAssembly.wabt.convertWast2Wasm(moduleTxt));
  const {exports} = new WebAssembly.Instance(mod);
  return exports.test;
}
function shouldSkipModule() {
  if (moduleId < moduleStart || moduleId > moduleEnd) {
    ++moduleId;
    return true;
  }
  return false;
}

function run(op, testFn, ...args) {
  try {
    const res = testFn(...args);
    if (res !== 0) {
      print(`Error for operator ${op}(${args.join(" ")}): ${res}`);
    }
  } catch (e) {
    const str = e.toString().toLowerCase();
    if (!str.includes("overflow") && !str.includes("division by zero")) {
      print(`Unexpected error: ${e}`);
    }
  }
}

for (let i = 0; i < ops.length; ++i) {
  if (i < start || i > end) {
    continue;
  }
  const {unsigned, sig} = ops[i];
  const [ret, ...argTypes] = sig;
  let values = argTypes.map(t => allValues[t]);
  if (values.length === 1) {
    values = values[0].map(v => [v]);
  } else {
    const [a1, a2] = values;
    values = [];
    for (const v1 of a1) {
      for (const v2 of a2) {
        values.push([v1, v2]);
      }
    }
  }

  let paramFn, paramModuleId;
  if (!shouldSkipModule()) {
    paramModuleId = moduleId;
    if (!verbose) {
      print(`${i}: Running arguments tests for ${unsigned}. Module #${paramModuleId}`);
    }
    paramFn = generateModule(ops[i]);
  }
  for (const arg of values) {
    // Default test
    if (paramFn) {
      if (verbose) {
        print(`${i}: Running arguments tests for ${unsigned}(${arg.join(", ")}). Module #${paramModuleId}`);
      }
      run(unsigned, paramFn, ...arg);
    }

    // Test with all hardcoded values
    if (!shouldSkipModule()) {
      if (verbose) {
        print(`${i}: Running hard coded constant tests for ${unsigned}(${arg.join(", ")}). Module #${moduleId}`);
      }

      const hardCodedFn = generateModule(ops[i], arg);
      run(unsigned, hardCodedFn);
    }
    if (arg.length === 2) {
      if (!shouldSkipModule()) {
        // Test with left hardcoded value
        const semiArg1 = [arg[0], null];
        if (verbose) {
          print(`${i}: Running left hard coded constant tests for ${unsigned}([${arg[0]}], ${arg[1]}). Module #${moduleId}`);
        }
        const hardCodedFn2 = generateModule(ops[i], semiArg1);
        run(unsigned, hardCodedFn2, arg[1]);
      }

      if (!shouldSkipModule()) {
        // Test with right hardcoded value
        const semiArg2 = [null, arg[1]];
        if (verbose) {
          print(`${i}: Running right hard coded constant tests for ${unsigned}(${arg[0]}, [${arg[1]}]). Module #${moduleId}`);
        }
        const hardCodedFn3 = generateModule(ops[i], semiArg2);
        run(unsigned, hardCodedFn3, arg[0]);
      }
    }
  }
}
