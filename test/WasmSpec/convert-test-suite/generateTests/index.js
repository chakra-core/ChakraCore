//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
const chakraIntConst = require("./chakra_intConst");

const genTests = [
  {name: "chakra_i64", getContent: chakraIntConst.bind(null, "i64")},
  {name: "chakra_i32", getContent: chakraIntConst.bind(null, "i32")},
];

module.exports = genTests;
