//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let verbose = false;
let doInvalid = true;

const buf = readbuffer("binaries/api.wasm");
const imports = {
  test: {
    fn() {return 1;},
    fn2() {return 2.14;},
    memory: new WebAssembly.Memory({initial: 1, maximum: 1}),
    g1: 45,
    g2: -8,
  },
  table: {"": new WebAssembly.Table({element: "anyfunc", initial: 30, maximum: 100})}
};

function overrideImports(overrides) {
  return {
    test: Object.assign({}, imports.test, overrides),
    table: overrides.table ? {"":overrides.table} : imports.table
  };
}

function objectToString(o, maxDepth = 5, depth = 0) {
  if (depth >= maxDepth) {
    return o.toString();
  }
  if (Array.isArray(o)) {
    return `[${o.map(e => objectToString(e, maxDepth, depth + 1)).join(", ")}]`;
  } else if (typeof o === "object") {
    return `{${Object.keys(o).map(k => `"${k}":"${objectToString(o[k], maxDepth, depth + 1)}"`).join(", ")}}`;
  }
  return o.toString();
}

function test(module, {exports} = {}) {
  if (module) {
    console.log("Testing module");
    console.log("exports");
    console.log(objectToString(WebAssembly.Module.exports(module)));
    console.log("imports");
    console.log(objectToString(WebAssembly.Module.imports(module)));
  }
  if (exports) {
    console.log("Testing instance");
    console.log(`f1: ${exports.f1()}`);
    console.log(`fn: ${exports.fn()}`);
    console.log(`fn2: ${exports.fn2()}`);
    console.log(`g1: ${exports.g1}`);
    console.log(`g2: ${exports.g2}`);
    console.log(`g3: ${exports.g3}`);
  }
}

async function testInvalidCases(tests) {
  if (!doInvalid) return;

  let i = 0;
  for (const testCase of tests) {
    if (verbose) {
      console.log(testCase);
    }
    try {
      await testCase();
      console.log(`Test ${i++} failed. Should have thrown error`);
    } catch (e) {
      console.log(`Test ${i++} passed. Expected Error: ${e}`);
    }
  }
}

function createView(bytes) {
  const buffer = new ArrayBuffer(bytes.length);
  const view = new Uint8Array(buffer);
  for (let i = 0; i < bytes.length; ++i) {
    view[i] = bytes.charCodeAt(i);
  }
  return view;
}

const invalidBuffers = [
  ,
  "",
  "123",
  4568,
  {},
  {length: 15},
  function() {},
  new ArrayBuffer(),
  createView("\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x85\x80\x80"),
  new Proxy(buf, {get(target, name) {return target[name];}}),
];

const invalidImports = [
  ,
  "",
  123,
  function() {},
  {wrongNamespace: imports.test, table: imports.table},
];

const invalidModules = [
  null,
  "123",
  {},
  new Proxy({}, {})
];

async function testValidate() {
  console.log("\nWebAssembly.validate tests");
  await testInvalidCases([
    // Invalid buffer source
    ...(invalidBuffers.map(b => () => {
      if (!WebAssembly.validate(b)) {
        throw new Error("Buffer source is the right type, but doesn't validate");
      }
    })),
  ]);
  // Make sure empty buffer doesn't validate and doesn't throw
  const emptyBuffer = new ArrayBuffer();
  console.log(`WebAssembly.validate(new ArrayBuffer()) = ${WebAssembly.validate(emptyBuffer)}`);
  console.log(`WebAssembly.validate(new Uint8Array(emptyBuffer)) = ${WebAssembly.validate(new Uint8Array(emptyBuffer))}`);
  console.log(`api.wasm valid: ${WebAssembly.validate(buf)}`);
}

async function testCompile() {
  console.log("\nWebAssembly.compile tests");
  await testInvalidCases([
    ...(invalidBuffers.map(b => () => WebAssembly.compile(b))),
  ]);
  const module = await WebAssembly.compile(buf);
  test(module);
}

async function testInstantiate(baseModule) {
  console.log("\nWebAssembly.instantiate tests");
  await testInvalidCases([
    // Invalid buffer source
    ...(invalidBuffers.map(b => () => WebAssembly.instantiate(b))),
    // Invalid Imports
    ...(invalidImports.map(i => () => WebAssembly.instantiate(buf, i))),
    ...(invalidImports.map(i => () => WebAssembly.instantiate(baseModule, i))),
  ]);

  const {module, instance} = await WebAssembly.instantiate(buf, imports);
  test(module, instance);
  const instance2 = await WebAssembly.instantiate(baseModule, imports);
  test(baseModule, instance2);
}

async function testModuleConstructor() {
  console.log("\nnew WebAssembly.Module tests");
  await testInvalidCases([
    () => WebAssembly.Module(),
    () => WebAssembly.Module(buf),
    ...(invalidBuffers.map(b => () => new WebAssembly.Module(b))),
  ]);
  const module = new WebAssembly.Module(buf);
  test(module);
}

async function testModuleApi() {
  console.log("\nWebAssembly.Module api tests");
  console.log("\nWebAssembly.Module.exports invalid tests");
  await testInvalidCases([
    ...(invalidModules.map(b => () => WebAssembly.Module.exports(b))),
  ]);

  console.log("\nWebAssembly.Module.imports invalid tests");
  await testInvalidCases([
    ...(invalidModules.map(b => () => WebAssembly.Module.imports(b))),
  ]);
}

async function testModuleCustomSection(baseModule) {
  console.log("\nWebAssembly.Module.customSections tests");
  await testInvalidCases([
    ...(invalidModules.map(b => () => WebAssembly.Module.customSections(b))),
    () => WebAssembly.Module.customSections(baseModule, {toString() {throw new Error("Doesn't support toString");}}),
    () => WebAssembly.Module.customSections(baseModule, Symbol()),
  ]);

  const baseSections = WebAssembly.Module.customSections(baseModule, "");
  if (!Array.isArray(baseSections) || baseSections.length !== 0) {
    console.log("Invalid result for WebAssembly.Module.customSections");
    console.log(baseSections);
  }

  let passed = 0, failed = 0;
  function compare(module, customSectionName, expected) {
    const sections = WebAssembly.Module.customSections(module, customSectionName);
    if (sections.length !== expected.length) {
      console.log(`Invalid length. Got ${sections.length}, expected ${expected.length}. ${(new Error()).stack}`);
      ++failed;
      return;
    }
    let asExpected = true;
    for (let iSection = 0; iSection < sections.length; ++iSection) {
      const resBuffer = sections[iSection];
      const expectedStr = expected[iSection];

      const view = new Uint8Array(resBuffer);
      let resStr = "";
      for (let i = 0; i < view.length; ++i) {
        resStr += String.fromCharCode(view[i]);
      }
      if (resStr !== expectedStr) {
        asExpected = false;
        console.log(`Invalid buffer result. Got ${resStr}, expected ${expectedStr}. ${(new Error()).stack}`);
      }
    }
    passed += asExpected;
    failed += !asExpected;
  }

  // See wasts/custom_section.wast for reference
  const module1 = new WebAssembly.Module(createView("\x00\x61\x73\x6d\x01\x00\x00\x00\x00\x24\x10\x61\x20\x63\x75\x73\x74\x6f\x6d\x20\x73\x65\x63\x74\x69\x6f\x6e\x74\x68\x69\x73\x20\x69\x73\x20\x74\x68\x65\x20\x70\x61\x79\x6c\x6f\x61\x64\x00\x20\x10\x61\x20\x63\x75\x73\x74\x6f\x6d\x20\x73\x65\x63\x74\x69\x6f\x6e\x74\x68\x69\x73\x20\x69\x73\x20\x70\x61\x79\x6c\x6f\x61\x64\x00\x11\x10\x61\x20\x63\x75\x73\x74\x6f\x6d\x20\x73\x65\x63\x74\x69\x6f\x6e\x00\x10\x00\x74\x68\x69\x73\x20\x69\x73\x20\x70\x61\x79\x6c\x6f\x61\x64\x00\x01\x00\x00\x24\x10\x00\x00\x63\x75\x73\x74\x6f\x6d\x20\x73\x65\x63\x74\x69\x6f\x00\x74\x68\x69\x73\x20\x69\x73\x20\x74\x68\x65\x20\x70\x61\x79\x6c\x6f\x61\x64"));
  compare(module1, "a custom section", [
    "this is the payload",
    "this is payload",
    "",
  ]);
  compare(module1, "", [
    "this is payload",
    "",
  ]);
  compare(module1, "\00\00custom sectio\00", [
    "this is the payload",
  ]);
  const module2 = new WebAssembly.Module(createView("\x00\x61\x73\x6d\x01\x00\x00\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x01\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x02\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x03\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x04\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x05\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x06\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x07\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x09\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x0a\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x0b\x01\x00\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64\x00\x0e\x06\x63\x75\x73\x74\x6f\x6d\x70\x61\x79\x6c\x6f\x61\x64"));
  compare(module2, "custom", new Proxy({length: 22}, {
    get: (target, name) => name in target ? target[name] : "payload"
  }));
  const module3 = new WebAssembly.Module(createView("\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x07\x01\x60\x02\x7f\x7f\x01\x7f\x00\x1a\x06\x63\x75\x73\x74\x6f\x6d\x74\x68\x69\x73\x20\x69\x73\x20\x74\x68\x65\x20\x70\x61\x79\x6c\x6f\x61\x64\x03\x02\x01\x00\x07\x0a\x01\x06\x61\x64\x64\x54\x77\x6f\x00\x00\x0a\x09\x01\x07\x00\x20\x00\x20\x01\x6a\x0b\x00\x1b\x07\x63\x75\x73\x74\x6f\x6d\x32\x74\x68\x69\x73\x20\x69\x73\x20\x74\x68\x65\x20\x70\x61\x79\x6c\x6f\x61\x64"));
  compare(module3, "custom", ["this is the payload"]);
  compare(module3, "custom2", ["this is the payload"]);
  console.log(`${passed}/${passed + failed} tests passed`);
}

async function testInstanceConstructor(module) {
  console.log("\nnew WebAssembly.Instance tests");
  const instance = new WebAssembly.Instance(module, imports);
  await testInvalidCases([
    () => WebAssembly.Instance(),
    () => WebAssembly.Instance(module, imports),
    ...(invalidImports.map(i => () => new WebAssembly.Instance(module, i))),
    () => new WebAssembly.Instance(module, overrideImports({fn: 4})),
    () => new WebAssembly.Instance(module, overrideImports({fn2: instance.exports.fn /*wrong signature*/})),
    () => new WebAssembly.Instance(module, overrideImports({memory: 123})),
    () => new WebAssembly.Instance(module, overrideImports({memory: ""})),
    () => new WebAssembly.Instance(module, overrideImports({memory: {}})),
    () => new WebAssembly.Instance(module, overrideImports({table: 123})),
    () => new WebAssembly.Instance(module, overrideImports({table: "135"})),
    () => new WebAssembly.Instance(module, overrideImports({table: {}})),
  ]);
  test(null, instance);
}

async function testMemoryApi(baseModule) {
  console.log("\nWebAssembly.Memory tests");
  await testInvalidCases([
    () => WebAssembly.Memory({initial: 1, maximum: 2}),
    () => new WebAssembly.Memory(),
    () => new WebAssembly.Memory(0),
    () => new WebAssembly.Memory(""),
    () => Reflect.apply(WebAssembly.Memory.prototype.buffer, null, []),
    () => Reflect.apply(WebAssembly.Memory.prototype.buffer, {}, []),
    () => Reflect.apply(WebAssembly.Memory.prototype.buffer, baseModule, []),
    () => Reflect.apply(WebAssembly.Memory.prototype.grow, null, []),
    () => Reflect.apply(WebAssembly.Memory.prototype.grow, {}, []),
    () => Reflect.apply(WebAssembly.Memory.prototype.grow, baseModule, []),
    // todo:: test invalid memory imports, need to find the spec
  ]);
  const toHex = v => typeof v === "number" ? `0x${v.toString(16)}` : "undefined";

  const memory = new WebAssembly.Memory({initial: 1, maximum: 2});
  const {exports: {load}} = new WebAssembly.Instance(baseModule, overrideImports({memory}));
  const v1 = load(1);
  const v2 = load(5);
  const v3 = load(9) & 0xFFFF;
  console.log(`${toHex(v1)} == "0123"`);
  console.log(`${toHex(v2)} == "4567"`);
  console.log(`${toHex(v3)} == "89"`);
  memory.buffer = null; // make sure we can't modify the buffer
  let view = new Int32Array(memory.buffer);
  view[0] = 45;
  console.log(`heap32[0] = ${load(0)}`);

  function testOutOfBounds(index) {
    try {
      load(index);
      console.log("Should have trap with out of bounds error");
    } catch (e) {
      if (e instanceof WebAssembly.RuntimeError) {
        console.log(`Correctly trap on heap access at ${index}`);
      } else {
        console.log(`Unexpected Error: ${e}`);
      }
    }
  }
  const pageSize = 64 * 1024;
  console.log(`memory.buffer.byteLength = ${memory.buffer.byteLength} == ${pageSize}`);
  {
    view[pageSize / 4 - 1] = 0x12345678;
    console.log(`view32[${pageSize / 4 - 1}] = ${toHex(view[pageSize / 4 - 1])}`);
    console.log(`view32[${pageSize / 4}] = ${toHex(view[pageSize / 4])}`);
    console.log(`heap[${pageSize - 4}] = ${toHex(load(pageSize - 4))}`);
    testOutOfBounds(pageSize - 3);
    testOutOfBounds(pageSize - 2);
    testOutOfBounds(pageSize - 1);
    testOutOfBounds(pageSize);
  }
  console.log("grow by 1 page");
  memory.grow(1);
  console.log(`memory.buffer.byteLength = ${memory.buffer.byteLength} == 2 * ${pageSize}`);
  view = new Int32Array(memory.buffer); // buffer has been detached, reset the view
  {
    view[2 * pageSize / 4 - 1] = 0x87654321;
    console.log(`view32[${2 * pageSize / 4 - 1}] = ${toHex(view[2 * pageSize / 4 - 1])}`);
    console.log(`view32[${2 * pageSize / 4}] = ${toHex(view[2 * pageSize / 4])}`);
    console.log(`heap[${pageSize - 4}] = ${toHex(load(pageSize - 4))}`);
    console.log(`heap[${2 * pageSize - 4}] = ${toHex(load(2 * pageSize - 4))}`);
    testOutOfBounds(2 * pageSize - 3);
    testOutOfBounds(2 * pageSize - 2);
    testOutOfBounds(2 * pageSize - 1);
    testOutOfBounds(2 * pageSize);
  }
  try {
    memory.grow(1);
    console.log("Should have trap when growing past maximum");
  } catch (e) {
    if (e instanceof RangeError) {
      console.log("Correctly trap when growing past maximum");
    } else {
      console.log(`Unexpected Error: ${e}`);
    }
  }
}

async function testTableApi(baseModule) {
  //new WebAssembly.Table({element: "anyfunc", initial: 30, maximum: 100})
  const table = new WebAssembly.Table({element: "anyfunc", initial: 30, maximum: 100});
  console.log("\nWebAssembly.Table tests");
  await testInvalidCases([
    () => WebAssembly.Table({element: "anyfunc", initial: 30, maximum: 100}),
    () => new WebAssembly.Table(),
    () => new WebAssembly.Table(0),
    () => new WebAssembly.Table(""),
    () => new WebAssembly.Table({element: "notanyfunc", initial: 30, maximum: 100}),
    () => Reflect.apply(WebAssembly.Table.prototype.length, null, []),
    () => Reflect.apply(WebAssembly.Table.prototype.length, {}, []),
    () => Reflect.apply(WebAssembly.Table.prototype.length, baseModule, []),
    () => Reflect.apply(WebAssembly.Table.prototype.grow, null, []),
    () => Reflect.apply(WebAssembly.Table.prototype.grow, {}, []),
    () => Reflect.apply(WebAssembly.Table.prototype.grow, baseModule, []),
    () => Reflect.apply(WebAssembly.Table.prototype.get, null, [0]),
    () => Reflect.apply(WebAssembly.Table.prototype.get, {}, [0]),
    () => Reflect.apply(WebAssembly.Table.prototype.get, baseModule, [0]),
    () => table.get(30),
    () => table.get(100),
    () => Reflect.apply(WebAssembly.Table.prototype.get, table, [30]),
    () => Reflect.apply(WebAssembly.Table.prototype.get, table, [100]),
  ]);

  const {instance: {exports: {"f32.load": load}}} = await WebAssembly.instantiate(readbuffer("f32address.wasm"), {});
  const {exports: {fn, call_i32, call_f32}} = new WebAssembly.Instance(baseModule, overrideImports({table, fn2: load}));
  const {exports: {fn: myFn}} = new WebAssembly.Instance(baseModule, overrideImports({fn: () => 123456}));
  await testInvalidCases([
    () => table.set(30, fn),
    () => table.set(100, fn),
    () => table.set(0),
    () => table.set(0, {}),
    () => table.set(0, ""),
    () => table.set(0, 123),
    () => table.set(0, function(){}),
    () => table.set(0, () => 2),
    () => call_i32(0),
    () => call_f32(0),
    () => call_i32(29),
    () => call_i32(30),
    () => call_i32(2),
    () => call_f32(1),
  ]);

  console.log(`Current length: ${table.length}`);
  table.length = 456;
  console.log(`Length after attempt to modify : ${table.length}`);
  console.log(`Is element in table the same as the one exported: ${table.get(1) === fn}`);
  console.log(`Unset element should be null: ${table.get(0)}`);
  table.set(0, myFn);
  console.log(`call_i32(0): ${call_i32(0)}`);
  console.log(`call_i32(1): ${call_i32(1)}`);
  console.log(`call_f32(2): ${call_f32(2)}`);
  table.set(0, fn);
  console.log(`call_i32(0): ${call_i32(0)}`);
  table.set(29, myFn);
  table.grow(1);
  table.set(30, myFn);
  console.log(`call_i32(29): ${call_i32(29)}`);
  console.log(`call_i32(30): ${call_i32(30)}`);

  const table2 = new WebAssembly.Table({element: "anyfunc"});
  table.grow(0);
  try {table2.get(0); console.log("Failed. Unexpected successfull call to table2.get(0)");} catch (e) {}
  table.grow(1);
  table.set(0, myFn);
  console.log(`table2[0](): ${table.get(0)()}`);
}

async function main() {
  const args = WScript.Arguments;
  verbose = args.includes("-v");
  doInvalid = !args.includes("-i");
  const [start = 0, end = 99] = args
    .filter(a => !a.startsWith("-"))
    .map(a => parseInt(a));

  const baseModule = new WebAssembly.Module(buf);
  const tests = [
    /*0*/ testValidate,
    /*1*/ testCompile,
    /*2*/ testInstantiate,
    /*3*/ testModuleConstructor,
    /*4*/ testModuleApi,
    /*5*/ testModuleCustomSection,
    /*6*/ testInstanceConstructor,
    /*7*/ testMemoryApi,
    /*8*/ testTableApi,
  ];
  for (let i = 0; i < tests.length; ++i) {
    if (i >= start && i <= end) {
      try {
        await tests[i](baseModule);
      } catch (e) {
        console.log("Unexpected error");
        console.log(e.stack);
      }
    }
  }
}

main().then(() => console.log("done"), err => {
  console.log(err);
  console.log(err.stack);
});
