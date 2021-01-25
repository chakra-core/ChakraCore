//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function (intrinsic) {
    var platform = intrinsic.JsBuiltIn;

    __chakraLibrary.raiseNeedObject = platform.raiseNeedObject;
    __chakraLibrary.raiseNonObjectFromIterable = platform.raiseNonObjectFromIterable;
    __chakraLibrary.objectDefineProperty = platform.builtInJavascriptObjectEntryDefineProperty;

    platform.registerFunction('fromEntries', function (iterable) {
        // #sec-object.fromentries
        if (iterable === null || iterable === undefined) {
            __chakraLibrary.raiseNeedObject("Object.fromEntries");
        }

        const o = {};
        const propDescriptor = {
            enumerable : true,
            configurable : true,
            writable : true,
            value : undefined
        };

        let key;
        for (const entry of iterable) {
            if (typeof entry !== "object" || entry === null) {
                __chakraLibrary.raiseNonObjectFromIterable("Object.fromEntries");
            }

            key = entry[0];
            propDescriptor.value = entry[1];
            __chakraLibrary.objectDefineProperty(o, key, propDescriptor);
        }
        return o;
    });
});
