//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// This file has been modified by Microsoft on [07/2016].

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

/* polyfill from SM/CH to D8 */
if (typeof arguments == 'undefined') {
  if (typeof scriptArgs != 'undefined') {
    arguments = scriptArgs;
  } else if(typeof WScript != 'undefined') {
    arguments = WScript.Arguments || [];
  }
}

if (typeof quit == 'undefined') {
  if (typeof WScript != 'undefined') {
    quit = WScript.quit;
  }
}

if (typeof readbuffer == 'undefined') {
  readbuffer = function(path) { return read(path, 'binary'); };
}

if (arguments.length < 1) {
  print('usage: <exe> spec.js -args <filename.json> [start index] [end index] [-v] -endargs');
  quit(0);
}

if (typeof IMPORTS_FROM_OTHER_SCRIPT === "undefined") {
  IMPORTS_FROM_OTHER_SCRIPT = {};
}

var passed = 0;
var failed = 0;
const iVerbose = arguments.indexOf("-v");
var verbose = iVerbose !== -1;
if (verbose) {
  arguments.splice(iVerbose, 1)
}
const iTrap = arguments.indexOf("-t");
var trap = iTrap !== -1;
if (trap) {
  arguments.splice(iTrap, 1)
}
run(arguments[0], arguments[1]|0, arguments[2] === undefined ? undefined : arguments[2]|0);

function getCommandStr({file, line, name}) {
  return `${file}:${line}: ${name}`;
}

function run(inPath, iStart, iEnd) {
  var lastSlash = Math.max(inPath.lastIndexOf('/'), inPath.lastIndexOf('\\'));
  var inDir = lastSlash == -1 ? '.' : inPath.slice(0, lastSlash);
  var data = read(inPath);
  var jsonData = JSON.parse(data);

  var iTest = 0;
  var registry = Object.assign({spectest: {print: print, global : 666}}, IMPORTS_FROM_OTHER_SCRIPT);
  for (var i = 0; i < jsonData.modules.length; ++i) {
    var module = jsonData.modules[i] || {};
    try {
      var moduleFile = readbuffer(inDir + '/' + module.filename);
      var m = createModule(moduleFile, registry);
      for (var j = 0; j < module.commands.length; ++j, ++iTest) {
        var command = module.commands[j];
        if (iTest < iStart || iTest > iEnd) {
          if (verbose) {
            print(`${getCommandStr(command)}: Skipped`);
          }
          continue;
        }
        switch (command.type) {
          case 'register':
            registry[command.name] = m.exports;
            break;
          case 'action':
            invoke(m, command);
            break;

          case 'assert_return':
            assertReturn(m, command);
            break;

          case 'assert_return_nan':
            assertReturn(m, command);
            break;

          case 'assert_trap':
            if (trap) {
              assertTrap(m, command);
            } else {
              failed++;
              print(`${getCommandStr(command)} failed, runtime trap NYI`);
            }
            break;
        }
      }
    } catch (e) {
      print("Unexpected Error while compiling " + module.filename);
      print(e);
    }
  }
  end();
}

function createModule(a, registry) {
  var memory = null;
  var u8a = new Uint8Array(a);
  var module = Wasm.instantiateModule(u8a, registry);
  memory = module.memory;
  return module;
}

function assertReturn(m, command) {
  const {file, line, name} = command;
  try {
    var result = m.exports[name]();
  } catch(e) {
    print(`${getCommandStr(command)} unexpectedly threw: ${e}`);
    failed++;
    return;
  }

  if (result == 1) {
    passed++;
    if (verbose) {
      print(`${getCommandStr(command)} passed.`);
    }
  } else {
    print(`${getCommandStr(command)} failed.`);
    failed++;
  }
}

function assertTrap(m, command) {
  const {file, line, name} = command;
  var threw = false;
  var error;
  try {
    m.exports[name]();
  } catch (e) {
    error = e;
    threw = true;
  }

  if (threw) {
    passed++;
    if (verbose) {
      print(`${getCommandStr(command)} passed. Error thrown: ${error}`);
    }
  } else {
    print(`${getCommandStr(command)} failed, didn't throw`);
    failed++;
  }
}

function invoke(m, command) {
  const {file, line, name} = command;
  try {
    var invokeResult = m.exports[name]();
  } catch(e) {
    print(`${getCommandStr(command)} unexpectedly threw: ${e}`);
  }

  print(name + " = " + invokeResult);
}

function end() {
  print(passed + "/" + (passed + failed) + " tests passed.");
}
