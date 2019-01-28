/*
 * Copyright 2018 WebAssembly Community Group participants
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

let kJSEmbeddingMaxTypes = 1000000;
let kJSEmbeddingMaxFunctions = 1000000;
let kJSEmbeddingMaxImports = 100000;
let kJSEmbeddingMaxExports = 100000;
let kJSEmbeddingMaxGlobals = 1000000;
let kJSEmbeddingMaxDataSegments = 100000;

let kJSEmbeddingMaxMemoryPages = 65536;
let kJSEmbeddingMaxModuleSize = 1024 * 1024 * 1024;  // = 1 GiB
let kJSEmbeddingMaxFunctionSize = 7654321;
let kJSEmbeddingMaxFunctionLocals = 50000;
let kJSEmbeddingMaxFunctionParams = 1000;
let kJSEmbeddingMaxFunctionReturns = 1;
let kJSEmbeddingMaxTableSize = 10000000;
let kJSEmbeddingMaxElementSegments = 10000000;
let kJSEmbeddingMaxTables = 1;
let kJSEmbeddingMaxMemories = 1;

function verbose(...args) {
  if (false) print(...args);
}

let kTestValidate = true;
let kTestSyncCompile = true;
let kTestAsyncCompile = true;

//=======================================================================
// HARNESS SNIPPET, DO NOT COMMIT
//=======================================================================
const known_failures = {
  // Enter failing tests like follows:
  // "'WebAssembly.Instance.prototype.exports' accessor property":
  //  'https://bugs.chromium.org/p/v8/issues/detail?id=5507',
};

let failures = [];
let unexpected_successes = [];

let last_promise = new Promise((resolve, reject) => { resolve(); });

function test(func, description) {
  let maybeErr;
  try { func(); }
  catch(e) { maybeErr = e; }
  if (typeof maybeErr !== 'undefined') {
    var known = "";
    if (known_failures[description]) {
      known = " (known)";
    }
    print(`${description}: FAIL${known}. ${maybeErr}`);
    failures.push(description);
  } else {
    if (known_failures[description]) {
      unexpected_successes.push(description);
    }
    print(`${description}: PASS.`);
  }
}

function promise_test(func, description) {
  last_promise = last_promise.then(func)
  .then(_ => {
    if (known_failures[description]) {
      unexpected_successes.push(description);
    }
    print(`${description}: PASS.`);
  })
  .catch(err => {
    var known = "";
    if (known_failures[description]) {
      known = " (known)";
    }
    print(`${description}: FAIL${known}. ${err}`);
    failures.push(description);
  });
}
//=======================================================================

// This function runs the {gen} function with the values {min}, {limit}, and
// {limit+1}, assuming that values below and including the limit should
// pass. {name} is used for debugging output.
function testLimit(name, min, limit, gen) {
  print(`==== Test ${name} limit = ${limit} ====`);
  function run_validate(count) {
    let expected = count <= limit;
    verbose(`  ${expected ? "(expect ok)  " : "(expect fail)"} = ${count}...`);

    // TODO(titzer): builder is slow for large modules; make manual?
    let builder = new WasmModuleBuilder();
    gen(builder, count);
    let result = WebAssembly.validate(builder.toBuffer());
    
    if (result != expected) {
      let msg = `UNEXPECTED ${expected ? "FAIL" : "PASS"}: ${name} == ${count}`;
      verbose(`=====> UNEXPECTED ${msg}`);
      throw new Error(msg);
    }
  }

  function run_compile(count) {
    let expected = count <= limit;
    verbose(`  ${expected ? "(expect ok)  " : "(expect fail)"} = ${count}...`);

    // TODO(titzer): builder is slow for large modules; make manual?
    let builder = new WasmModuleBuilder();
    gen(builder, count);
    try {
      let result = new WebAssembly.Module(builder.toBuffer());
    } catch (e) {
      if (expected) {
        let msg = `UNEXPECTED FAIL: ${name} == ${count} (${e})`;
        verbose(`=====> UNEXPECTED ${msg}`);
        throw new Error(msg);
      }
      return;
    }
    if (!expected) {
      let msg = `UNEXPECTED PASS: ${name} == ${count}`;
      verbose(`=====> UNEXPECTED ${msg}`);
      throw new Error(msg);
    }
  }

  function run_async_compile(count) {
    let expected = count <= limit;
    verbose(`  ${expected ? "(expect ok)  " : "(expect fail)"} = ${count}...`);

    // TODO(titzer): builder is slow for large modules; make manual?
    let builder = new WasmModuleBuilder();
    gen(builder, count);
    let buffer = builder.toBuffer();
    WebAssembly.compile(buffer)
      .then(result => {
        if (!expected) {
          let msg = `UNEXPECTED PASS: ${name} == ${count}`;
          verbose(`=====> UNEXPECTED ${msg}`);
          throw new Error(msg);
        }
      })
      .catch(err => {
        if (expected) {
          let msg = `UNEXPECTED FAIL: ${name} == ${count} (${e})`;
          verbose(`=====> UNEXPECTED ${msg}`);
          throw new Error(msg);
        }
      })
  }

  if (kTestValidate) {
    test(() => {
      run_validate(min);
    }, `Validate ${name} mininum`);
    test(() => {
      run_validate(limit);
    }, `Validate ${name} limit`);
    test(() => {
      run_validate(limit+1);
    }, `Validate ${name} over limit`);
  }

  if (kTestSyncCompile) {
    test(() => {
      run_compile(min);
    }, `Compile ${name} mininum`);
    test(() => {
      run_compile(limit);
    }, `Compile ${name} limit`);
    test(() => {
      run_compile(limit+1);
    }, `Compile ${name} over limit`);
  }

  if (kTestAsyncCompile) {
    promise_test(() => {
      run_async_compile(min);
    }, `Async compile ${name} mininum`);
    promise_test(() => {
      run_async_compile(limit);
    }, `Async compile ${name} limit`);
    promise_test(() => {
      run_async_compile(limit+1);
    }, `Async compile ${name} over limit`);
  }
}

testLimit("types", 1, kJSEmbeddingMaxTypes, (builder, count) => {
        for (let i = 0; i < count; i++) {
            builder.addType(kSig_i_i);
        }
    });

testLimit("functions", 1, kJSEmbeddingMaxFunctions, (builder, count) => {
        let type = builder.addType(kSig_v_v);
        let body = [];
        for (let i = 0; i < count; i++) {
            builder.addFunction(/*name=*/ undefined, type).addBody(body);
        }
    });

testLimit("imports", 1, kJSEmbeddingMaxImports, (builder, count) => {
        let type = builder.addType(kSig_v_v);
        for (let i = 0; i < count; i++) {
            builder.addImport("", "", type);
        }
    });

testLimit("exports", 1, kJSEmbeddingMaxExports, (builder, count) => {
        let type = builder.addType(kSig_v_v);
        let f = builder.addFunction(/*name=*/ undefined, type);
        f.addBody([]);
        for (let i = 0; i < count; i++) {
            builder.addExport("f" + i, f.index);
        }
    });

testLimit("globals", 1, kJSEmbeddingMaxGlobals, (builder, count) => {
        for (let i = 0; i < count; i++) {
            builder.addGlobal(kWasmI32, true);
        }
    });

testLimit("data segments", 1, kJSEmbeddingMaxDataSegments, (builder, count) => {
        let data = [];
        builder.addMemory(1, 1, false, false);
        for (let i = 0; i < count; i++) {
            builder.addDataSegment(0, data);
        }
    });

testLimit("initial declared memory pages", 1, kJSEmbeddingMaxMemoryPages,
          (builder, count) => {
            builder.addMemory(count, undefined, false, false);
          });

testLimit("maximum declared memory pages", 1, kJSEmbeddingMaxMemoryPages,
          (builder, count) => {
            builder.addMemory(1, count, false, false);
          });

testLimit("initial imported memory pages", 1, kJSEmbeddingMaxMemoryPages,
          (builder, count) => {
            builder.addImportedMemory("mod", "mem", count, undefined);
          });

testLimit("maximum imported memory pages", 1, kJSEmbeddingMaxMemoryPages,
          (builder, count) => {
            builder.addImportedMemory("mod", "mem", 1, count);
          });

testLimit("function size", 2, kJSEmbeddingMaxFunctionSize, (builder, count) => {
        let type = builder.addType(kSig_v_v);
        let nops = count-2;
        let array = new Array(nops);
        for (let i = 0; i < nops; i++) array[i] = kExprNop;
        builder.addFunction(undefined, type).addBody(array);
    });

testLimit("function locals", 1, kJSEmbeddingMaxFunctionLocals, (builder, count) => {
        let type = builder.addType(kSig_v_v);
        builder.addFunction(undefined, type)
          .addLocals({i32_count: count})
          .addBody([]);
    });

testLimit("function params", 1, kJSEmbeddingMaxFunctionParams, (builder, count) => {
        let array = new Array(count);
        for (let i = 0; i < count; i++) array[i] = kWasmI32;
        let type = builder.addType({params: array, results: []});
    });

testLimit("function params+locals", 1, kJSEmbeddingMaxFunctionLocals - 2, (builder, count) => {
        let type = builder.addType(kSig_i_ii);
        builder.addFunction(undefined, type)
          .addLocals({i32_count: count})
          .addBody([kExprUnreachable]);
    });

testLimit("function returns", 0, kJSEmbeddingMaxFunctionReturns, (builder, count) => {
        let array = new Array(count);
        for (let i = 0; i < count; i++) array[i] = kWasmI32;
        let type = builder.addType({params: [], results: array});
    });

testLimit("initial table size", 1, kJSEmbeddingMaxTableSize, (builder, count) => {
        builder.setTableBounds(count, undefined);
    });

testLimit("maximum table size", 1, kJSEmbeddingMaxTableSize, (builder, count) => {
        builder.setTableBounds(1, count);
    });

testLimit("element segments", 1, kJSEmbeddingMaxElementSegments, (builder, count) => {
        builder.setTableBounds(1, 1);
        let array = [];
        for (let i = 0; i < count; i++) {
            builder.addElementSegment(0, false, array, false);
        }
    });

testLimit("tables", 0, kJSEmbeddingMaxTables, (builder, count) => {
        for (let i = 0; i < count; i++) {
            builder.addImportedTable("", "", 1, 1);
        }
    });

testLimit("memories", 0, kJSEmbeddingMaxMemories, (builder, count) => {
        for (let i = 0; i < count; i++) {
            builder.addImportedMemory("", "", 1, 1, false);
        }
    });

//=======================================================================
// HARNESS SNIPPET, DO NOT COMMIT
//=======================================================================
if (failures.length > 0) {
  throw failures[0];
}
//=======================================================================
