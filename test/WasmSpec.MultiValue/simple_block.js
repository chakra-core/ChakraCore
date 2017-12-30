'use strict';

let spectest = {
  print: print || ((...xs) => console.log(...xs)),
  global: 666,
  table: new WebAssembly.Table({initial: 10, maximum: 20, element: 'anyfunc'}),  memory: new WebAssembly.Memory({initial: 1, maximum: 2}),};

let registry = {spectest};

function register(name, instance) {
  registry[name] = instance.exports;
}

function module(bytes, valid = true) {
  let buffer = new ArrayBuffer(bytes.length);
  let view = new Uint8Array(buffer);
  for (let i = 0; i < bytes.length; ++i) {
    view[i] = bytes.charCodeAt(i);
  }
  let validated;
  try {
    validated = WebAssembly.validate(buffer);
  } catch (e) {
    throw new Error("Wasm validate throws");
  }
  if (validated !== valid) {
    throw new Error("Wasm validate failure" + (valid ? "" : " expected"));
  }
  return new WebAssembly.Module(buffer);
}

function instance(bytes, imports = registry) {
  return new WebAssembly.Instance(module(bytes), imports);
}

function call(instance, name, args) {
  return instance.exports[name](...args);
}

function get(instance, name) {
  return instance.exports[name];
}

function exports(name, instance) {
  return {[name]: instance.exports};
}

function run(action) {
  action();
}

function assert_malformed(bytes) {
  try { module(bytes, false) } catch (e) {
    if (e instanceof WebAssembly.CompileError) return;
  }
  throw new Error("Wasm decoding failure expected");
}

function assert_invalid(bytes) {
  try { module(bytes, false) } catch (e) {
    if (e instanceof WebAssembly.CompileError) return;
  }
  throw new Error("Wasm validation failure expected");
}

function assert_unlinkable(bytes) {
  let mod = module(bytes);
  try { new WebAssembly.Instance(mod, registry) } catch (e) {
    if (e instanceof WebAssembly.LinkError) return;
  }
  throw new Error("Wasm linking failure expected");
}

function assert_uninstantiable(bytes) {
  let mod = module(bytes);
  try { new WebAssembly.Instance(mod, registry) } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) return;
  }
  throw new Error("Wasm trap expected");
}

function assert_trap(action) {
  try { action() } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) return;
  }
  throw new Error("Wasm trap expected");
}

let StackOverflow;
try { (function f() { 1 + f() })() } catch (e) { StackOverflow = e.constructor }

function assert_exhaustion(action) {
  try { action() } catch (e) {
    if (e instanceof StackOverflow) return;
  }
  throw new Error("Wasm resource exhaustion expected");
}

function assert_return(action, expected) {
  let actual = action();
  if (!Object.is(actual, expected)) {
    throw new Error("Wasm return value " + expected + " expected, got " + actual);
  };
}

function assert_return_canonical_nan(action) {
  let actual = action();
  // Note that JS can't reliably distinguish different NaN values,
  // so there's no good way to test that it's a canonical NaN.
  if (!Number.isNaN(actual)) {
    throw new Error("Wasm return value NaN expected, got " + actual);
  };
}

function assert_return_arithmetic_nan(action) {
  // Note that JS can't reliably distinguish different NaN values,
  // so there's no good way to test for specific bitpatterns here.
  let actual = action();
  if (!Number.isNaN(actual)) {
    throw new Error("Wasm return value NaN expected, got " + actual);
  };
}


// simple_block.wast:6
let $1 = instance("\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x8b\x80\x80\x80\x00\x02\x60\x00\x01\x7f\x60\x02\x7f\x7f\x01\x7f\x03\x82\x80\x80\x80\x00\x01\x00\x07\x8a\x80\x80\x80\x00\x01\x06\x70\x61\x72\x61\x6d\x73\x00\x00\x0a\x90\x80\x80\x80\x00\x01\x8a\x80\x80\x80\x00\x00\x41\x01\x41\x02\x02\x01\x6a\x0b\x0b");

// simple_block.wast:15
assert_return(() => call($1, "params", []), 3);

// simple_block.wast:17
let $2 = instance("\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x8c\x80\x80\x80\x00\x02\x60\x00\x01\x7f\x60\x02\x7f\x7f\x02\x7f\x7f\x03\x82\x80\x80\x80\x00\x01\x00\x07\x8d\x80\x80\x80\x00\x01\x09\x70\x61\x72\x61\x6d\x73\x2d\x69\x64\x00\x00\x0a\x90\x80\x80\x80\x00\x01\x8a\x80\x80\x80\x00\x00\x41\x01\x41\x02\x02\x01\x0b\x6a\x0b");

// simple_block.wast:25
assert_return(() => call($2, "params-id", []), 3);

// simple_block.wast:27
let $3 = instance("\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x8a\x80\x80\x80\x00\x02\x60\x00\x01\x7f\x60\x00\x02\x7f\x7f\x03\x82\x80\x80\x80\x00\x01\x00\x07\x8b\x80\x80\x80\x00\x01\x07\x72\x65\x73\x75\x6c\x74\x73\x00\x00\x0a\x90\x80\x80\x80\x00\x01\x8a\x80\x80\x80\x00\x00\x02\x01\x41\x01\x41\x02\x0b\x6a\x0b");

// simple_block.wast:36
assert_return(() => call($3, "results", []), 3);

// simple_block.wast:38
let $4 = instance("\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x8c\x80\x80\x80\x00\x02\x60\x00\x01\x7f\x60\x02\x7f\x7c\x02\x7f\x7f\x03\x82\x80\x80\x80\x00\x01\x00\x07\x8e\x80\x80\x80\x00\x01\x0a\x6d\x75\x6c\x74\x69\x2d\x74\x79\x70\x65\x00\x00\x0a\x98\x80\x80\x80\x00\x01\x92\x80\x80\x80\x00\x00\x41\x01\x44\x00\x00\x00\x00\x00\x00\x00\x40\x02\x01\xaa\x0b\x6a\x0b");

// simple_block.wast:48
assert_return(() => call($4, "multi-type", []), 3);

console.log("pass");