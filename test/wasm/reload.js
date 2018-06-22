//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test(shared) {
  if (shared) {
    WScript.Flag("-WasmThreads");
  }
  const memPrefix = shared ? "i32.atomic" : "i32";
  const buf = WebAssembly.wabt.convertWast2Wasm(`
  (module
    (import "test" "grow" (func $grow_import))
    (memory (export "mem") 1 100 ${shared ? "shared" : ""})
    (func $grow (memory.grow (i32.const 1)) drop)
    (func $empty)
    (func (export "test") (param $i i32) (result i32)
      (local $v i32)
      (i32.ge_s (get_local $i) (i32.const 10))
      (if (result i32)
        ;; not growing scenario
        (then
          (${memPrefix}.store (i32.const 0) (get_local $i))
          (call $empty)
          (i32.load (i32.const 0))
          (set_local $v (i32.add (i32.const 1)))
          (i32.store (i32.const 2) (get_local $v))
          (call $empty)
          (${memPrefix}.load (i32.const 0))
        )
        ;; growing scenario
        (else
          (${memPrefix}.store (i32.const 0) (get_local $i))
          (call $grow)
          (${memPrefix}.load (i32.const 0))
          (set_local $v (i32.add (get_local $i)))
          (i32.store (i32.const 2) (get_local $v))
          (call $grow_import)
          (${memPrefix}.load (i32.const 0))
        )
      )
    )
  )`);

  const hex = v => "0x" + v.toString(16);

  function validate(res, val) {
    const growing = val < 10;
    const expected = !growing
      ? (val & 0xFFFF) + (((val + 1) << 16) & 0xFFFFFFFF)
      : (val & 0xFFFF) + (((val + val) << 16) & 0xFFFFFFFF);
    if (res !== expected) {
      const testType = growing ? ".grow" : "";
      const sharing = shared ? "shared memory" : "normal memory";
      console.log(`${sharing}: test${testType}(${val}) Invalid value. Expected ${hex(expected)}, got ${hex(res)}`);
    }
  }

  const mod = new WebAssembly.Module(buf);
  const {exports} = new WebAssembly.Instance(mod, {
    test: {grow() {
      try {
        // try to detach, should not be possible
        ArrayBuffer.detach(exports.mem.buffer);
        console.log("ArrayBuffer.detach should not have succeeded");
      } catch (e) {
        if (
          // WebAssemblySharedArrayBuffer are not classic ArrayBuffer
          (shared && !e.message.includes("ArrayBuffer object expected")) ||
          (!shared && !e.message.includes("Not allowed to detach WebAssembly.Memory buffer"))
        ) {
           console.log(e);
        }
      }
      exports.mem.grow(1);
    }}
  });

  validate(exports.test(5), 5);

  for (let i = 0; i < 100; ++i) {
    // Warm up with empty calls
    const val = ((i % 20) + 15)|0;
    validate(exports.test(val), val);
  }

  validate(exports.test(5), 5);
}

test(false);
test(true);

console.log("pass");
