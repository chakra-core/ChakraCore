//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
const chakraIntConst = require("./chakra_intConst");
const path = require("path");

const genTests = [
  {name: "chakra_i64", getContent(rlRoot) {
    return chakraIntConst("i64", path.join(rlRoot, "testsuite", "core"));
  }},
  {name: "chakra_i32", getContent(rlRoot) {
    return chakraIntConst("i32", path.join(rlRoot, "testsuite", "core"));
  }},
  {name: "chakra_extends_i32", getContent(rlRoot) {
    return chakraIntConst("extends_i32", path.join(rlRoot, "features", "extends"));
  }},
  {name: "chakra_extends_i64", getContent(rlRoot) {
    return chakraIntConst("extends_i64", path.join(rlRoot, "features", "extends"));
  }},
];

module.exports = genTests;
