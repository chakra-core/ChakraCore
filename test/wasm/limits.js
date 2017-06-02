//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Limits taken from WasmLimits.h
const MaxTypes = 1000000;
const MaxFunctions = 1000000;
const MaxImports = 100000;
const MaxExports = 100000;
const MaxGlobals = 1000000;
const MaxDataSegments = 100000;
const MaxElementSegments = 10000000;
const MaxTableSize = 10000000;

const MaxStringSize = 100000;
const MaxFunctionLocals = 50000;
const MaxFunctionParams = 1000;
const MaxBrTableElems = 1000000;

const MaxMemoryInitialPages = 16384;
const MaxMemoryMaximumPages = 65536;
const MaxModuleSize = 1024 * 1024 * 1024;
const MaxFunctionSize = 128 * 1024;

/* global assert,testRunner */ // eslint rule
WScript.LoadScriptFile("../UnitTestFrameWork/UnitTestFrameWork.js");
WScript.Flag("-off:wasmdeferred");

function compile(moduleStr) {
  const buf = WebAssembly.wabt.convertWast2Wasm(moduleStr);
  return new WebAssembly.Module(buf);
}

function leb128(number) {
  if (number === 0) return String.fromCharCode(0);
  if (number < 0) throw new Error("SLEB128 not supported");
  let res = "";
  while (number > 0) {
    var value = number & 0x7f;
    number >>= 7;
    if (number > 0) {
      value |= 0x80;
    }
    res += String.fromCharCode(value);
  }
  return res;
}

function createView(bytes) {
  const buffer = new ArrayBuffer(bytes.length);
  const view = new Uint8Array(buffer);
  for (let i = 0; i < bytes.length; ++i) {
    view[i] = bytes.charCodeAt(i);
  }
  return view;
}
const moduleHeader = "\x00\x61\x73\x6d\x01\x00\x00\x00";

function makeLimitTests(opt) {
  return [{
    name: "valid: " + opt.name,
    validTest: true,
    body() {
      assert.doesNotThrow(() => compile(`(module
        ${opt.makeModule(opt.limit)}
      )`), `WebAssembly should allow up to ${opt.limit} ${opt.type}`);
    }
  }, {
    name: "invalid: " + opt.name,
    invalidTest: true,
    body() {
      assert.throws(() => compile(`(module
        ${opt.makeModule(opt.limit + 1)}
      )`), WebAssembly.CompileError, `WebAssembly should not allow ${opt.limit + 1} ${opt.type}`, opt.errorMsg);
    }
  }];
}

const tests = [
  // Tests 0,1
  ...makeLimitTests({
    name: "test max types",
    limit: MaxTypes,
    type: "types",
    errorMsg: "Too many signatures",
    makeModule: limit => (new Array(limit)).fill("(type (func))").join("\n")
  }),
  // Tests 2,3
  ...makeLimitTests({
    name: "test max functions",
    limit: MaxFunctions,
    type: "functions",
    errorMsg: "Too many functions",
    makeModule: limit => (new Array(limit)).fill("(func)").join("\n")
  }),
  // Tests 4,5
  ...makeLimitTests({
    name: "test max imports",
    limit: MaxImports,
    type: "imports",
    errorMsg: "Too many imports",
    makeModule: limit => (new Array(limit)).fill(`(import "" "" (func))`).join("\n"),
  }),
  // Tests 6,7
  ...makeLimitTests({
    name: "test max exports",
    limit: MaxExports,
    type: "exports",
    errorMsg: "Too many exports",
    makeModule: limit => "(func $f)\n" + (new Array(limit)).fill("").map((_, i) => `(export "a${i}" (func $f))`).join("\n"),
  }),
  // Tests 8,9
  ...makeLimitTests({
    name: "test max globals",
    limit: MaxGlobals,
    type: "globals",
    errorMsg: "Too many globals",
    makeModule: limit => (new Array(limit)).fill(`(global i32 (i32.const 0))`).join("\n"),
  }),
  // Tests 10,11
  ...makeLimitTests({
    name: "test max data segments",
    limit: MaxDataSegments,
    type: "data segments",
    errorMsg: "Too many data segments",
    makeModule: limit => "(memory 0)" + (new Array(limit)).fill(`(data (i32.const 0) "")`).join("\n"),
  }),
  // Tests 12,13
  ...makeLimitTests({
    name: "test max element segments",
    limit: MaxElementSegments,
    type: "element segments",
    errorMsg: "Too many element segments",
    makeModule: limit => "(table 0 anyfunc) (func $f1)" + (new Array(limit)).fill(`(elem (i32.const 0) $f1)`).join("\n"),
  }),
  // Test 14
  {
    name: "test max table size",
    body() {
      assert.doesNotThrow(() => new WebAssembly.Table({element: "anyfunc", initial: MaxTableSize}));
      assert.doesNotThrow(() => new WebAssembly.Table({element: "anyfunc", initial: MaxTableSize, maximum: MaxTableSize}));
      assert.doesNotThrow(() => new WebAssembly.Table({element: "anyfunc", maximum: MaxTableSize}));
      assert.throws(() => new WebAssembly.Table({element: "anyfunc", initial: MaxTableSize + 1}));
      assert.throws(() => new WebAssembly.Table({element: "anyfunc", initial: MaxTableSize + 1, maximum: MaxTableSize + 1}));
      assert.throws(() => new WebAssembly.Table({element: "anyfunc", maximum: MaxTableSize + 1}));

      assert.doesNotThrow(() => compile(`(module (table ${MaxTableSize} anyfunc))`));
      assert.doesNotThrow(() => compile(`(module (table ${MaxTableSize} ${MaxTableSize} anyfunc))`));
      assert.doesNotThrow(() => compile(`(module (table 0 ${MaxTableSize} anyfunc))`));
      assert.throws(() => compile(`(module (table ${MaxTableSize + 1} anyfunc))`), WebAssembly.CompileError, "table too big");
      assert.throws(() => compile(`(module (table ${MaxTableSize + 1} ${MaxTableSize + 1} anyfunc))`), WebAssembly.CompileError, "table too big");
      assert.throws(() => compile(`(module (table 0 ${MaxTableSize + 1} anyfunc))`), WebAssembly.CompileError, "table too big");
    }
  },
  {
    name: "test long names",
    body() {
      const customSectionId = "\x00";
      const view = createView(`${moduleHeader}${customSectionId}${leb128(MaxStringSize+3)}${leb128(MaxStringSize)}${"a".repeat(MaxStringSize)}"b"`);
      assert.doesNotThrow(() => new WebAssembly.Module(view));
    }
  }
// MaxStringSize
// MaxFunctionLocals
// MaxFunctionParams
// MaxBrTableElems
// MaxMemoryInitialPages
// MaxMemoryMaximumPages
// MaxModuleSize
// MaxFunctionSize
];

WScript.LoadScriptFile("../UnitTestFrameWork/yargs.js");
const argv = yargsParse(WScript.Arguments, {
  boolean: ["valid", "invalid", "verbose"],
  number: ["start", "end"],
  default: {
    verbose: true,
    valid: true,
    invalid: true,
    start: 0,
    end: tests.length
  }
}).argv;

const todoTests = tests
  .slice(argv.start, argv.end)
  .filter(test => (!test.slow || argv.slow) &&
                  (!test.invalidTest || argv.invalid) &&
                  (!test.validTest || argv.valid));

testRunner.run(todoTests, {verbose: argv.verbose});
