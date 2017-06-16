//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// This file has been modified by Microsoft on [12/2016].

/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const cliArgs = WScript.Arguments || [];
WScript.Flag("-wasmI64");

if (cliArgs.length < 1) {
  print("usage: <exe> spec.js -args <filename.json> [start index] [end index] [-verbose] [-nt] -endargs");
  WScript.Quit(0);
}

if (typeof IMPORTS_FROM_OTHER_SCRIPT === "undefined") {
  IMPORTS_FROM_OTHER_SCRIPT = {};
}

let passed = 0;
let failed = 0;

// Parse arguments
const iVerbose = cliArgs.indexOf("-verbose");
const verbose = iVerbose !== -1;
if (verbose) {
  cliArgs.splice(iVerbose, 1);
}
const iNoTrap = cliArgs.indexOf("-nt");
const noTrap = iNoTrap !== -1;
const trap = !noTrap;
if (noTrap) {
  cliArgs.splice(iNoTrap, 1);
}

let file = "";
let iTest = 0;
function getValueStr(value) {
  if (typeof value === "object") {
    const {high, low} = value;
    const convert = a => (a >>> 0).toString(16).padStart(8, "0");
    return `0x${convert(high)}${convert(low)}`;
  }
  return "" + value;
}

function getArgsStr(args) {
  return args
    .map(({type, value}) => `${type}:${getValueStr(mapWasmArg({type, value}))}`)
    .join(", ");
}

function getActionStr(action) {
  const moduleName = action.module || "$$";
  switch (action.type) {
    case "invoke": return `${moduleName}.${action.field}(${getArgsStr(action.args)})`;
    case "get": return `${moduleName}[${action.field}]`;
    default: return "Unkown action type";
  }
}

function getCommandStr(command) {
  const base = `(${iTest}) ${file}:${command.line}`;
  switch (command.type) {
    case "module": return `${base}: generate module ${command.name ? ` as ${command.name}` : ""}`;
    case "register": return `${base}: register module ${command.name || "$$"} as ${command.as}`;
    case "assert_malformed":
    case "assert_unlinkable":
    case "assert_uninstantiable":
    case "assert_invalid": return `${base}: ${command.type} module`;
    case "assert_return": return `${base}: assert_return(${getActionStr(command.action)} == ${getArgsStr(command.expected)})`;
    case "action":
    case "assert_trap":
    case "assert_return_nan":
    case "assert_exhaustion":
      return `${base}: ${command.type}(${getActionStr(command.action)})`;
  }
  return base;
}

function run(inPath, iStart, iEnd) {
  const lastSlash = Math.max(inPath.lastIndexOf("/"), inPath.lastIndexOf("\\"));
  const inDir = lastSlash === -1 ? "." : inPath.slice(0, lastSlash);
  file = inPath;
  const data = read(inPath);
  const {commands} = WebAssembly.wabt.convertWast2Wasm(data, {spec: true});

  const registry = Object.assign({spectest: {
    print,
    global: 666,
    table: new WebAssembly.Table({initial: 10, maximum: 20, element: "anyfunc"}),
    memory: new WebAssembly.Memory({initial: 1, maximum: 2})
  }}, IMPORTS_FROM_OTHER_SCRIPT);

  const moduleRegistry = {};
  moduleRegistry.currentModule = null;

  for (const command of commands) {
    ++iTest;
    if (iTest < iStart) {
      // always run module/register commands that happens before iStart
      if (command.type !== "module" && command.type !== "register") {
        continue;
      }
    } else if (iTest > iEnd) {
      continue;
    }

    if (verbose) {
      print(`Running:${getCommandStr(command)}`);
    }

    switch (command.type) {
      case "module":
        moduleCommand(inDir, command, registry, moduleRegistry);
        break;

      case "assert_malformed":
        assertMalformed(inDir, command);
        break;

      case "assert_unlinkable":
        assertUnlinkable(inDir, command, registry);
        break;

      case "assert_uninstantiable":
        assertUninstantiable(inDir, command, registry);
        break;

      case "assert_invalid":
        assertMalformed(inDir, command, registry);
        break;

      case "register": {
        const {as, name = "currentModule"} = command;
        registry[as] = moduleRegistry[name].exports;
        break;
      }

      case "action":
        runSimpleAction(moduleRegistry, command);
        break;

      case "assert_return":
        assertReturn(moduleRegistry, command);
        break;

      case "assert_return_canonical_nan":
        assertReturn(moduleRegistry, command, {canonicalNan: true});
        break;

      case "assert_return_arithmetic_nan":
        assertReturn(moduleRegistry, command, {arithmeticNan: true});
        break;

      case "assert_exhaustion":
        assertStackExhaustion(moduleRegistry, command);
        break;

      case "assert_trap":
        if (trap) {
          assertTrap(moduleRegistry, command);
        } else {
          print(`${getCommandStr(command)} skipped because it has traps`);
        }
        break;
      default:
        print(`Unknown command: ${JSON.stringify(command)}`);
    }
  }
  end();
}

function createModule(baseDir, buffer, registry, output) {
  if (verbose) {
    const u8a = new Uint8Array(buffer);
    console.log(u8a);
  }
  output.module = new WebAssembly.Module(buffer);
  // We'll know if an error occurs at instanciation because output.module will be set
  output.instance = new WebAssembly.Instance(output.module, registry);
}

function moduleCommand(baseDir, command, registry, moduleRegistry) {
  const {buffer, name} = command;
  try {
    const output = {};
    createModule(baseDir, buffer, registry, output);
    if (name) {
      moduleRegistry[name] = output.instance;
    }
    moduleRegistry.currentModule = output.instance;
    ++passed;
  } catch (e) {
    ++failed;
    print(`${getCommandStr(command)} failed. Unexpected Error: ${e}`);
  }
  return null;
}

function assertMalformed(baseDir, command) {
  const {buffer, text} = command;
  // Test hook to prevent deferred parsing
  WScript.Flag("-off:wasmdeferred");
  const output = {};
  try {
    createModule(baseDir, buffer, null, output);
    ++failed;
    print(`${getCommandStr(command)} failed. Should have had an error`);
  } catch (e) {
    if (output.module) {
      ++failed;
      print(`${getCommandStr(command)} failed. Had a linking error, expected a compile error: ${e}`);
    } else if (e instanceof WebAssembly.CompileError) {
      ++passed;
      if (verbose) {
        print(`${getCommandStr(command)} passed. Had compile error: ${e}`);
        print(`  Spec expected error: ${text}`);
      }
    } else {
      ++failed;
      print(`${getCommandStr(command)} failed. Expected a compile error: ${e}`);
    }
  } finally {
    // Reset the test hook
    WScript.Flag("-on:wasmdeferred");
  }
}

function assertUnlinkable(baseDir, command, registry) {
  const {buffer, text} = command;
  // Test hook to prevent deferred parsing
  WScript.Flag("-off:wasmdeferred");
  const output = {};
  try {
    createModule(baseDir, buffer, registry, output);
    ++failed;
    print(`${getCommandStr(command)} failed. Should have had an error`);
  } catch (e) {
    if (e instanceof WebAssembly.LinkError) {
      ++passed;
      if (verbose) {
        print(`${getCommandStr(command)} passed. Had linking error: ${e}`);
        print(`  Spec expected error: ${text}`);
      }
    } else {
      ++failed;
      print(`${getCommandStr(command)} failed. Expected a linking error: ${e}`);
    }
  } finally {
    // Reset the test hook
    WScript.Flag("-on:wasmdeferred");
  }
}

function assertUninstantiable(baseDir, command, registry) {
  const {buffer, text} = command;
  // Test hook to prevent deferred parsing
  WScript.Flag("-off:wasmdeferred");
  const output = {};
  try {
    createModule(baseDir, buffer, registry, output);
    ++failed;
    print(`${getCommandStr(command)} failed. Should have had an error`);
  } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) {
      ++passed;
      if (verbose) {
        print(`${getCommandStr(command)} passed. Had instanciation error: ${e}`);
        print(`  Spec expected error: ${text}`);
      }
    } else {
      ++failed;
      print(`${getCommandStr(command)} failed. Expected a instanciation error: ${e}`);
    }
  } finally {
    // Reset the test hook
    WScript.Flag("-on:wasmdeferred");
  }
}

function genConverters() {
  const buffer = WebAssembly.wabt.convertWast2Wasm(`
(module
  (func (export "convertI64") (param i64) (result i64) (get_local 0))
  (func (export "toF32") (param i32) (result f32) (f32.reinterpret/i32 (get_local 0)))
  (func (export "toF64") (param i64) (result f64) (f64.reinterpret/i64 (get_local 0)))
)`);
  const module = new WebAssembly.Module(buffer);
  const instance = new WebAssembly.Instance(module);
  return instance.exports;
}

const {toF32, toF64, convertI64} = genConverters();

function mapWasmArg({type, value}) {
  switch (type) {
    case "i32": return parseInt(value)|0;
    case "i64": return convertI64(value); // will convert string to {high, low}
    case "f32": return toF32(parseInt(value));
    case "f64": return toF64(value);
  }
  throw new Error("Unknown argument type");
}

const wrappers = {};

function getArthimeticNanWrapper(action, expected) {
  if (action.type === "invoke") {
    const args = action.args.map(({type}) => type);
    const resultType = expected[0].type;
    const signature = resultType + args.join("");

    let wasmModule = wrappers[signature];
    if (!wasmModule) {
      const resultSize = resultType === "f32" ? 32 : 64;
      const matchingIntType = resultSize === 32 ? "i32" : "i64";
      const expectedResult = resultSize === 32 ? "0x7f800000" : "0x7ff0000000000000";

      const params = args.length > 0 ? `(param ${args.join(" ")})` : "";
      const newMod = `
(module
  (import "test" "fn" (func $fn ${params} (result ${resultType})))
  (func (export "compare") ${params} (result i32)
    ${args.map((arg, i) => `(get_local ${i|0})`).join(" ")}
    (call $fn)
    (${matchingIntType}.reinterpret/${resultType})
    (${matchingIntType}.and (${matchingIntType}.const ${expectedResult}))
    (${matchingIntType}.eq (${matchingIntType}.const ${expectedResult}))
  )
)`;
      if (verbose) {
        console.log(newMod);
      }
      const buf = WebAssembly.wabt.convertWast2Wasm(newMod);
      wrappers[signature] = wasmModule = new WebAssembly.Module(buf);
    }

    return (fn, ...args) => {
      const {exports: {compare}} = new WebAssembly.Instance(wasmModule, {test: {fn}});
      return compare(...args);
    }
  }
}

function assertReturn(moduleRegistry, command, {canonicalNan, arithmeticNan} = {}) {
  const {action, expected} = command;
  try {
    const wrapper = null; // arithmeticNan ? getArthimeticNanWrapper(action, expected) : null;
    const res = runAction(moduleRegistry, action, wrapper);
    let success = true;
    if (expected.length === 0) {
      success = typeof res === "undefined";
    } else {
      // We don't support multi return right now
      const [ex1] = expected;
      const expectedResult = mapWasmArg(ex1);
      if (ex1.type === "i64") {
        success = expectedResult.low === res.low && expectedResult.high === res.high;
      } else if (arithmeticNan || canonicalNan || isNaN(expectedResult)) {
        // todo:: do exact compare for nan once bug resolved
        success = isNaN(res);
      } else {
        success = res === expectedResult;
      }
    }

    if (success) {
      passed++;
      if (verbose) {
        print(`${getCommandStr(command)} passed.`);
      }
    } else {
      print(`${getCommandStr(command)} failed. Returned ${getValueStr(res)}`);
      failed++;
    }
  } catch (e) {
    print(`${getCommandStr(command)} failed. Unexpectedly threw: ${e}`);
    failed++;
  }
}

function assertTrap(moduleRegistry, command) {
  const {action, text} = command;
  try {
    runAction(moduleRegistry, action);
    print(`${getCommandStr(command)} failed. Should have had an error`);
    failed++;
  } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) {
      passed++;
      if (verbose) {
        print(`${getCommandStr(command)} passed. Error thrown: ${e}`);
        print(`  Spec error message expected: ${text}`);
      }
    } else {
      failed++;
      print(`${getCommandStr(command)} failed. Unexpected error thrown: ${e}`);
    }
  }
}

let StackOverflow;
function assertStackExhaustion(moduleRegistry, command) {
  if (!StackOverflow) {
    // Get the stack overflow exception type
    // Javascript and WebAssembly must throw the same type of error
    try { (function f() { 1 + f() })() } catch (e) { StackOverflow = e.constructor }
  }

  const {action, text} = command;
  try {
    runAction(moduleRegistry, action);
    print(`${getCommandStr(command)} failed. Should have exhausted the stack`);
    failed++;
  } catch (e) {
    if (e instanceof StackOverflow) {
      passed++;
      if (verbose) {
        print(`${getCommandStr(command)} passed. Had a StackOverflow`);
      }
    } else {
      failed++;
      print(`${getCommandStr(command)} failed. Unexpected error thrown: ${e}`);
    }
  }
}


function runSimpleAction(moduleRegistry, command) {
  try {
    const res = runAction(moduleRegistry, command.action);
    ++passed;
    if (verbose) {
      print(`${getCommandStr(command)} = ${res}`);
    }
  } catch (e) {
    ++failed;
    print(`${getCommandStr(command)} failed. Unexpectedly threw: ${e}`);
  }
}

function runAction(moduleRegistry, action, wrapper) {
  const m = action.module ? moduleRegistry[action.module] : moduleRegistry.currentModule;
  if (!m) {
    print("Module unavailable to run action");
    return;
  }
  switch (action.type) {
    case "invoke": {
      const {field, args} = action;
      const mappedArgs = args.map(({value}) => value);
      let wasmFn = m.exports[field];
      if (wrapper) {
        wasmFn = wrapper.bind(null, wasmFn);
      }
      const res = wasmFn(...mappedArgs);
      return res;
    }
    case "get": {
      const {field} = action;
      if (wrapper) {
        return wrapper(m, field);
      }
      return m.exports[field];
    }
    default:
      print(`Unknown action: ${JSON.stringify(action)}`);
  }
}

function end() {
  print(`${passed}/${passed + failed} tests passed.`);
}

run(cliArgs[0], cliArgs[1] | 0, cliArgs[2] === undefined ? undefined : cliArgs[2] | 0);
