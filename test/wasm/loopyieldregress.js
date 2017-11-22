//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
function getBuf() {
  if (WebAssembly.wabt) {
    const buf = WebAssembly.wabt.convertWast2Wasm(`(module
      (memory 1 1)
      (func $_main (export "foo") (param i32 i32) (result i32)
      (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64)
      block
      block
      loop  ;; label = @5
        block  ;; label = @6
          get_local 3
          i32.const -3
          i32.and
          set_local 3
          get_local 3
          i32.const 16
          i32.shl
          i32.const 16
          i32.shr_s
          set_local 1
          get_local 0
          get_local 1
          i32.lt_s
          if (result i32)  ;; label = @7
            get_local 4
            i32.load8_s
            set_local 1
            get_local 1
            i32.const 1
            i32.ne
            br_if 1 (;@6;)
            get_local 9
            i32.const -2115251625
            i32.store
            i32.const -2115251625
            set_local 1
            i32.const 1
          else
            get_local 7
            i32.const 0
            i32.store
            get_local 0
            set_local 1
            get_local 3
          end
          set_local 0
          i32.const 22112
          get_local 6
          i32.const 1
          i32.add
          tee_local 6
          i32.store
          get_local 6
          i32.const 20
          i32.ge_s
          br_if 2 (;@4;)
          get_local 0
          set_local 3
          get_local 1
          set_local 0
          br 1 (;@5;)
        end
      end
      end
      end
      (i32.const 0)
    )
    )`);
    //const view = new Uint8Array(buf);
    //console.log(view.join(","));
    return buf;
  } else {
    const arr = [0,97,115,109,1,0,0,0,1,7,1,96,2,127,127,1,127,3,2,1,0,5,4,1,1,1,1,7,7,1,3,102,111,111,0,0,10,131,1,1,128,1,2,158,1,127,1,126,2,64,2,64,3,64,2,64,32,3,65,125,113,33,3,32,3,65,16,116,65,16,117,33,1,32,0,32,1,72,4,127,32,4,44,0,0,33,1,32,1,65,1,71,13,1,32,9,65,215,164,175,143,120,54,2,0,65,215,164,175,143,120,33,1,65,1,5,32,7,65,0,54,2,0,32,0,33,1,32,3,11,33,0,65,224,172,1,32,6,65,1,106,34,6,54,2,0,32,6,65,20,78,13,2,32,0,33,3,32,1,33,0,12,1,11,11,11,11,65,0,11]
    const buf = new ArrayBuffer(arr.length);
    const view = new Uint8Array(buf);
    for (let i = 0; i < arr.length; ++i) {
      view[i] = arr[i];
    }
    return buf;
  }
}

const mod = new WebAssembly.Module(getBuf());
const {exports} = new WebAssembly.Instance(mod);
console.log(exports.foo() == 0 ? "pass" : "failed");
