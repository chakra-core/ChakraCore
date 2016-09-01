//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that the property ID is properly enumerated in the case
// of calling RemoteDictionaryTypeHandlerBase<TPropertyIndex>::GetPropertyInfo()
// in the hybrid debugger (no assert in ActivationObjectWalker::IsPropertyValid()
// for the global property 'prop0').
// Bug #241480.

function test0() {
    var obj0 = {};
    var obj1 = {};
    var arrObj0 = {};
    var func0 = function (argFunc0, argMath1, argArr2) {
        arrObj0.prop1 = (this.prop0 %= 1);
        obj0.prop1 %= 1;
        h = 1; /**bp:locals();evaluate('this.prop0')**/
        return 1;
    }
    var func1 = function (argArr3) {
        func0.call(obj0, 1, (func0(1, 1, 1)), 1);
    }
    arrObj0.method0 = func0;
    var h = 1;
    func1(1);
    obj1.length <<= (Object.defineProperty(this, 'prop0', {
        set: function (_x) { },
        configurable: true
    }), 1);
    arrObj0.method0.call(obj0, 1, 1, 1)
};
test0();

WScript.Echo("PASSED");