//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

class dummy {
    constructor() {
        return new Int16Array(4);
    }
}

var handler = {
    get: function(oTarget, sKey) {
        if (sKey.toString()=="constructor") {
            return { [Symbol.species] : dummy };
        } else {
            return 4;
        }
    },

    has: function (oTarget, sKey) {
        return Reflect.has(oTarget, sKey);
    },
};

var array = [1];
var proxy = new Proxy(array, handler);

try
{
    // By spec, Array.prototype.filter (and other built-ins) adds configurable properties to a new array, created from ArraySpeciesCreate. 
    // If the constructed array is a TypedArray, setting of index properties should throw a type error because they cannot be configurable.
    var boundFilter = Array.prototype.filter.bind(proxy);
    boundFilter(function() { return true; });
    WScript.Echo("TypeError expected. TypedArray indicies should be non-configurable.");
}
catch (e)
{
    if (e == "TypeError: Cannot redefine property '0'")
    {
        WScript.Echo("passed");
    }
}
