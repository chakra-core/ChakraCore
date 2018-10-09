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
    var CreateObject = platform.builtInJavascriptObjectEntryCreate;

    platform.registerChakraLibraryFunction("ArrayIterator", function (arrayObj, iterationKind) {
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
    __chakraLibrary.raiseNeedObject = platform.raiseNeedObject;
    __chakraLibrary.raiseNonObjectFromIterable = platform.raiseNonObjectFromIterable;
    __chakraLibrary.raiseEmptyArrayAndInitValueNotPresent = platform.raiseEmptyArrayAndInitValueNotPresent;
    __chakraLibrary.raiseLengthIsTooBig = platform.raiseLengthIsTooBig;
    __chakraLibrary.raiseFunctionArgument_NeedFunction = platform.raiseFunctionArgument_NeedFunction;
    __chakraLibrary.functionBind = platform.builtInJavascriptFunctionEntryBind;
    __chakraLibrary.objectDefineProperty = _objectDefineProperty;
    __chakraLibrary.positiveInfinity = platform.POSITIVE_INFINITY;
    __chakraLibrary.negativeInfinity = platform.NEGATIVE_INFINITY;

    _objectDefineProperty(__chakraLibrary.ArrayIterator.prototype, 'next',
        // Object's getter and setter can get overriden on the prototype, in that case while setting the value attributes, we will end up with TypeError
        // So, we need to set the prototype of attributes to null
        setPrototype({
            value: function () {
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
        return new __chakraLibrary.ArrayIterator(arrayObj, iterationKind);
    });

    platform.registerFunction(platform.FunctionKind.Array_keys, function () {
        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.keys");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 0 /* ArrayIterationKind.Key*/);
    });

    platform.registerFunction(platform.FunctionKind.Array_values, function () {
        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.values");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 1 /* ArrayIterationKind.Value*/);
    });

    platform.registerFunction(platform.FunctionKind.Array_entries, function () {
        if (this === null || this === undefined) {
            __chakraLibrary.raiseThis_NullOrUndefined("Array.prototype.entries");
        }
        let o = __chakraLibrary.Object(this);
        return __chakraLibrary.CreateArrayIterator(o, 2 /* ArrayIterationKind.KeyAndValue*/);
    });

    platform.registerFunction(platform.FunctionKind.Array_indexOf, function (searchElement, fromIndex = undefined) {
        // ECMAScript 2017 #sec-array.prototype.indexof

        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.indexOf");

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

    platform.registerChakraLibraryFunction("CheckArrayAndGetLen", function (obj, builtInFunc) {
        if (__chakraLibrary.isArray(obj)) {
            return { o: obj, len: obj.length };
        } else {
            if (obj === null || obj === undefined) {
                __chakraLibrary.raiseThis_NullOrUndefined(builtInFunc);
            }
            return { o: __chakraLibrary.Object(obj), len: __chakraLibrary.toLength(obj["length"]) };
        }
    });

    platform.registerChakraLibraryFunction("MergeSort", function(array, length, compareFn) {
        const buffer = [];
        buffer.__proto__ = null;

        let bucketSize = 2, lastSize = 1, position = 0;
        const doubleLength = length + length;

        while (bucketSize < doubleLength) {
            while (position < length) {
                const left = position;
                const mid = left + lastSize;

                // perform a merge but only if it's necessary
                if (mid < length && compareFn(array[mid], array[mid - 1]) < 0) {
                    let right = position + bucketSize;
                    right = right < length ? right : length;
                    let i = mid - 1, j = 0, k = mid;

                    while (k < right) {
                        buffer[j++] = array[k++];
                    }
    
                    let rightElement = buffer[--j];
                    let leftElement = array[i];

                    for (;;) {
                        if (compareFn(rightElement, leftElement) < 0) {
                            array[--k] = leftElement;
                            if (i > left) {
                                leftElement = array[--i];
                            } else {
                                array[--k] = rightElement;
                                break;
                            }
                        } else {
                            array[--k] = rightElement;
                            if (j > 0) {
                                rightElement = buffer[--j];
                            } else {
                                break;
                            }
                        }
                    }

                    while (j > 0) {
                        array[--k] = buffer[--j];
                    }
                }  
                position += bucketSize;
            }
            position = 0;
            lastSize = bucketSize;
            bucketSize *= 2;
        }
    });

    platform.registerChakraLibraryFunction("DefaultStringSortCompare", function(left, right) {
        // this version is used when the array was already strings
        // as the sort only ever checks for < 0 on the return value of compare functions
        // only have to handle this case
        if (left < right) {
            return -1;
        }
        return 0;
    });

    platform.registerChakraLibraryFunction("DefaultSortCompare", function(left, right) {
        // as the sort only ever checks for < 0 on the return value of compare functions
        // only have to handle this case
        if (left.string < right.string) {
            return -1;
        }
        return 0;
    });

    platform.registerChakraLibraryFunction("CreateCompareArray", function(array, length) {
        let useCompareArray = false;
        let i = 0;
        while (i < length) {
            if (typeof array[i++] !== "string") {
                useCompareArray = true;
                break;
            }
        }

        if (useCompareArray === true) {
            const compArray = [];
            compArray.__proto__ = null;
            i = 0;
            let value;
            while (i < length) {
                value = array[i];
                compArray[i++] = {
                    value : value,
                    string : "" + value
                };
            }
            return compArray;
        }
        return array;
    });

    platform.registerChakraLibraryFunction("FillArrayHoles", function(array, length, offset) {
        let i = offset, j = offset, holes = 0;
        let value;
        while (i < length) {
            value = array[i];
            if (value !== undefined) {
                array[j++] = value;
            } else if (!(i in array)) {
                ++holes;
            }
            ++i;
        }

        const valuesLength = j;
        const hasLength = length - holes;
        while (j < hasLength) {
            array[j++] = undefined;
        }
        while (j < length) {
            delete array[j++];
        }
        return valuesLength;
    });

    platform.registerFunction(platform.FunctionKind.Array_sort, function (compareFn) {
        //#sec-array.prototype.sort
        if (compareFn !== undefined && typeof compareFn !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.sort");
        }

        const {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.sort");

        if (len < 2) { // early return if length < 2
            return o;
        }

        // check for if the array has any missing values
        // also pull in any values from the prototype
        let i = 0, length = len;
        while (i < len) {
            if (o[i] === undefined) {
                length = __chakraLibrary.FillArrayHoles(o, len, i);
                break;
            }
            o[i] = o[i++];
        }

        let compArray = o;
        if (compareFn === undefined && length > 1) {
            compArray = __chakraLibrary.CreateCompareArray(o, length);
            if (compArray === o) {
                compareFn = __chakraLibrary.DefaultStringSortCompare;
            } else {
                compareFn = __chakraLibrary.DefaultSortCompare;
            }
        }

        // for short arrays perform an insertion sort
        if (length < 2048) {
            let sortedCount = 1, lowerBound = 0, insertPoint = 0, upperBound = 0;
            while (sortedCount < length) {
                const item = compArray[sortedCount];
                upperBound = sortedCount;
                insertPoint = sortedCount - 1; // this lets us check for already ordered first
                lowerBound = 0;
                for (;;) {
                    if (compareFn (item, compArray[insertPoint]) < 0) {
                        upperBound = insertPoint;
                    } else {
                        lowerBound = insertPoint + 1;
                    }
                    if (lowerBound >= upperBound) {
                        break;
                    }
                    insertPoint = lowerBound + (upperBound - lowerBound >> 1);
                }
                insertPoint = sortedCount;
                while (insertPoint > lowerBound) {
                    compArray[insertPoint--] = compArray[insertPoint];
                }
                compArray[lowerBound] = item;
                ++sortedCount;
            }
        } else {
            __chakraLibrary.MergeSort(compArray, length, compareFn);
        }

        if (compArray !== o) {
            i = 0;
            while (i < length) {
                o[i] = compArray[i++].value;
            }
        }

        return o;
    });

    platform.registerFunction(platform.FunctionKind.Array_filter, function (callbackfn, thisArg = undefined) {
        // ECMAScript 2017 #sec-array.prototype.filter

        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.filter");
        
        if (typeof callbackfn !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.filter");
        }

        let a = __chakraLibrary.arraySpeciesCreate(o, 0);
        let k = 0;
        let to = 0;

        while (k < len) {
            if (k in o) {
                let kValue = o[k];
                if (__chakraLibrary.builtInCallInstanceFunction(callbackfn, thisArg, kValue, k, o)) {
                    __chakraLibrary.arrayCreateDataPropertyOrThrow(a, to, kValue);
                    to++;
                }
            }
            k++;
        }

        return a;
    });

    platform.registerChakraLibraryFunction("FlattenIntoArray", function(target, source, sourceLen, start, depth)
    {
        // this is FlattenIntoArray from the flat/flatMap proposal BUT with no mapperFunction
        // a seperate function has been made to handle the case where there is a mapperFunction

        //1. Let targetIndex be start.
        let targetIndex = start;
        //2. Let sourceIndex be 0.
        let sourceIndex = 0;
        //3. Repeat, while sourceIndex < sourceLen
        let element;
        while (sourceIndex < sourceLen) {
            // a. Let P be ! ToString(sourceIndex).
            // b. Let exists be ? HasProperty(source, P).
            if (sourceIndex in source) {
                // c. If exists is true, then
                //  i. Let element be ? Get(source, P).
                element = source[sourceIndex];
                //  ii. If mapperFunction is present - skipped see separate function
                //  iii. Let shouldFlatten be false.
                //  iv. If depth > 0, then
                //      1. Set shouldFlatten to ? IsArray(element).
                if (depth > 0 && __chakraLibrary.isArray(element)) {
                    // v. If shouldFlatten is true, then
                    //  1. Let elementLen be ? ToLength(? Get(element, "length")).
                    //  2. Set targetIndex to ? FlattenIntoArray(target, element, elementLen, targetIndex, depth - 1).
                    targetIndex = __chakraLibrary.FlattenIntoArray(target, element, __chakraLibrary.toLength(element.length), targetIndex, depth - 1);
                } else {
                    // vi. Else,
                    //  1. If targetIndex >= 2^53-1, throw a TypeError exception.
                    if (targetIndex >= 9007199254740991 /* 2^53-1 */) {
                        __chakraLibrary.raiseLengthIsTooBig("Array.prototype.flat");
                    }
                    // 2. Perform ? CreateDataPropertyOrThrow(target, ! ToString(targetIndex), element).
                    __chakraLibrary.arrayCreateDataPropertyOrThrow(target, targetIndex, element);
                    // 3. Increase targetIndex by 1.
                    ++targetIndex;
                }
            }
            //  d. Increase sourceIndex by 1.
            ++sourceIndex;
        }
        //4. Return targetIndex.
        return targetIndex;
    });

    platform.registerChakraLibraryFunction("FlattenIntoArrayMapped", function(target, source, sourceLen, start, mapperFunction, thisArg) {
        // this is FlattenIntoArray from the flat/flatMap proposal BUT with:
        // depth = 1 and the presence of a mapperFunction guaranteed
        // both these conditions are always met when this is called from flatMap
        // Additionally this is slightly refactored rather than taking a thisArg
        // the calling function binds the thisArg if it's required
        //1. Let targetIndex be start.
        let targetIndex = start;
        //2. Let sourceIndex be 0.
        let sourceIndex = 0;
        //3. Repeat, while sourceIndex < sourceLen

        let element, innerLength, innerIndex;
        while (sourceIndex < sourceLen) {
            // a. Let P be ! ToString(sourceIndex).
            // b. Let exists be ? HasProperty(source, P).
            if (sourceIndex in source) {
                // c. If exists is true, then
                //  i. Let element be ? Get(source, P).
                //  ii. If mapperFunction is present, then
                //      1. Assert: thisArg is present.
                //      2. Set element to ? Call(mapperFunction, thisArg , element, sourceIndex, source).
                element = __chakraLibrary.builtInCallInstanceFunction(mapperFunction, thisArg, source[sourceIndex], sourceIndex, source);
                //  iii. Let shouldFlatten be false.
                //  iv. If depth > 0, then
                //      1. Set shouldFlatten to ? IsArray(element).
                //  v. If shouldFlatten is true, then
                //      1. Let elementLen be ? ToLength(? Get(element, "length")).
                //      2. Set targetIndex to ? FlattenIntoArray(target, element, elementLen, targetIndex, depth - 1).
                if (__chakraLibrary.isArray(element)) {
                    // instead of calling FlattenIntoArray use a simple loop here - as depth is always 0
                    innerLength = __chakraLibrary.toLength(element.length);
                    innerIndex = 0;
                    while (innerIndex < innerLength) {
                        if (innerIndex in element) {
                            //  1. If targetIndex >= 2^53-1, throw a TypeError exception.
                            if (targetIndex >= 9007199254740991 /* 2^53-1 */) {
                                __chakraLibrary.raiseLengthIsTooBig("Array.prototype.flatMap");
                            }
                            //  2. Perform ? CreateDataPropertyOrThrow(target, ! ToString(targetIndex), element).
                            __chakraLibrary.arrayCreateDataPropertyOrThrow(target, targetIndex, element[innerIndex]);
                            //  3. Increase targetIndex by 1.
                            ++targetIndex;
                        }
                        ++innerIndex;
                    }
                } else {
                    //  vi. Else,
                    //      1. If targetIndex >= 2^53-1, throw a TypeError exception.
                    if (targetIndex >= 9007199254740991 /* 2^53-1 */) {
                        __chakraLibrary.raiseLengthIsTooBig("Array.prototype.flatMap");
                    }
                    //  2. Perform ? CreateDataPropertyOrThrow(target, ! ToString(targetIndex), element).
                    __chakraLibrary.arrayCreateDataPropertyOrThrow(target, targetIndex, element);
                    //  3. Increase targetIndex by 1.
                    ++targetIndex;
                }
            }
            //  d. Increase sourceIndex by 1.
            ++sourceIndex;
        }
        //4. Return targetIndex.
        return targetIndex;
    });

    platform.registerFunction(platform.FunctionKind.Array_flat, function (depth = undefined) {
        //1. Let O be ? ToObject(this value).
        //2. Let sourceLen be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.flat");

        //3. Let depthNum be 1.
        //4. If depth is not undefined, then
        //5. Set depthNum to ? ToInteger(depth).
        const depthNum = depth !== undefined ? __chakraLibrary.toInteger(depth) : 1;
        //6. Let A be ? ArraySpeciesCreate(O, 0).
        const A = __chakraLibrary.arraySpeciesCreate(o, 0);
        //7. Perform ? FlattenIntoArray(A, O, sourceLen, 0, depthNum).
        __chakraLibrary.FlattenIntoArray(A, o, len, 0, depthNum);
        //8. Return A.
        return A;
    });

    platform.registerFunction(platform.FunctionKind.Array_flatMap, function (mapperFunction, thisArg = undefined) {
        //1. Let O be ? ToObject(this value).
        //2. Let sourceLen be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.flatMap");

        //3. If IsCallable(mapperFunction) is false throw a TypeError exception
        if (typeof mapperFunction !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.flatMap");
        }
        //4. If thisArg is present, let T be thisArg; else let T be undefined
        //5. Let A be ? ArraySpeciesCreate(O, 0).
        const A = __chakraLibrary.arraySpeciesCreate(o, 0);
        //6. Perform ? FlattenIntoArray(A, O, sourceLen, 0, depthNum).
        __chakraLibrary.FlattenIntoArrayMapped(A, o, len, 0, mapperFunction, thisArg);
        //7. Return A.
        return A;
    });

    platform.registerFunction(platform.FunctionKind.Array_forEach, function (callbackfn, thisArg = undefined) {
        // ECMAScript 2017 #sec-array.prototype.foreach

        //Let O be ? ToObject(this value).
        //Let len be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.forEach");
        
        //If IsCallable(callbackfn) is false, throw a TypeError exception.
        if (typeof callbackfn !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.forEach");
        }

        //If thisArg is present, let T be thisArg; else let T be undefined.
        //Let k be 0.
        let k = 0;

        //Repeat, while k < len
        while (k < len) {
            //Let Pk be ! ToString(k).
            //Let kPresent be ? HasProperty(O, Pk).
            //If kPresent is true, then
            if (k in o) {
                //Let kValue be ? Get(O, Pk).
                let kValue = o[k];
                //Perform ? Call(callbackfn, T, kValue, k, O ).
                __chakraLibrary.builtInCallInstanceFunction(callbackfn, thisArg, kValue, k, o);
            }
            //Increase k by 1.
            k++;
        }
        //Return undefined. 
        return undefined;
    });

    platform.registerFunction(platform.FunctionKind.Array_some, function (callbackfn, thisArg = undefined) {
        // ECMAScript 2017 #sec-array.prototype.some
        
        //Let O be ? ToObject(this value).
        //Let len be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.some");

        //If IsCallable(callbackfn) is false, throw a TypeError exception.
        if (typeof callbackfn !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.some");
        }

        //If thisArg is present, let T be thisArg; else let T be undefined.
        //Let k be 0.
        let k = 0;

        //Repeat, while k < len
        while (k < len) {
            //Let Pk be ! ToString(k).
            //Let kPresent be ? HasProperty(O, Pk).
            //If kPresent is true, then
            if (k in o) {
                //Let kValue be ? Get(O, Pk).
                let kValue = o[k];
                //Let testResult be ToBoolean(? Call(callbackfn, T, kValue, k, O )).
                //If testResult is true, return true.
                if (__chakraLibrary.builtInCallInstanceFunction(callbackfn, thisArg, kValue, k, o)) {
                    return true;
                }
            }
            //Increase k by 1.
            k++;
        }
        //Return false. 
        return false;
    });

    platform.registerFunction(platform.FunctionKind.Array_every, function (callbackfn, thisArg = undefined) {
        // ECMAScript 2017 #sec-array.prototype.every
        
        //Let O be ? ToObject(this value).
        //Let len be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.every");

        //If IsCallable(callbackfn) is false, throw a TypeError exception.
        if (typeof callbackfn !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.every");
        }

        //If thisArg is present, let T be thisArg; else let T be undefined.
        //Let k be 0.
        let k = 0;

        //Repeat, while k < len
        while (k < len) {
            //Let Pk be ! ToString(k).
            //Let kPresent be ? HasProperty(O, Pk).
            //If kPresent is true, then
            if (k in o) {
                //Let kValue be ? Get(O, Pk).
                let kValue = o[k];
                //Let testResult be ToBoolean(? Call(callbackfn, T, kValue, k, O )).
                //If testResult is false, return false.
                if (!__chakraLibrary.builtInCallInstanceFunction(callbackfn, thisArg, kValue, k, o)) {
                    return false;
                }
            }
            //Increase k by 1.
            k++;
        }
        //Return true. 
        return true;
    });

    platform.registerFunction(platform.FunctionKind.Array_includes, function (searchElement, fromIndex = undefined) {
        // ECMAScript 2017 #sec-array.prototype.includes

        //Let O be ? ToObject(this value).
        //Let len be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this,"Array.prototype.includes");

        //If len is 0, return false.
        if (len === 0) {
            return false;
        }

        //Let n be ? ToInteger(fromIndex).
        //Assert: If fromIndex is undefined, then n is 0.
        let n = __chakraLibrary.toInteger(fromIndex);
        let k;

        //If n >= 0, then
        //  Let k be n.
        //Else n < 0,
        //  Let k be len + n.
        //  If k < 0, set k to 0.
        if (n >= 0) {
            k = n;
        } 
        else {
            k = len + n;

            if (k < 0) {
                k = 0;
            }
        }

        //Repeat, while k < len
        while (k < len) {
            //Let elementK be the result of ? Get(O, ! ToString(k)).
            let elementK = o[k];
            //If SameValueZero(searchElement, elementK) is true, return true.
            if ((searchElement === elementK) || (searchElement !== searchElement && elementK !== elementK)) { // check for isNaN
                return true;
            }
            //Increase k by 1.
            k++;
        }
        //Return false. 
        return false;
    });

    platform.registerFunction(platform.FunctionKind.Array_reduce, function (callbackfn, initialValue = undefined) {
        // ECMAScript 2017 #sec-array.prototype.reduce

        //Let O be ? ToObject(this value).
        //Let len be ? ToLength(? Get(O, "length")).
        let {o, len} = __chakraLibrary.CheckArrayAndGetLen(this, "Array.prototype.reduce");

        //If IsCallable(callbackfn) is false, throw a TypeError exception.
        if (typeof callbackfn !== "function") {
            __chakraLibrary.raiseFunctionArgument_NeedFunction("Array.prototype.reduce");
        }

        //If len is 0 and initialValue is not present, throw a TypeError exception.
        if (len === 0 && initialValue === undefined) {
            __chakraLibrary.raiseEmptyArrayAndInitValueNotPresent("Array.prototype.reduce"); 
        }

        //Let k be 0.
        //Let accumulator be undefined.
        let k = 0;
        let accumulator = undefined;

        //If initialValue is present, then
        //Set accumulator to initialValue.
        if (arguments.length > 1) { //Checking for array length because intialValue could be passed in as undefined
            accumulator = initialValue;
        }
        //Else initialValue is not present,
        else {
            //Let kPresent be false.
            let kPresent = false;
            //Repeat, while kPresent is false and k < len
            while (!kPresent && k < len) {
                //Let Pk be ! ToString(k).
                //Set kPresent to ? HasProperty(O, Pk).
                //If kPresent is true, then
                //  Set accumulator to ? Get(O, Pk).
                if (k in o) {
                    kPresent = true;
                    accumulator = o[k];
                }
                //Increase k by 1.
                k++;
            }
            //If kPresent is false, throw a TypeError exception.
            if (!kPresent) {
                __chakraLibrary.raiseEmptyArrayAndInitValueNotPresent("Array.prototype.reduce"); 
            }
        }

        //Repeat, while k < len
        while (k < len) {
            //Let Pk be ! ToString(k).
            //Let kPresent be ? HasProperty(O, Pk).
            //If kPresent is true, then
            if (k in o) {
                //Let kValue be ? Get(O, Pk).
                let kValue = o[k];
                //Set accumulator to ? Call(callbackfn, undefined, accumulator, kValue, k, O ).
                accumulator = __chakraLibrary.builtInCallInstanceFunction(callbackfn, undefined, accumulator, kValue, k, o);
            }
            //Increase k by 1.
            k++;
        }
        //Return accumulator. 
        return accumulator;
    });

    platform.registerFunction(platform.FunctionKind.Object_fromEntries, function (iterable) {
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

    platform.registerFunction(platform.FunctionKind.Math_min, function (value1, value2) {
        // #sec-math.min

        // If no arguments are given, the result is positive infinity
        // If any value is NaN, the result is NaN. 
        // The comparison of values to determine the smallest value is done using the Abstract Relational Comparison algorithm except that +0 is considered to be larger than -0.
        if (arguments.length === 0 ) {
            return __chakraLibrary.positiveInfinity;
        }
        
        value1 = +value1;
        if (value1 !== value1) {
            return NaN;
        }

        if (arguments.length === 1) {
            return value1;
        }

        if (arguments.length === 2) {
            value2 = +value2;
            if (value2 !== value2) {
                return NaN;
            }
            if ((value1 < value2) || (value1 === value2 && value1 === 0 && 1/value1 < 1/value2)) { // checks for -0 and +0
                return value1;
            }
            else {
                return value2;
            }
        }

        let min = value1;
        let nextVal;

        for (let i = 1; i < arguments.length; i++) {
            nextVal = +arguments[i];
            if (nextVal !== nextVal) {
                return NaN;
            }
            else if ((min > nextVal) || (min === nextVal && min === 0 && 1/min > 1/nextVal)) { // checks for -0 and +0
                min = nextVal;
            }
        }
        
        return min;
    });

    platform.registerFunction(platform.FunctionKind.Math_max, function (value1, value2) {
        // #sec-math.max

        // If no arguments are given, the result is negative infinity
        // If any value is NaN, the result is NaN. 
        // The comparison of values to determine the largest value is done using the Abstract Relational Comparison algorithm except that +0 is considered to be larger than -0.
        if (arguments.length === 0) {
            return __chakraLibrary.negativeInfinity;
        }
        
        value1 = +value1;
        if (value1 !== value1) {
            return NaN;
        }

        if (arguments.length === 1) {
            return value1;
        }

        if (arguments.length === 2) {
            value2 = +value2;
            if (value2 !== value2) {
                return NaN;
            }
            if ((value1 > value2) || (value1 === value2 && value1 === 0 && 1/value1 > 1/value2)) { // checks for -0 and +0
                return value1;
            }
            else {
                return value2;
            }
        }

        let max = value1;
        let nextVal;

        for (let i = 1; i < arguments.length; i++) {
            nextVal = +arguments[i];
            if (nextVal !== nextVal) {
                return NaN;
            }
            else if ((max < nextVal) || (max === nextVal && max === 0 && 1/max < 1/nextVal)) { // checks for -0 and +0
                max = nextVal;
            } 
        }
        
        return max;
    });
});
