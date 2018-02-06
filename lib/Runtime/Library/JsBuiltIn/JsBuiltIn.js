//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function (intrinsic) {
    var platform = intrinsic.JsBuiltIn;

    let FunctionsEnum = {
        ArrayValues: { className: "Array", methodName: "values", argumentsCount: 0, forceInline: true /*optional*/, alias: "Symbol.iterator" },
        ArrayKeys: { className: "Array", methodName: "keys", argumentsCount: 0, forceInline: true /*optional*/ },
        ArrayEntries: { className: "Array", methodName: "entries", argumentsCount: 0, forceInline: true /*optional*/ }
    };

    var setPrototype = platform.builtInSetPrototype;
    var _objectDefineProperty = platform.builtInJavascriptObjectEntryDefineProperty;
    var Symbol = platform.Symbol;

    // Object's getter and setter can get overriden on the prototype, in that case while setting the value attributes, we will end up with TypeError
    // So, we need to set the prototype of attributes to null
    var ObjectDefineProperty = function (obj, prop, attributes) {
        _objectDefineProperty(obj, prop, setPrototype(attributes, null));
    };
    var CreateObject = platform.builtInJavascriptObjectCreate;

    platform.registerChakraLibraryFunction("ArrayIterator", function (arrayObj, iterationKind) {
        "use strict";
        __chakraLibrary.InitInternalProperties(this, 4, "__$arrayObj$__", "__$nextIndex$__", "__$kind$__", "__$internalDone$__");
        this.__$arrayObj$__ = arrayObj;
        this.__$nextIndex$__ = 0;
        this.__$kind$__ = iterationKind;
        this.__$internalDone$__ = false; // We use this additional property to enable hoisting load of arrayObj outside the loop, we write to this property instead of the arrayObj
    });

    // ArrayIterator's prototype is the C++ Iterator, which is also the prototype for StringIterator, MapIterator etc
    var iteratorPrototype = platform.GetIteratorPrototype();
    // Establish prototype chain here
    __chakraLibrary.ArrayIterator.prototype = CreateObject(iteratorPrototype);

    ObjectDefineProperty(__chakraLibrary.ArrayIterator.prototype, 'next',
        {
            value: function () {
                "use strict";
                let o = this;

                if (!(o instanceof __chakraLibrary.ArrayIterator)) {
                    throw new TypeError("Array Iterator.prototype.next: 'this' is not an Array Iterator object");
                }

                let a = o.__$arrayObj$__;
                let value, done;

                if (o.__$internalDone$__ === true) {
                    value = undefined;
                    done = true;
                } else {
                    let index = o.__$nextIndex$__;
                    let len = __chakraLibrary.isArray(a) ? a.length : __chakraLibrary.GetLength(a);

                    if (index < len) { // < comparison should happen instead of >= , because len can be NaN
                        let itemKind = o.__$kind$__;

                        o.__$nextIndex$__ = index + 1;

                        if (itemKind === 1 /*ArrayIterationKind.Value*/) {
                            value = a[index];
                        } else if (itemKind === 0 /*ArrayIterationKind.Key*/) { // TODO (megupta) : Use clean enums here ?
                            value = index;
                        } else {
                            let elementKey = index;
                            let elementValue = a[index];
                            value = [elementKey, elementValue];
                        }
                        done = false;
                    } else {
                        o.__$internalDone$__ = true;
                        value = undefined;
                        done = true;
                    }
                }
                return { value: value, done: done };
            },
            writable: true,
            enumerable: false,
            configurable: true
        }
    );

    ObjectDefineProperty(__chakraLibrary.ArrayIterator.prototype, Symbol.toStringTag, { value: "Array Iterator", writable: false, enumerable: false, configurable: true });

    ObjectDefineProperty(__chakraLibrary.ArrayIterator.prototype.next, 'length', { value: 0, writable: false, enumerable: false, configurable: true });

    ObjectDefineProperty(__chakraLibrary.ArrayIterator.prototype.next, 'name', { value: "next", writable: false, enumerable: false, configurable: true });

    platform.registerChakraLibraryFunction("CreateArrayIterator", function (arrayObj, iterationKind) {
        "use strict";
        return new __chakraLibrary.ArrayIterator(arrayObj, iterationKind);
    });

    platform.registerFunction(FunctionsEnum.ArrayKeys, function () {
        "use strict";
        if (this === null || this === undefined) {
            throw new TypeError("Array.prototype.keys: 'this' is null or undefined");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 0 /* ArrayIterationKind.Key*/);
    });

    platform.registerFunction(FunctionsEnum.ArrayValues, function () {
        "use strict";
        if (this === null || this === undefined) {
            throw new TypeError("Array.prototype.values: 'this' is null or undefined");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 1 /* ArrayIterationKind.Value*/);
    });

    platform.registerFunction(FunctionsEnum.ArrayEntries, function () {
        "use strict";
        if (this === null || this === undefined) {
            throw new TypeError("Array.prototype.entries: 'this' is null or undefined");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 2 /* ArrayIterationKind.KeyAndValue*/);
    });
});