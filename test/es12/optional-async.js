//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check

const simpleObj = { null: null, undefined: undefined, something: 42 };
Object.freeze(simpleObj);

// Short-circuiting ignores indexer expression and method args
(async () => {
    simpleObj.nothing?.[await Promise.reject()];
    simpleObj.null?.[await Promise.reject()];
    simpleObj.undefined?.[await Promise.reject()];
})().then(
    () => {
        console.log("pass");
    },
    (x) => console.log(x)
);
