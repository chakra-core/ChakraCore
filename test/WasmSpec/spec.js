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
  print("usage: <exe> spec.js -args <filename.json> [start index] [end index] [-v] [-nt] -endargs");
  WScript.quit(0);
}

if (typeof IMPORTS_FROM_OTHER_SCRIPT === "undefined") {
  IMPORTS_FROM_OTHER_SCRIPT = {};
}

let passed = 0;
let failed = 0;

// Parse arguments
const iVerbose = cliArgs.indexOf("-v");
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
  switch (action.type) {
    case "invoke": return `${action.field}(${getArgsStr(action.args)})`;
    case "get": return `${action.module || "$$"}[${action.field}]`;
    default: return "Unkown action type";
  }
}

function getCommandStr(command) {
  const base = `(${iTest}) ${file}:${command.line}`;
  switch (command.type) {
    case "module": return `${base}: generate module ${command.filename}${command.name ? ` as ${command.name}` : ""}`;
    case "register": return `${base}: register module ${command.name || "$$"} as ${command.as}`;
    case "assert_malformed": return `${base}: assert_malformed module ${command.filename}`;
    case "assert_unlinkable": return `${base}: assert_unlinkable module ${command.filename}`;
    case "assert_invalid": return `${base}: assert_invalid module ${command.filename}`;
    case "action": return `${base}: action ${getActionStr(command.action)}`;
    case "assert_trap": return `${base}: assert_trap ${getActionStr(command.action)}`;
    case "assert_return": return `${base}: assert_return ${getActionStr(command.action)} == ${getArgsStr(command.expected)}`;
    case "assert_return_nan": return `${base}: assert_return_nan ${getActionStr(command.action)}`;
  }
  return base;
}

function run(inPath, iStart, iEnd) {
  const lastSlash = Math.max(inPath.lastIndexOf("/"), inPath.lastIndexOf("\\"));
  const inDir = lastSlash === -1 ? "." : inPath.slice(0, lastSlash);
  const data = read(inPath);
  const jsonData = JSON.parse(data);
  file = jsonData.source_filename;

  const registry = Object.assign({spectest: {
    print,
    global: 666,
    table: new WebAssembly.Table({initial: 10, maximum: 20, element: "anyfunc"}),
    memory: new WebAssembly.Memory({initial: 1, maximum: 2})
  }}, IMPORTS_FROM_OTHER_SCRIPT);

  const moduleRegistry = {};
  moduleRegistry.currentModule = null;

  for (const command of jsonData.commands) {
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

      case "assert_return_nan":
        assertReturn(moduleRegistry, command, true);
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

function createModule(baseDir, filename, registry, output) {
  const moduleFile = readbuffer(baseDir + "/" + filename);
  const u8a = new Uint8Array(moduleFile);
  output.module = new WebAssembly.Module(u8a);
  // We'll know if an error occurs at instanciation because output.module will be set
  output.instance = new WebAssembly.Instance(output.module, registry);
}

function moduleCommand(baseDir, command, registry, moduleRegistry) {
  const {filename, name} = command;
  try {
    const output = {};
    createModule(baseDir, filename, registry, output);
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
  const {filename, text} = command;
  // Test hook to prevent deferred parsing
  WScript.Flag("-off:wasmdeferred");
  const output = {};
  try {
    createModule(baseDir, filename, null, output);
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
  const {filename, text} = command;
  // Test hook to prevent deferred parsing
  WebAssembly.Module.prototype.deferred = false;
  const output = {};
  try {
    createModule(baseDir, filename, registry, output);
    ++failed;
    print(`${getCommandStr(command)} failed. Should have had an error`);
  } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) {
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
    WebAssembly.Module.prototype.deferred = undefined;
  }
}

function genConverters() {
  /*
  (module $converterBuffer
    (func (export "convertI64") (param i64) (result i64) (get_local 0))
    (func (export "toF32") (param i32) (result f32) (f32.reinterpret/i32 (get_local 0)))
    (func (export "toF64") (param i64) (result f64) (f64.reinterpret/i64 (get_local 0)))
  )
  */
  const converterBuffer = "\x00\x61\x73\x6d\x0d\x00\x00\x00\x01\x90\x80\x80\x80\x00\x03\x60\x01\x7e\x01\x7e\x60\x01\x7f\x01\x7d\x60\x01\x7e\x01\x7c\x03\x84\x80\x80\x80\x00\x03\x00\x01\x02\x07\x9e\x80\x80\x80\x00\x03\x0a\x63\x6f\x6e\x76\x65\x72\x74\x49\x36\x34\x00\x00\x05\x74\x6f\x46\x33\x32\x00\x01\x05\x74\x6f\x46\x36\x34\x00\x02\x0a\x9e\x80\x80\x80\x00\x03\x84\x80\x80\x80\x00\x00\x20\x00\x0b\x85\x80\x80\x80\x00\x00\x20\x00\xbe\x0b\x85\x80\x80\x80\x00\x00\x20\x00\xbf\x0b";
  const buffer = new ArrayBuffer(converterBuffer.length);
  const view = new Uint8Array(buffer);
  for (let i = 0; i < converterBuffer.length; ++i) {
    view[i] = converterBuffer.charCodeAt(i);
  }
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

function assertReturn(moduleRegistry, command, checkNaN) {
  const {action, expected} = command;
  try {
    const res = runAction(moduleRegistry, action);
    let success = true;
    if (expected.length === 0) {
      success = typeof res === "undefined";
    } else {
      // We don't support multi return right now
      const [ex1] = expected;
      const expectedResult = mapWasmArg(ex1);
      if (ex1.type === "i64") {
        success = expectedResult.low === res.low && expectedResult.high === res.high;
      } else if (checkNaN || isNaN(expectedResult)) {
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
    if (e instanceof WebAssembly.RuntimeError || e.message.indexOf("Out of stack") !== -1) {
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

function runAction(moduleRegistry, action) {
  const m = action.module ? moduleRegistry[action.module] : moduleRegistry.currentModule;
  if (!m) {
    print("Module unavailable to run action");
    return;
  }
  switch (action.type) {
    case "invoke": {
      const {field, args} = action;
      const mappedArgs = args.map(({value}) => value);
      const wasmFn = m.exports[field];
      const res = wasmFn(...mappedArgs);
      return res;
    }
    case "get": {
      const {field} = action;
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
