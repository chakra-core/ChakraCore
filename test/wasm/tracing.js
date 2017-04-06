//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.Flag("-wasmI64");
WScript.Flag("-trace:wasminout");
var {i64ToString} = WScript.LoadScriptFile("./wasmutils.js");

const buf = WebAssembly.wabt.convertWast2Wasm(`
(module
  ;; Auxiliary definitions
  (func $const-i32 (export "const-i32") (result i32) (i32.const 0x132))
  (func $const-i64 (export "const-i64") (result i64) (i64.const 0x164))
  (func $const-f32 (export "const-f32") (result f32) (f32.const 0xf32))
  (func $const-f64 (export "const-f64") (result f64) (f64.const 0xf64))

  (func $id-i32 (export "id-i32") (param i32) (result i32) (get_local 0))
  (func $id-i64 (export "id-i64") (param i64) (result i64) (get_local 0))
  (func $id-f32 (export "id-f32") (param f32) (result f32) (get_local 0))
  (func $id-f64 (export "id-f64") (param f64) (result f64) (get_local 0))

  (func $f32-i32 (export "f32-i32") (param f32 i32) (result i32) (get_local 1))
  (func $i32-i64 (export "i32-i64") (param i32 i64) (result i64) (get_local 1))
  (func $f64-f32 (export "f64-f32") (param f64 f32) (result f32) (get_local 1))
  (func $i64-f64 (export "i64-f64") (param i64 f64) (result f64) (get_local 1))

  ;; Typing

  (func (export "type-i32") (result i32) (call $const-i32))
  (func (export "type-i64") (result i64) (call $const-i64))
  (func (export "type-f32") (result f32) (call $const-f32))
  (func (export "type-f64") (result f64) (call $const-f64))

  (func (export "type-first-i32") (param i32) (result i32) (call $id-i32 (get_local 0)))
  (func (export "type-first-i64") (param i64) (result i64) (call $id-i64 (get_local 0)))
  (func (export "type-first-f32") (param f32) (result f32) (call $id-f32 (get_local 0)))
  (func (export "type-first-f64") (param f64) (result f64) (call $id-f64 (get_local 0)))

  (func (export "type-second-i32") (param f32 i32) (result i32) (call $f32-i32 (get_local 0) (get_local 1)))
  (func (export "type-second-i64") (param i32 i64) (result i64) (call $i32-i64 (get_local 0) (get_local 1)))
  (func (export "type-second-f32") (param f64 f32) (result f32) (call $f64-f32 (get_local 0) (get_local 1)))
  (func (export "type-second-f64") (param i64 f64) (result f64) (call $i64-f64 (get_local 0) (get_local 1)))
)`);

WebAssembly.instantiate(buf)
  .then(({instance: {exports}}) => {
    for (const key in exports) {
      const res1 = exports[key](32.123, -5);
      const res2 = exports[key](3, -45.147);
      if (key.endsWith("i64")) {
        console.log(i64ToString(res1));
        console.log(i64ToString(res2));
      } else {
        console.log(res1);
        console.log(res2);
      }
    }
    return "Done";
  })
  .then(console.log, console.log)
