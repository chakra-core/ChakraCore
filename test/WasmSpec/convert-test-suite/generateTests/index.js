//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const genTests = [
  "chakra_i64"
];

module.exports = genTests.map(f => ({name: f, getContent: require(`./${f}`)}));
