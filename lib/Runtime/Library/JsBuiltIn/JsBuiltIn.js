//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function (intrinsic) {
    var platform = intrinsic.JsBuiltIn;

    var setPrototype = platform.builtInSetPrototype;
    var _objectDefineProperty = platform.builtInJavascriptObjectEntryDefineProperty;
    var Symbol = platform.Symbol;
    var CreateObject = platform.builtInJavascriptObjectCreate;

    let FunctionsEnum = {
        ArrayValues: setPrototype({ className: "Array", methodName: "values", argumentsCount: 0, forceInline: true /*optional*/, alias: "Symbol.iterator" }, null),
        ArrayKeys: setPrototype({ className: "Array", methodName: "keys", argumentsCount: 0, forceInline: true /*optional*/ }, null),
        ArrayEntries: setPrototype({ className: "Array", methodName: "entries", argumentsCount: 0, forceInline: true /*optional*/ }, null),
        ArrayIndexOf: setPrototype({ className: "Array", methodName: "indexOf", argumentsCount: 1, forceInline: true /*optional*/ }, null),
        ArrayFilter: setPrototype({ className: "Array", methodName: "filter", argumentsCount: 1, forceInline: true /*optional*/ }, null),
    };

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
    __chakraLibrary.raiseNeedObjectOfType = platform.raiseNeedObjectOfType;
    __chakraLibrary.raiseThis_NullOrUndefined = platform.raiseThis_NullOrUndefined;
    __chakraLibrary.raiseFunctionArgument_NeedFunction = platform.raiseFunctionArgument_NeedFunction;
    __chakraLibrary.callInstanceFunc = platform.builtInCallInstanceFunction;
    __chakraLibrary.functionBind = platform.builtInJavascriptFunctionEntryBind;

    _objectDefineProperty(__chakraLibrary.ArrayIterator.prototype, 'next',
        // Object's getter and setter can get overriden on the prototype, in that case while setting the value attributes, we will end up with TypeError
        // So, we need to set the prototype of attributes to null
        setPrototype({
            value: function () {
                "use strict";
                let o = this;

                if (!(o instanceof __chakraLibrary.ArrayIterator)) {
                    __chakraLibrary.raiseNeedObjectOfType("Array Iterator.prototype.next", "Array Iterator");
                }

                let a = o.__$arrayObj$__;

                if (o.__$internalDone$__ === true) {
                    return { value: undefined, done: true };
                } else {
                    let index = o.__$nextIndex$__;
                    let len = __chakraLibrary.isArray(a) ? a.length : __chakraLibrary.GetLength(a);

                    if (index < len) { // < comparison should happen instead of >= , because len can be NaN
                        let itemKind = o.__$kind$__;

                        o.__$nextIndex$__ = index + 1;

                        if (itemKind === 1 /*ArrayIterationKind.Value*/) {
                            return {value : a[index], done : false};
                        } else if (itemKind === 0 /*ArrayIterationKind.Key*/) { // TODO (megupta) : Use clean enums here ?
                            return {value : index, done : false};
                        } else {
                            let elementKey = index;
                            let elementValue = a[index];
                            return {value : [elementKey, elementValue], done : false};
                        }
                    } else {
                        o.__$internalDone$__ = true;
                        return { value : undefined, done : true};
                    }
                }
            },
            writable: true,
            enumerable: false,
            configurable: true
        }, null)
    );

    _objectDefineProperty(__chakraLibrary.ArrayIterator.prototype, Symbol.toStringTag, setPrototype({ value: "Array Iterator", writable: false, enumerable: false, configurable: true }, null));

    _objectDefineProperty(__chakraLibrary.ArrayIterator.prototype.next, 'length', setPrototype({ value: 0, writable: false, enumerable: false, configurable: true }, null));

    _objectDefineProperty(__chakraLibrary.ArrayIterator.prototype.next, 'name', setPrototype({ value: "next", writable: false, enumerable: false, configurable: true }, null));

    platform.registerChakraLibraryFunction("CreateArrayIterator", function (arrayObj, iterationKind) {
        "use strict";
        return new __chakraLibrary.ArrayIterator(arrayObj, iterationKind);
    });

    platform.registerFunction(FunctionsEnum.ArrayKeys, function () {
        "use strict";
        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.keys");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 0 /* ArrayIterationKind.Key*/);
    });

    platform.registerFunction(FunctionsEnum.ArrayValues, function () {
        "use strict";
        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.values");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 1 /* ArrayIterationKind.Value*/);
    });

    platform.registerFunction(FunctionsEnum.ArrayEntries, function () {
        "use strict";
        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.entries");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 2 /* ArrayIterationKind.KeyAndValue*/);
    });

    platform.registerFunction(FunctionsEnum.ArrayIndexOf, function (searchElement, fromIndex) {
        // ECMAScript 2017 #sec-array.prototype.indexof
        "use strict";

        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.indexOf");
        }

        let o;
        if (__chakraLibrary.isArray(this)) {
            o = this;
        } else {
            o = __chakraLibrary.Object(this);
        }

        let len = __chakraLibrary.toLength(o["length"]);
        if (len === 0) {
            return -1;
        }

        let n = __chakraLibrary.toInteger(fromIndex);
        if (n >= len) {
            return -1;
        }

        let k;

        /* We refactored the code but it still respect the spec.
           When using -0 or +0, the engine might think we are meaning
           to use floating point numbers which can hurt performance.
           So we refactored to use integers instead. */
        if (n === 0) {      // Corresponds to "If n is -0, let k be +0" in the spec
            k = 0;
        } else if (n > 0) { // Corresponds to "If n >= 0, then [...] let k be n."
            k = n;
        } else {            // Corresponds to "Else n < 0"
            k = len + n;

            if (k < 0) {
                k = 0;
            }
        }

        while (k < len) {
            if (k in o) {
                let elementK = o[k];

                if (elementK === searchElement) {
                    return k;
                }
            }

            k++;
        }

        return -1;
    });

    platform.registerFunction(FunctionsEnum.ArrayFilter, function (callbackfn, thisArg) {
        // ECMAScript 2017 #sec-array.prototype.filter
        "use strict";

        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.filter");
        }

        let o;
        let len
        if (__chakraLibrary.isArray(this)) {
            o = this;
            len = o.length;
        } else {
            o = __chakraLibrary.Object(this);
            len = __chakraLibrary.GetLength(o);
        }
        
        if (typeof callbackfn != "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.filter");
        }

        let a = __chakraLibrary.arraySpeciesCreate(o, 0);
        let k = 0;
        let to = 0;

        if (thisArg === undefined) {
            // fast path.
            while (k < len) {
                if (k in o) {
                    let kValue = o[k];
                    if (callbackfn(kValue, k, o)) {
                        __chakraLibrary.arrayCreateDataPropertyOrThrow(a, to, kValue);
                        to++;
                    }
                }
                k++;
            }
        } else {
            // slow path.
            // safe equivalent of calling "callbackfn.bind(thisArg)"
            let boundCallback = __chakraLibrary.callInstanceFunc(__chakraLibrary.functionBind, callbackfn, thisArg);
            while (k < len) {
                if (k in o) {
                    let kValue = o[k];
                    if (boundCallback(kValue, k, o)) {
                        __chakraLibrary.arrayCreateDataPropertyOrThrow(a, to, kValue);
                        to++;
                    }
                }
                k++;
            }
        }

        return a;
    });
  
});
