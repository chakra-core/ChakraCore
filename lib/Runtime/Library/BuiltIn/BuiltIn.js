//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function (intrinsic) {
    var platform = intrinsic.BuiltIn;

    var FunctionsEnum = {
        ArrayIndexOf: { className: "Array", methodName: "indexOf", argumentsCount: 1 },
    };

    platform.registerFunction(FunctionsEnum.ArrayIndexOf, function (searchElement, fromIndex) {
        return "call to Array.prototype.indexOf => searchElement: " + searchElement + " fromIndex: " + fromIndex;
    });
});
