//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var externalContextPromise = (function () {
    let resolvePromise;
    let promise = new Promise((resolve, reject) => {
        resolvePromise = resolve;

    });
    promise = promise.then(()=> {
        throw new Error("error from externalContextPromise1");
    })

    return {
        promise,
        resolvePromise
    };
})();
