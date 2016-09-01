//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// Display __proto__ or [prototype]
//

Object.keys(this).forEach(function (p) {
    Object.defineProperty(this, p, { enumerable: false });
});
Object.defineProperty(this, "__saved__proto__desc", {
    value: Object.getOwnPropertyDescriptor(Object.prototype, "__proto__"),
    enumerable: false,
});

(function () {
    arguments = null;
    this; /**bp:locals(2)**/

    Object.defineProperty(Object.prototype, "__proto__", { get: function () { } });
    this; /**bp:locals(2)**/

    Object.defineProperty(Object.prototype, "__proto__", { get: __saved__proto__desc.get });
    this; /**bp:locals(2)**/

    Object.defineProperty(Object.prototype, "__proto__", { set: function () { } });
    this; /**bp:locals(2)**/

    Object.defineProperty(Object.prototype, "__proto__", { set: __saved__proto__desc.set });
    this; /**bp:locals(2)**/

    Object.defineProperty(Object.prototype, "__proto__", {
        get: function () { return __saved__proto__desc.get.apply(this); },
        set: function (p) { return __saved__proto__desc.set.apply(this, [p]); },
    });
    this.__proto__ = { pp2: 2 };
    this; /**bp:locals(2)**/

    Object.defineProperty(Object.prototype, "__proto__", { value: 123 });
    this; /**bp:locals(3)**/

    Object.defineProperty(Object.prototype, "__proto__", __saved__proto__desc);
    this; /**bp:locals(2)**/

}).apply({ thisp: 1, __proto__: { protop: 123 } });

WScript.Echo("pass");
