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
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("../wasmspec/testsuite/harness/wasm-module-builder.js");
WScript.Flag("-off:wasmdeferred");

function compile(moduleStr) {
  const buf = WebAssembly.wabt.convertWast2Wasm(moduleStr);
  return new WebAssembly.Module(buf);
}

const notTooLongName = "a".repeat(MaxStringSize);
const tooLongName = "a".repeat(MaxStringSize + 1);

function makeLimitTests(opt) {
  return [{
    name: "valid: " + opt.name,
    validTest: true,
    body() {
      const builder = new WasmModuleBuilder();
      opt.makeModule(builder, opt.limit);
      assert.doesNotThrow(() => new WebAssembly.Module(builder.toBuffer()), `WebAssembly should allow up to ${opt.limit} ${opt.type}`);
    }
  }, {
    name: "invalid: " + opt.name,
    invalidTest: true,
    body() {
      const builder = new WasmModuleBuilder();
      opt.makeModule(builder, opt.limit + 1);
      assert.throws(() => new WebAssembly.Module(builder.toBuffer()), WebAssembly.CompileError, `WebAssembly should not allow ${opt.limit + 1} ${opt.type}`, opt.errorMsg);
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
    makeModule: (builder, limit) => {for (let i = 0; i < limit; ++i) {builder.addType(kSig_v_v);}}
  }),
  // Tests 2,3
  ...makeLimitTests({
    name: "test max functions",
    limit: MaxFunctions,
    type: "functions",
    errorMsg: "Too many functions",
    makeModule: (builder, limit) => {
      const typeIndex = builder.addType(kSig_v_v);
      for (let i = 0; i < limit; ++i) {builder.addFunction(null, typeIndex).addBody([]);}
    }
  }),
  // Tests 4,5
  ...makeLimitTests({
    name: "test max imports",
    limit: MaxImports,
    type: "imports",
    errorMsg: "Too many imports",
    makeModule: (builder, limit) => {
      const typeIndex = builder.addType(kSig_v_v);
      for (let i = 0; i < limit; ++i) {builder.addImport("", "", typeIndex);}
    }
  }),
  // Tests 6,7
  ...makeLimitTests({
    name: "test max exports",
    limit: MaxExports,
    type: "exports",
    errorMsg: "Too many exports",
    makeModule: (builder, limit) => {
      builder.addFunction(null, kSig_v_v).addBody([]);
      for (let i = 0; i < limit; ++i) {builder.addExport("" + i, 0);}
    }
  }),
  // Tests 8,9
  ...makeLimitTests({
    name: "test max globals",
    limit: MaxGlobals,
    type: "globals",
    errorMsg: "Too many globals",
    makeModule: (builder, limit) => {for (let i = 0; i < limit; ++i) {builder.addGlobal(kWasmI32, /* mutable */ false);}}
  }),
  // Tests 10,11
  ...makeLimitTests({
    name: "test max data segments",
    limit: MaxDataSegments,
    type: "data segments",
    errorMsg: "Too many data segments",
    makeModule: (builder, limit) => {
      builder.addMemory(0);
      for (let i = 0; i < limit; ++i) {builder.addDataSegment(0, "");}
    }
  }),
  // Tests 12,13
  ...makeLimitTests({
    name: "test max element segments",
    limit: MaxElementSegments,
    type: "element segments",
    errorMsg: "Too many element segments",
    makeModule: (builder, limit) => {
      builder.addFunction(null, kSig_v_v).addBody([]);
      builder.setFunctionTableLength(1);
      builder.function_table_inits.length = limit;
      builder.function_table_inits.fill({base: 0, is_global: false, array: [0]})
    }
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
  // Test 15
  {
    name: "test custom sections with long names",
    body() {
      function makeCustomSection(name) {
        const customSection = new Binary();
        customSection.emit_section(0, section => {
          section.emit_string(name)
          section.emit_string("payload")
        });
        const builder = new WasmModuleBuilder();
        builder.addExplicitSection(customSection);
        return new WebAssembly.Module(builder.toBuffer());
      }
      assert.doesNotThrow(() => makeCustomSection(notTooLongName));
      assert.throws(() => makeCustomSection(tooLongName), WebAssembly.CompileError, "Name too long");
    }
  },
  // Test 16
  {
    name: "test exports with long names",
    body() {
      function makeExportName(name)
      {
        const builder = new WasmModuleBuilder();
        builder
          .addFunction(name, kSig_v_v)
          .exportFunc()
          .addBody([]);
        return new WebAssembly.Module(builder.toBuffer());
      }
      assert.doesNotThrow(() => makeExportName(notTooLongName));
      assert.throws(() => makeExportName(tooLongName), WebAssembly.CompileError, "Name too long");
    }
  },
  // Test 17
  {
    name: "test imports with long names",
    body() {
      function makeImportName(name)
      {
        const builder = new WasmModuleBuilder();
        builder.addImport(name, 'b', kSig_v_v);
        return new WebAssembly.Module(builder.toBuffer());
      }
      assert.doesNotThrow(() => makeImportName(notTooLongName));
      assert.throws(() => makeImportName(tooLongName), WebAssembly.CompileError, "Name too long");
    }
  },
  // Test 18
  {
    name: "test Name section with long names",
    body() {
      function makeName(name)
      {
        const builder = new WasmModuleBuilder();
        builder
          .addFunction(notTooLongName, kSig_v_v)
          .addBody([]);
        return new WebAssembly.Module(builder.toBuffer());
      }
      assert.doesNotThrow(() => makeName(notTooLongName));
      // todo:: enable test once we start using the Name section
      // assert.throws(() => makeName(tooLongName), WebAssembly.CompileError, "Name too long");
    }
  },
  // Tests 19,20
  ...makeLimitTests({
    name: "test max function locals",
    limit: MaxFunctionLocals,
    type: "locals",
    makeModule: (builder, limit) => {
      builder.addFunction(null, kSig_v_v).addLocals({i32_count: limit}).addBody([]);
    }
  }),
  // Tests 21,22
  ...makeLimitTests({
    name: "test max function params",
    limit: MaxFunctionParams,
    type: "params",
    errorMsg: "Too many arguments in signature",
    makeModule: (builder, limit) => {
      builder.addFunction(null, {params: (new Array(limit)).fill(kWasmI32), results: []}).addBody([]);
    }
  }),
  // Tests 23
  {
    name: "test max memory pages",
    body() {
      assert.doesNotThrow(() => new WebAssembly.Memory({initial: MaxMemoryInitialPages}));
      assert.doesNotThrow(() => new WebAssembly.Memory({initial: MaxMemoryInitialPages, maximum: MaxMemoryMaximumPages}));
      assert.doesNotThrow(() => new WebAssembly.Memory({maximum: MaxMemoryMaximumPages}));
      assert.throws(() => new WebAssembly.Memory({initial: MaxMemoryInitialPages + 1}));
      assert.throws(() => new WebAssembly.Memory({initial: MaxMemoryInitialPages + 1, maximum: MaxMemoryMaximumPages + 1}));
      assert.throws(() => new WebAssembly.Memory({maximum: MaxMemoryMaximumPages + 1}));

      assert.doesNotThrow(() => compile(`(module (memory ${MaxMemoryInitialPages}))`));
      assert.doesNotThrow(() => compile(`(module (memory ${MaxMemoryInitialPages} ${MaxMemoryMaximumPages}))`));
      assert.doesNotThrow(() => compile(`(module (memory 0 ${MaxMemoryMaximumPages}))`));
      assert.throws(() => compile(`(module (memory ${MaxMemoryInitialPages + 1}))`), WebAssembly.CompileError, "Minimum memory size too big");
      assert.throws(() => compile(`(module (memory ${MaxMemoryInitialPages + 1} ${MaxMemoryMaximumPages + 1}))`), WebAssembly.CompileError, "Minimum memory size too big");
      assert.throws(() => compile(`(module (memory 0 ${MaxMemoryMaximumPages + 1}))`), WebAssembly.CompileError, "Maximum memory size too big");
    }
  },
  // Tests 24
  {
    name: "test max module size",
    body() {
      assert.throws(() => new WebAssembly.Module(new ArrayBuffer(MaxModuleSize)), WebAssembly.CompileError, "Malformed WASM module header");
      assert.throws(() => new WebAssembly.Module(new ArrayBuffer(MaxModuleSize + 1)), WebAssembly.CompileError, "Module too big");
    }
  },
  // Tests 25,26
  ...makeLimitTests({
    name: "test max function size",
    limit: MaxFunctionSize,
    type: "size",
    errorMsg: "Function body too big",
    makeModule: (builder, limit) => {
      // limit -1 (number of locals byte) -1 (endOpCode)
      builder.addFunction(null, kSig_v_v).addBody((new Array(limit - 2)).fill(kExprNop));
    }
  }),
  // todo:: test MaxBrTableElems
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
  .filter(test => (!test.invalidTest || argv.invalid) &&
                  (!test.validTest || argv.valid));

testRunner.run(todoTests, {verbose: argv.verbose});
